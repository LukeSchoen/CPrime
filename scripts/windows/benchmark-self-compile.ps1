param(
    [string]$CpcPath = "",
    [string]$RootPath = "",
    [int]$WarmupRuns = 1,
    [int]$MeasuredRuns = 5,
    [double]$MaxMedianSeconds = 1.0,
    [switch]$NoThreshold
)

$ErrorActionPreference = "Stop"

if (-not $RootPath) {
    $RootPath = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot "..\.."))
} else {
    $RootPath = [System.IO.Path]::GetFullPath($RootPath)
}
if (-not $CpcPath) {
    $CpcPath = Join-Path $RootPath "cpc.exe"
}
$CpcPath = [System.IO.Path]::GetFullPath($CpcPath)

if (-not (Test-Path -LiteralPath $CpcPath)) {
    throw "Compiler not found: $CpcPath"
}
if ($WarmupRuns -lt 0 -or $MeasuredRuns -lt 1) {
    throw "WarmupRuns must be non-negative and MeasuredRuns must be positive."
}

$source = Join-Path $RootPath "src\compiler\driver\cprime.c"
$commonArgs = @(
    ("-B" + $RootPath),
    ("-I" + (Join-Path $RootPath "include\runtime")),
    ("-I" + (Join-Path $RootPath "include\cprime")),
    ("-I" + (Join-Path $RootPath "third-party\win32-sdk\include")),
    ("-I" + (Join-Path $RootPath "third-party\win32-sdk\include\winapi")),
    ("-I" + (Join-Path $RootPath "src\compiler\frontend")),
    ("-I" + (Join-Path $RootPath "src\compiler\middleend")),
    ("-I" + (Join-Path $RootPath "src\compiler\backend\x64")),
    ("-I" + $RootPath),
    "-DCPRIME_TARGET_PE",
    "-DCPRIME_TARGET_X86_64"
)

function Get-Median {
    param([double[]]$Values)

    $sorted = @($Values | Sort-Object)
    $middle = [int][Math]::Floor($sorted.Count / 2)
    if (($sorted.Count % 2) -eq 1) {
        return $sorted[$middle]
    }
    return ($sorted[$middle - 1] + $sorted[$middle]) / 2.0
}

function Invoke-BenchmarkPhase {
    param(
        [string]$Name,
        [string[]]$ModeArgs,
        [string]$Extension,
        [int]$RunCount,
        [string]$WorkDir,
        [switch]$Warmup
    )

    $times = @()
    for ($i = 0; $i -lt $RunCount; ++$i) {
        $output = Join-Path $WorkDir ("{0}_{1}{2}" -f $Name, $i, $Extension)
        $log = Join-Path $WorkDir ("{0}_{1}.log" -f $Name, $i)
        $arguments = @($commonArgs) + @($ModeArgs) + @("-o", $output, $source)
        $timer = [System.Diagnostics.Stopwatch]::StartNew()
        & $CpcPath @arguments *> $log
        $exitCode = $LASTEXITCODE
        $timer.Stop()
        if ($exitCode -ne 0 -or -not (Test-Path -LiteralPath $output)) {
            $details = if (Test-Path -LiteralPath $log) {
                (Get-Content -LiteralPath $log -Tail 20) -join [Environment]::NewLine
            } else {
                "No compiler log was produced."
            }
            throw "$Name failed with exit code $exitCode.$([Environment]::NewLine)$details"
        }
        if (-not $Warmup) {
            $times += $timer.Elapsed.TotalSeconds
        }
    }
    return $times
}

$workDir = Join-Path ([System.IO.Path]::GetTempPath()) ("cprime-self-benchmark-" + [Guid]::NewGuid().ToString("N"))
New-Item -ItemType Directory -Path $workDir | Out-Null
try {
    if ($WarmupRuns -gt 0) {
        Invoke-BenchmarkPhase -Name "warmup" -ModeArgs @("-c") -Extension ".obj" `
            -RunCount $WarmupRuns -WorkDir $workDir -Warmup | Out-Null
    }

    $preprocessTimes = @(Invoke-BenchmarkPhase -Name "preprocess" -ModeArgs @("-E") `
        -Extension ".i" -RunCount $MeasuredRuns -WorkDir $workDir)
    $compileTimes = @(Invoke-BenchmarkPhase -Name "compile" -ModeArgs @("-c") `
        -Extension ".obj" -RunCount $MeasuredRuns -WorkDir $workDir)

    $preprocessMedian = Get-Median $preprocessTimes
    $compileMedian = Get-Median $compileTimes
    Write-Host ("CPC: {0}" -f $CpcPath)
    Write-Host ("Preprocess median: {0:N3}s ({1} runs)" -f $preprocessMedian, $MeasuredRuns)
    Write-Host ("Object median:     {0:N3}s ({1} runs)" -f $compileMedian, $MeasuredRuns)

    if (-not $NoThreshold -and $compileMedian -gt $MaxMedianSeconds) {
        Write-Error ("Object median {0:N3}s exceeds the {1:N3}s limit." -f `
            $compileMedian, $MaxMedianSeconds)
        exit 1
    }
} finally {
    Remove-Item -LiteralPath $workDir -Recurse -Force -ErrorAction SilentlyContinue
}
