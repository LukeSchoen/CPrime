function Read-TestPedanticPaths([string]$TestsRoot) {
    $paths = @{}
    $manifest = Join-Path $TestsRoot 'tiers.json'
    if (Test-Path -LiteralPath $manifest) {
        $tiers = Get-Content -LiteralPath $manifest -Raw | ConvertFrom-Json
        if ($tiers.schema -ne 1) { throw 'Unknown test tier schema' }
        foreach ($path in $tiers.pedantic) {
            if (-not $path -or $path -match '(^|/)\.\.(/|$)|\\|^/|:' -or $paths.ContainsKey($path)) {
                throw "Invalid or duplicate tier path: $path"
            }
            $paths[$path] = $true
        }
    }
    return $paths
}

function Invoke-TestProcess([string[]]$Command, [ValidateRange(0.001, 300)][double]$Timeout) {
    $timer = [Diagnostics.Stopwatch]::StartNew()
    $process = New-Object Diagnostics.Process
    $process.StartInfo.FileName = $Command[0]
    $process.StartInfo.UseShellExecute = $false
    $process.StartInfo.CreateNoWindow = $true
    $process.StartInfo.RedirectStandardOutput = $true
    $process.StartInfo.RedirectStandardError = $true
    $process.StartInfo.StandardOutputEncoding = [Text.Encoding]::UTF8
    $process.StartInfo.StandardErrorEncoding = [Text.Encoding]::UTF8
    # Windows argv quoting; never pass compiler arguments through a command shell.
    $process.StartInfo.Arguments = (@($Command | Select-Object -Skip 1 | ForEach-Object {
        '"' + [regex]::Replace([regex]::Replace($_, '(\\*)"', '$1$1\"'), '(\\+)$', '$1$1') + '"'
    }) -join ' ')
    try {
        $setupSeconds = $timer.Elapsed.TotalSeconds
        [void]$process.Start()
        $startedSeconds = $timer.Elapsed.TotalSeconds
        $stdout = $process.StandardOutput.ReadToEndAsync()
        $stderr = $process.StandardError.ReadToEndAsync()
        $remaining = [Math]::Max(0, [Math]::Ceiling(($Timeout - $timer.Elapsed.TotalSeconds) * 1000))
        $completed = $process.WaitForExit([int]$remaining)
        $executionSeconds = $timer.Elapsed.TotalSeconds
        $timedOut = -not $completed -or $executionSeconds -ge $Timeout
        if (-not $completed) {
            try {
                if (-not $process.HasExited) {
                    # Batch/compiler drivers may own children. Terminate the
                    # tree before disposing the redirected output handles.
                    $killer = New-Object Diagnostics.Process
                    try {
                        $killer.StartInfo.FileName = Join-Path $env:SystemRoot 'System32/taskkill.exe'
                        $killer.StartInfo.Arguments = '/PID ' + $process.Id + ' /T /F'
                        $killer.StartInfo.UseShellExecute = $false
                        $killer.StartInfo.CreateNoWindow = $true
                        $killer.StartInfo.RedirectStandardOutput = $true
                        $killer.StartInfo.RedirectStandardError = $true
                        [void]$killer.Start()
                        $killOutput = $killer.StandardOutput.ReadToEndAsync()
                        $killError = $killer.StandardError.ReadToEndAsync()
                        if (-not $killer.WaitForExit(1000)) { $killer.Kill() }
                    } finally { $killer.Dispose() }
                    if (-not $process.HasExited) { $process.Kill() }
                }
            }
            catch { if (-not $process.HasExited) { $process.Kill() } }
        }
        # Cleanup and inherited output handles must never cause an unlimited wait.
        if (-not $process.WaitForExit(1000)) { throw 'Process did not terminate within the cleanup deadline' }
        $drained = [Threading.Tasks.Task]::WaitAll([Threading.Tasks.Task[]]@($stdout, $stderr), 1000)
        $code = if (-not $timedOut) { $process.ExitCode } else { $null }
        # Preserve Windows' unsigned exit status in the existing JSON schema.
        if ($null -ne $code -and $code -lt 0) { $code = [long]$code + 4294967296 }
        $output = ''
        if ($stdout.IsCompleted) { $output += $stdout.Result }
        if ($stderr.IsCompleted) { $output += $stderr.Result }
        [ordered]@{ exit = $code; seconds = $executionSeconds; timeout_seconds = $Timeout;
            setup_seconds = $setupSeconds; startup_seconds = $startedSeconds - $setupSeconds;
            wait_seconds = $executionSeconds - $startedSeconds;
            timed_out = $timedOut; output_complete = $drained;
            cleanup_seconds = $timer.Elapsed.TotalSeconds - $executionSeconds; output = $output }
    } finally { $process.Dispose() }
}
