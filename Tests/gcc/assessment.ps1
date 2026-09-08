# Shared assessment logic. Parses GCC directives as data; never executes Tcl.
function Get-GccDirectives([string]$Source) {
    foreach ($match in [regex]::Matches($Source, '\{\s*(dg-[\w-]+)\b')) {
        $depth = 0; $quoted = $false; $escaped = $false; $closed = $false
        for ($end = $match.Index; $end -lt $Source.Length; ++$end) {
            $ch = $Source[$end]
            if ($escaped) { $escaped = $false; continue }
            if ($ch -eq '\') { $escaped = $true; continue }
            if ($ch -eq '"') { $quoted = -not $quoted }
            if (-not $quoted) {
                if ($ch -eq '{') { ++$depth }
                if ($ch -eq '}') { --$depth }
                if ($depth -eq 0) {
                    [pscustomobject]@{ Name = $match.Groups[1].Value; Arguments = $Source.Substring($match.Index + $match.Length, $end - $match.Index - $match.Length).Trim() }
                    $closed = $true
                    break
                }
            }
        }
        if (-not $closed) {
            [pscustomobject]@{ Name = 'unclosed-directive'; Arguments = $Source.Substring($match.Index, [Math]::Min(100, $Source.Length - $match.Index)) }
        }
    }
}

function Get-GccAssessment([string]$Source) {
    $action = 'compile'; $options = @(); $reasons = @(); $negative = $false
    foreach ($directive in @(Get-GccDirectives $Source)) {
        $name = $directive.Name; $arguments = $directive.Arguments
        if ($name -ceq 'dg-do' -and $arguments -cin @('compile', 'assemble', 'link', 'run', 'preprocess')) {
            $action = $arguments
        } elseif ($name -cin @('dg-options', 'dg-additional-options')) {
            $match = [regex]::Match($arguments, '^"([^"\\]*)"$')
            if (-not $match.Success) { $reasons += $name + ': conditional or escaped options'; continue }
            if ($name -ceq 'dg-options') { $options = @() }
            foreach ($option in ($match.Groups[1].Value -split '\s+' | Where-Object { $_ })) {
                if ($option -cin @('-O0', '-O1', '-O2', '-O3', '-g', '-fno-inline', '-fno-builtin')) {
                    $options += $option
                } else { $reasons += 'unverified option: ' + $option }
            }
        } elseif ($name -cin @('dg-error', 'dg-warning', 'dg-message', 'dg-note', 'dg-bogus')) {
            $negative = $negative -or $name -ceq 'dg-error'
            $reasons += 'diagnostic expectations not checked'
        } else { $reasons += $name + ': ' + $arguments }
    }
    [string[]]$unique = @($reasons | Select-Object -Unique)
    [Array]::Sort($unique, [StringComparer]::Ordinal)
    [pscustomobject]@{ Action = $action; Options = @($options); Reasons = @($unique); Negative = $negative }
}

function Invoke-GccProcess([string[]]$Command, [ValidateRange(0.001, 5)][double]$Timeout) {
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
        [void]$process.Start()
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
            timed_out = $timedOut; output_complete = $drained;
            cleanup_seconds = $timer.Elapsed.TotalSeconds - $executionSeconds; output = $output }
    } finally { $process.Dispose() }
}

function Get-GccCompileStatus($Result, [bool]$Unsupported, [string]$Action, [string]$Artifact) {
    if ($null -eq $Result.exit) { return 'TIMEOUT' }
    if ($Result.Contains('output_complete') -and -not $Result.output_complete) { return 'FAIL_OUTPUT_CAPTURE' }
    if ($Result.exit -lt 0 -or $Result.exit -gt 255) { return 'CRASH' }
    if ($Unsupported) { if ($Result.exit -eq 0) { return 'PROBE_ACCEPTED' }; return 'PROBE_REJECTED' }
    if ($Result.exit -ne 0) { return 'FAIL_COMPILE' }
    if ($Action -ne 'preprocess' -and -not (Test-Path -LiteralPath $Artifact -PathType Leaf)) { return 'FAIL_NO_OUTPUT' }
    if ($Action -eq 'run') { return 'RUN' }
    return 'PASS_' + $Action.ToUpperInvariant()
}

function Get-GccRunStatus($Result) {
    if ($null -eq $Result.exit) { return 'FAIL_RUN_TIMEOUT' }
    if ($Result.Contains('output_complete') -and -not $Result.output_complete) { return 'FAIL_OUTPUT_CAPTURE' }
    if ($Result.exit -lt 0 -or $Result.exit -gt 255) { return 'FAIL_RUN_CRASH' }
    if ($Result.exit -ne 0) { return 'FAIL_RUN' }
    return 'PASS_RUN'
}

# Probes and unsupported entries never make a validation gate green.
function Get-GccGateExitCode($Counts) {
    $passes = 0
    foreach ($status in $Counts.Keys) {
        if ($Counts[$status] -le 0) { continue }
        if ($status.StartsWith('FAIL') -or $status -in @('CRASH', 'TIMEOUT')) { return 1 }
        if ($status -in @('PASS_COMPILE', 'PASS_ASSEMBLE', 'PASS_LINK', 'PASS_RUN', 'PASS_PREPROCESS')) {
            $passes += $Counts[$status]
        }
    }
    if ($passes -eq 0) { return 1 }
    return 0
}
