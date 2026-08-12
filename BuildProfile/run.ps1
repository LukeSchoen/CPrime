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
    }

    foreach ($line in Get-Content -LiteralPath $FilePath -TotalCount 12) {
        if ($line -match '^\s*//\s*(PROFILE_[A-Z_]+)\s*:\s*(.*)$') {
            $meta[$matches[1]] = $matches[2].Trim()
        }
    }

    return $meta
}

function Invoke-CompileSample {
    param(
        [string]$Compiler,
        [string]$SourcePath,
        [string]$OutputPath,
        [string[]]$ExtraArgs
    )

    if (Test-Path -LiteralPath $OutputPath) {
        Remove-Item -Force -LiteralPath $OutputPath
    }

    $args = @($ExtraArgs) + @($SourcePath, "-o", $OutputPath)
    $sw = [System.Diagnostics.Stopwatch]::StartNew()
    $savedEap = $ErrorActionPreference
    $ErrorActionPreference = "Continue"
    try {
        $output = & $Compiler @args 2>&1
        $exitCode = $LASTEXITCODE
    } finally {
        $ErrorActionPreference = $savedEap
        $sw.Stop()
    }

    $size = 0
    if (Test-Path -LiteralPath $OutputPath) {
        $size = (Get-Item -LiteralPath $OutputPath).Length
    }

    return [pscustomobject]@{
        ExitCode = $exitCode
        ElapsedMs = [math]::Round($sw.Elapsed.TotalMilliseconds, 3)
        OutputSizeBytes = $size
        CompilerOutput = (($output | Out-String) -replace "`r`n", "`n").Trim()
    }
}

if ($Iterations -lt 1) { throw "Iterations must be at least 1." }
if ($Warmups -lt 0) { throw "Warmups must be 0 or greater." }

$compiler = Resolve-CompilerPath -ExplicitPath $CompilerPath
if (-not $CasesRoot) { $CasesRoot = Join-Path $scriptDir "cases" }
if (-not $OutDir) { $OutDir = Join-Path $scriptDir "out" }
if (-not $CsvPath) { $CsvPath = Join-Path $OutDir "build-profile.csv" }

if (-not (Test-Path -LiteralPath $CasesRoot)) {
    throw "Cases directory does not exist: $CasesRoot"
}

New-Item -ItemType Directory -Force -Path $OutDir | Out-Null
$workDir = Join-Path ([System.IO.Path]::GetTempPath()) ("cprime-build-profile-" + $PID + "-" + ([guid]::NewGuid().ToString("N")))
New-Item -ItemType Directory -Force -Path $workDir | Out-Null

try {
    $cases = @()
    $cases += Get-ChildItem -LiteralPath $CasesRoot -Filter test_*.c -File -ErrorAction SilentlyContinue | Sort-Object Name
    $cases += Get-ChildItem -LiteralPath $CasesRoot -Filter test_*.cpp -File -ErrorAction SilentlyContinue | Sort-Object Name
    if ($cases.Count -eq 0) { throw "No profile cases found under $CasesRoot" }

    $rows = @()
    $failed = 0

    Write-Host "Using compiler: $compiler"
    Write-Host "Running BuildProfile: $($cases.Count) cases, $Warmups warmups, $Iterations samples"
    Write-Host ""

    foreach ($case in $cases) {
        $meta = Parse-Metadata -FilePath $case.FullName
        $extraArgs = @()
        if ($meta.PROFILE_ARGS) {
            $extraArgs = $meta.PROFILE_ARGS -split '\s+' | Where-Object { $_ }
        }

        $totalRuns = $Warmups + $Iterations
        for ($i = 1; $i -le $totalRuns; $i++) {
            $kind = if ($i -le $Warmups) { "warmup" } else { "sample" }
            $sampleIndex = if ($kind -eq "sample") { $i - $Warmups } else { $i }
            $outExe = Join-Path $workDir ("{0}-{1}-{2}.exe" -f $case.BaseName, $kind, $sampleIndex)
            $result = Invoke-CompileSample -Compiler $compiler -SourcePath $case.FullName -OutputPath $outExe -ExtraArgs $extraArgs
            if ($result.ExitCode -ne 0) { $failed++ }

            $rows += [pscustomobject]@{
                timestamp = (Get-Date).ToString("o")
                profile = "BuildProfile"
                case = $meta.PROFILE_NAME
                file = $case.Name
                kind = $kind
                iteration = $sampleIndex
                elapsed_ms = $result.ElapsedMs
                exit_code = $result.ExitCode
                output_size_bytes = $result.OutputSizeBytes
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
        Write-Host "BuildProfile failed: $failed compiler invocation(s) failed."
        exit 1
    }

    exit 0
} finally {
    if (Test-Path -LiteralPath $workDir) {
        Remove-Item -Recurse -Force -LiteralPath $workDir -ErrorAction SilentlyContinue
    }
}
