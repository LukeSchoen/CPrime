param(
    [Alias("TccPath")]
    [string]$CompilerPath = "",
    [int]$Iterations = 5,
    [int]$Warmups = 1,
    [string]$CasesRoot = "",
    [string]$OutDir = "",
    [string]$CsvPath = ""
)

$ErrorActionPreference = "Stop"
if (Get-Variable -Name PSNativeCommandUseErrorActionPreference -Scope Global -ErrorAction SilentlyContinue) {
    $global:PSNativeCommandUseErrorActionPreference = $false
}

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$rootDir = Resolve-Path (Join-Path $scriptDir "..")

function Resolve-CompilerPath {
    param([string]$ExplicitPath)

    $candidates = @()
    if ($ExplicitPath) { $candidates += $ExplicitPath }
    $candidates += @(
        (Join-Path $rootDir "cpc.exe"),
        (Join-Path $rootDir "win32\cpc.exe")
    )

    foreach ($candidate in $candidates) {
        if (-not $candidate) { continue }
        $resolved = Resolve-Path -LiteralPath $candidate -ErrorAction SilentlyContinue
        if ($resolved) { return $resolved.Path }
    }

    throw "Unable to find cpc.exe. Build first or pass -CompilerPath explicitly."
}

function Parse-Metadata {
    param([string]$FilePath)

    $meta = @{
        PROFILE_NAME = [System.IO.Path]::GetFileNameWithoutExtension($FilePath)
        PROFILE_ARGS = ""
        EXPECT_EXIT = "0"
    }

    foreach ($line in Get-Content -LiteralPath $FilePath -TotalCount 12) {
        if ($line -match '^\s*//\s*((PROFILE_[A-Z_]+)|EXPECT_EXIT)\s*:\s*(.*)$') {
            $meta[$matches[1]] = $matches[3].Trim()
        }
    }

    return $meta
}

function Invoke-Compiler {
    param(
        [string]$Compiler,
        [string]$SourcePath,
        [string]$OutputPath,
        [string[]]$ExtraArgs
    )

    $args = @($ExtraArgs) + @($SourcePath, "-o", $OutputPath)
    $savedEap = $ErrorActionPreference
    $ErrorActionPreference = "Continue"
    try {
        $output = & $Compiler @args 2>&1
        $exitCode = $LASTEXITCODE
    } finally {
        $ErrorActionPreference = $savedEap
    }

    return [pscustomobject]@{
        ExitCode = $exitCode
        Output = (($output | Out-String) -replace "`r`n", "`n").Trim()
    }
}

function Invoke-RunSample {
    param([string]$ExePath)

    $sw = [System.Diagnostics.Stopwatch]::StartNew()
    $savedEap = $ErrorActionPreference
    $ErrorActionPreference = "Continue"
    try {
        $output = & $ExePath 2>&1
        $exitCode = $LASTEXITCODE
    } finally {
        $ErrorActionPreference = $savedEap
        $sw.Stop()
    }

    return [pscustomobject]@{
        ExitCode = $exitCode
        ElapsedMs = [math]::Round($sw.Elapsed.TotalMilliseconds, 3)
        Output = (($output | Out-String) -replace "`r`n", "`n").Trim()
    }
}

if ($Iterations -lt 1) { throw "Iterations must be at least 1." }
if ($Warmups -lt 0) { throw "Warmups must be 0 or greater." }

$compiler = Resolve-CompilerPath -ExplicitPath $CompilerPath
if (-not $CasesRoot) { $CasesRoot = Join-Path $rootDir "Tests\benchmarks\runtime" }
if (-not $OutDir) { $OutDir = Join-Path $rootDir "build\profiles\runtime" }
if (-not $CsvPath) { $CsvPath = Join-Path $OutDir "code-profile.csv" }

if (-not (Test-Path -LiteralPath $CasesRoot)) {
    throw "Cases directory does not exist: $CasesRoot"
}

New-Item -ItemType Directory -Force -Path $OutDir | Out-Null
$workDir = Join-Path ([System.IO.Path]::GetTempPath()) ("cprime-code-profile-" + $PID + "-" + ([guid]::NewGuid().ToString("N")))
New-Item -ItemType Directory -Force -Path $workDir | Out-Null

try {
    $cases = @()
    $cases += Get-ChildItem -LiteralPath $CasesRoot -Filter test_*.c -File -ErrorAction SilentlyContinue | Sort-Object Name
    $cases += Get-ChildItem -LiteralPath $CasesRoot -Filter test_*.cpp -File -ErrorAction SilentlyContinue | Sort-Object Name
    if ($cases.Count -eq 0) { throw "No profile cases found under $CasesRoot" }

    $rows = @()
    $failed = 0

    Write-Host "Using compiler: $compiler"
    Write-Host "Running codeProfile: $($cases.Count) cases, $Warmups warmups, $Iterations samples"
    Write-Host ""

    foreach ($case in $cases) {
        $meta = Parse-Metadata -FilePath $case.FullName
        $extraArgs = @()
        if ($meta.PROFILE_ARGS) {
            $extraArgs = $meta.PROFILE_ARGS -split '\s+' | Where-Object { $_ }
        }
        $expectExit = [int]$meta.EXPECT_EXIT
        $outExe = Join-Path $workDir ($case.BaseName + ".exe")

        $compile = Invoke-Compiler -Compiler $compiler -SourcePath $case.FullName -OutputPath $outExe -ExtraArgs $extraArgs
        if ($compile.ExitCode -ne 0 -or -not (Test-Path -LiteralPath $outExe)) {
            Write-Host ("{0}: compile failed (exit {1}) {2}" -f $meta.PROFILE_NAME, $compile.ExitCode, $compile.Output)
            $failed++
            continue
        }

        $totalRuns = $Warmups + $Iterations
        for ($i = 1; $i -le $totalRuns; $i++) {
            $kind = if ($i -le $Warmups) { "warmup" } else { "sample" }
            $sampleIndex = if ($kind -eq "sample") { $i - $Warmups } else { $i }
            $result = Invoke-RunSample -ExePath $outExe
            if ($result.ExitCode -ne $expectExit) { $failed++ }

            $rows += [pscustomobject]@{
                timestamp = (Get-Date).ToString("o")
                profile = "codeProfile"
                case = $meta.PROFILE_NAME
                file = $case.Name
                kind = $kind
                iteration = $sampleIndex
                elapsed_ms = $result.ElapsedMs
                exit_code = $result.ExitCode
                expected_exit = $expectExit
            }
        }

        $samples = @($rows | Where-Object { $_.case -eq $meta.PROFILE_NAME -and $_.kind -eq "sample" })
        $avg = ($samples | Measure-Object -Property elapsed_ms -Average).Average
        Write-Host ("{0}: avg {1:n3} ms over {2} samples" -f $meta.PROFILE_NAME, $avg, $samples.Count)
    }

    $rows | Export-Csv -NoTypeInformation -LiteralPath $CsvPath
    Write-Host ""
    Write-Host "Wrote $CsvPath"

    if ($failed -gt 0) {
        Write-Host "codeProfile failed: $failed compile/run issue(s)."
        exit 1
    }

    exit 0
} finally {
    if (Test-Path -LiteralPath $workDir) {
        Remove-Item -Recurse -Force -LiteralPath $workDir -ErrorAction SilentlyContinue
    }
}
