param(
    [ValidateSet('Prime', 'Clang')][string]$Toolchain = 'Prime',
    [Alias('CpcPath')][string]$CompilerPath = '',
    [string]$ProjectRoot = 'C:\Luke\Src\OT\cl',
    [string]$ManifestPath = '',
    [string]$OutDir = '',
    [string]$ExePath = '',
    [ValidateRange(1, 64)][int]$Jobs = [Environment]::ProcessorCount,
    [ValidateRange(1, 3600)][int]$CompileTimeoutSeconds = 60,
    [switch]$Unity,
    [switch]$SkipLink
)

$ErrorActionPreference = 'Stop'
$ProjectRoot = [IO.Path]::GetFullPath($ProjectRoot)
if (-not $CompilerPath) {
    $CompilerPath = if ($Toolchain -eq 'Prime') { Join-Path $PSScriptRoot 'cpc.exe' }
        else { Join-Path $ProjectRoot 'CommonLib\Assets\Programs\Clang\clang.exe' }
}
$CompilerPath = [IO.Path]::GetFullPath($CompilerPath)
if (-not $ManifestPath) { $ManifestPath = Join-Path $ProjectRoot 'builds\manifest\Release-x64.json' }
if (-not $OutDir) { $OutDir = Join-Path $ProjectRoot ('builds\' + $Toolchain.ToLowerInvariant()) }
$OutDir = [IO.Path]::GetFullPath($OutDir)
if (-not (Test-Path -LiteralPath $CompilerPath -PathType Leaf)) { throw "Compiler not found: $CompilerPath" }
if (-not (Test-Path -LiteralPath $ManifestPath -PathType Leaf)) {
    throw "CodeClip manifest not found: $ManifestPath. Run CodeClip.exe to regenerate the selected project."
}
$manifest = Get-Content -Raw -LiteralPath $ManifestPath | ConvertFrom-Json
if ($manifest.schemaVersion -ne 1 -or $manifest.platform -ne 'x64') { throw 'Unsupported build manifest schema or platform' }
$applications = @($manifest.projects | Where-Object { $_.kind -eq 'Application' })
if ($applications.Count -ne 1) { throw 'The manifest must select one application project.' }
if (-not $ExePath) { $ExePath = Join-Path $ProjectRoot ('builds\' + $applications[0].targetName + '.exe') }
$ExePath = [IO.Path]::GetFullPath($ExePath)
New-Item -ItemType Directory -Force -Path $OutDir, (Split-Path $ExePath -Parent) | Out-Null
if (-not $SkipLink -and (Test-Path -LiteralPath $ExePath -PathType Leaf)) { Remove-Item -LiteralPath $ExePath -Force }

function Quote-Native([string]$Argument) {
    # CommandLineToArgvW quoting, including spaces, embedded quotes, and a
    # trailing backslash. Quoting is applied to complete arguments.
    return '"' + [regex]::Replace([regex]::Replace($Argument, '(\\*)"', '$1$1\"'), '(\\+)$', '$1$1') + '"'
}

function Start-Compiler([string[]]$Arguments, [string]$Directory) {
    $start = [Diagnostics.ProcessStartInfo]::new()
    $start.FileName = $CompilerPath
    $start.Arguments = ($Arguments | ForEach-Object { Quote-Native $_ }) -join ' '
    $start.WorkingDirectory = $Directory
    $start.UseShellExecute = $false
    $start.CreateNoWindow = $true
    $start.RedirectStandardOutput = $true
    $start.RedirectStandardError = $true
    $process = [Diagnostics.Process]::new()
    $process.StartInfo = $start
    [void]$process.Start()
    # Own the process handle from Start(), even when a small translation unit
    # finishes before a Start-Process cmdlet could return it to its caller.
    return @{
        Process = $process
        Output = $process.StandardOutput.ReadToEndAsync()
        Error = $process.StandardError.ReadToEndAsync()
    }
}

function Get-CompileFlags($Project, $Source) {
    if ($Toolchain -eq 'Clang') {
        if ([IO.Path]::GetExtension($Source.path) -ne '.c') { '-std=c++17' }
        '-fms-extensions'
    }
    foreach ($include in @($Project.includeDirectories) + @($Source.includeDirectories)) { '-I' + $include }
    foreach ($define in @($Project.defines) + @($Source.defines)) { '-D' + $define }
}

$jobsToRun = @()
$objects = @()
$index = 0
foreach ($project in $manifest.projects) {
    # The generated graph is authoritative. No exclusions, renamed sources,
    # application macros, or replacement implementations belong here.
    $groups = @{}
    foreach ($source in $project.sources) {
        if (-not (Test-Path -LiteralPath $source.path -PathType Leaf)) { throw "Selected source is missing: $($source.path)" }
        $flags = @(Get-CompileFlags $project $source)
        $key = if ($Unity -and [IO.Path]::GetExtension($source.path) -in @('.cpp', '.cxx', '.cc')) {
            $flags -join "`n"
        } else { 'single:' + $index }
        if (-not $groups.ContainsKey($key)) { $groups[$key] = [Collections.ArrayList]::new() }
        [void]$groups[$key].Add(@{ Source = $source.path; Flags = $flags; Index = $index++ })
    }
    foreach ($group in $groups.Values) {
        $chunkSize = if ($Unity -and $group.Count -gt 1) { 4 } else { 1 }
        for ($start = 0; $start -lt $group.Count; $start += $chunkSize) {
            $end = [Math]::Min($start + $chunkSize, $group.Count) - 1
            $chunk = @($group[$start..$end])
            $label = 'source_{0:d4}' -f $chunk[0].Index
            $inputSource = $chunk[0].Source
            if ($chunk.Count -gt 1) {
                $inputSource = Join-Path $OutDir ($label + '.cpp')
                $chunk | ForEach-Object { '#include "' + $_.Source.Replace('\', '/') + '"' } |
                    Set-Content -LiteralPath $inputSource -Encoding UTF8
            }
            $object = Join-Path $OutDir ($label + '.obj')
            if (Test-Path -LiteralPath $object) { Remove-Item -LiteralPath $object -Force }
            $objects += $object
            $jobsToRun += @{
                Label = $label; Source = $inputSource; Object = $object
                Flags = $chunk[0].Flags; Inputs = @($chunk | ForEach-Object { $_.Source })
            }
        }
    }
}
if (-not $jobsToRun.Count) { throw 'The manifest contains no compile sources.' }
$jobsToRun | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $OutDir 'compile_inputs.json') -Encoding UTF8
$active = [Collections.ArrayList]::new()
$script:failed = $false
$timer = [Diagnostics.Stopwatch]::StartNew()

function Complete-Compiles([switch]$Drain) {
    do {
        foreach ($job in @($active)) {
            $timedOut = -not $job.Process.HasExited -and $job.Timer.Elapsed.TotalSeconds -ge $CompileTimeoutSeconds
            if (-not $job.Process.HasExited -and -not $timedOut) { continue }
            if ($timedOut) { $job.Process.Kill() }
            $job.Process.WaitForExit()
            $job.Process.Refresh()
            $code = if ($timedOut) { -999 } else { $job.Process.ExitCode }
            $diagnostics = $job.Error.GetAwaiter().GetResult()
            $job.Output.GetAwaiter().GetResult() | Set-Content -LiteralPath (Join-Path $OutDir ($job.Label + '.log'))
            $diagnostics | Set-Content -LiteralPath $job.ErrorLog
            if ($null -eq $diagnostics) { $diagnostics = '' }
            $warnings = [regex]::Matches($diagnostics, 'warning:').Count
            Write-Host ('{0} {1,7:n3}s exit={2} warnings={3} {4}' -f $job.Label, $job.Timer.Elapsed.TotalSeconds, $code, $warnings, $job.Inputs[0])
            if ($code -ne 0 -or -not (Test-Path -LiteralPath $job.Object -PathType Leaf)) {
                $script:failed = $true
                if ($timedOut) { Write-Host "Compile timed out after ${CompileTimeoutSeconds}s" }
                ($diagnostics -split "`n" | Where-Object { $_ -match 'error:' } | Select-Object -First 3) | Write-Host
            }
            $job.Process.Dispose()
            [void]$active.Remove($job)
        }
        if ($active.Count -and ($Drain -or $active.Count -ge $Jobs)) { Start-Sleep -Milliseconds 10 }
    } while ($active.Count -and ($Drain -or $active.Count -ge $Jobs))
}

foreach ($job in $jobsToRun) {
    Complete-Compiles
    $arguments = @($job.Flags) + @('-c', $job.Source, '-o', $job.Object)
    $job.ErrorLog = Join-Path $OutDir ($job.Label + '.err.log')
    $job.Timer = [Diagnostics.Stopwatch]::StartNew()
    $started = Start-Compiler $arguments $ProjectRoot
    $job.Process = $started.Process
    $job.Output = $started.Output
    $job.Error = $started.Error
    [void]$active.Add($job)
}
Complete-Compiles -Drain
Write-Host ('Compile elapsed: {0:n3}s; {1} source files' -f $timer.Elapsed.TotalSeconds, $index)
if ($script:failed) { Write-Host 'Executable: NOT PRODUCED'; exit 1 }
if ($SkipLink) { exit 0 }

$linkArguments = @($objects)
foreach ($directory in @($manifest.projects | ForEach-Object { $_.libraryDirectories } | Select-Object -Unique)) {
    $linkArguments += '-L' + $directory
}
foreach ($library in @($manifest.projects | ForEach-Object { $_.linkLibraries } | Select-Object -Unique)) {
    if ($library -match '[/\\]') { $linkArguments += $library }
    elseif ($Toolchain -eq 'Prime') { $linkArguments += '-l' + [IO.Path]::GetFileNameWithoutExtension($library) }
    else { $linkArguments += '-Xlinker', $library }
}
$linkArguments += '-o', $ExePath
$linkLog = Join-Path $OutDir 'link.log'
$link = Start-Compiler $linkArguments $applications[0].directory
$linkProcess = $link.Process
$linkProcess.WaitForExit()
$link.Output.GetAwaiter().GetResult() | Set-Content -LiteralPath $linkLog
$link.Error.GetAwaiter().GetResult() | Set-Content -LiteralPath (Join-Path $OutDir 'link.err.log')
if ($linkProcess.ExitCode -ne 0 -or -not (Test-Path -LiteralPath $ExePath -PathType Leaf)) {
    Get-Content -LiteralPath $linkLog
    Get-Content -LiteralPath (Join-Path $OutDir 'link.err.log')
    if (Test-Path -LiteralPath $ExePath -PathType Leaf) { Remove-Item -LiteralPath $ExePath -Force }
    Write-Host 'Executable: NOT PRODUCED'
    exit 1
}
Write-Host "Executable: $ExePath"
