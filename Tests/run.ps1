param(
    [string]$Suite = "c_compat",
    [Alias("TccPath")]
    [string]$CompilerPath = "",
    [switch]$UseSharedBinaries,
    [string]$SharedOutDir = "",
    [switch]$RequireSharedHits
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
    if ($ExplicitPath) {
        $candidates += $ExplicitPath
    }

    $candidates += @(
        (Join-Path $rootDir "cpc.exe"),
        (Join-Path $rootDir "win32\\cpc.exe")
    )

    foreach ($candidate in $candidates) {
        if (-not $candidate) { continue }
        $resolved = Resolve-Path -LiteralPath $candidate -ErrorAction SilentlyContinue
        if ($resolved) {
            return $resolved.Path
        }
    }

    throw "Unable to find cpc.exe. Build first or pass -CompilerPath explicitly."
}

function Parse-Metadata {
    param([string]$FilePath)

    $meta = @{
        EXPECT_EXIT = "0"
        EXPECT_STDOUT = ""
        EXPECT_COMPILE_FAIL = "0"
        EXPECT_COMPILE_ARGS = ""
    }

    foreach ($line in Get-Content -LiteralPath $FilePath -TotalCount 12) {
        if ($line -match '^\s*//\s*(EXPECT_[A-Z_]+)\s*:\s*(.*)$') {
            $meta[$matches[1]] = $matches[2].Trim()
        }
    }

    return $meta
}

function Normalize-LineEndings {
    param([string]$Text)
    return (($Text -replace "`r`n", "`n") -replace "`r", "`n")
}

function Invoke-Compiler {
    param(
        [string]$CompilerPath,
        [string]$SourcePath,
        [string]$OutputPath,
        [string[]]$CompilerArgs
    )

    $args = @($CompilerArgs) + @($SourcePath, "-o", $OutputPath)
    $savedEap = $ErrorActionPreference
    $ErrorActionPreference = "Continue"
    try {
        $output = & $CompilerPath @args 2>&1
        $exitCode = $LASTEXITCODE
    } finally {
        $ErrorActionPreference = $savedEap
    }

    return @{
        ExitCode = $exitCode
        Output = (Normalize-LineEndings (($output | Out-String))).Trim()
    }
}

function Resolve-SharedOutDir {
    param([string]$ExplicitPath)

    if ($ExplicitPath) {
        $resolvedExplicit = Resolve-Path -LiteralPath $ExplicitPath -ErrorAction SilentlyContinue
        if ($resolvedExplicit) { return $resolvedExplicit.Path }
        return $null
    }

    $default = Join-Path $scriptDir "batch\out"
    $resolvedDefault = Resolve-Path -LiteralPath $default -ErrorAction SilentlyContinue
    if ($resolvedDefault) { return $resolvedDefault.Path }
    return $null
}

$compiler = Resolve-CompilerPath -ExplicitPath $CompilerPath
$testsRoot = Join-Path $scriptDir $Suite

if (-not (Test-Path -LiteralPath $testsRoot)) {
    throw "Suite directory does not exist: $testsRoot"
}

$passDir = Join-Path $testsRoot "pass"
$failDir = Join-Path $testsRoot "fail"

function Remove-LegacyTempDirs {
    param([string]$Root)

    Get-ChildItem -LiteralPath $Root -Force -Directory -ErrorAction SilentlyContinue |
        Where-Object { $_.Name -like ".tmp-*" } |
        ForEach-Object {
            Remove-Item -Recurse -Force -LiteralPath $_.FullName -ErrorAction SilentlyContinue
        }
}

Remove-LegacyTempDirs -Root $scriptDir

$workDir = Join-Path ([System.IO.Path]::GetTempPath()) ("cprime-language-tests-" + $PID + "-" + ([guid]::NewGuid().ToString("N")))

if (Test-Path -LiteralPath $workDir) {
    Remove-Item -Recurse -Force -LiteralPath $workDir -ErrorAction SilentlyContinue
}
New-Item -ItemType Directory -Force -Path $workDir | Out-Null

try {
$tests = @()
if (Test-Path -LiteralPath $passDir) {
    $tests += Get-ChildItem -LiteralPath $passDir -Filter test_*.c | Sort-Object Name
    $tests += Get-ChildItem -LiteralPath $passDir -Filter test_*.cpp | Sort-Object Name
}
if (Test-Path -LiteralPath $failDir) {
    $tests += Get-ChildItem -LiteralPath $failDir -Filter test_*.c | Sort-Object Name
    $tests += Get-ChildItem -LiteralPath $failDir -Filter test_*.cpp | Sort-Object Name
}

if ($tests.Count -eq 0) {
    throw "No test files found under $testsRoot"
}

$passed = 0
$failed = 0
$sharedHits = 0
$sharedMisses = 0
$resolvedSharedOutDir = $null
if ($UseSharedBinaries) {
    $resolvedSharedOutDir = Resolve-SharedOutDir -ExplicitPath $SharedOutDir
}

Write-Host "Using compiler: $compiler"
Write-Host "Running suite: $Suite"
Write-Host ""

foreach ($test in $tests) {
    $meta = Parse-Metadata -FilePath $test.FullName
    $expectCompileFail = ($meta.EXPECT_COMPILE_FAIL -eq "1")
    $expectExit = [int]$meta.EXPECT_EXIT
    $expectStdout = Normalize-LineEndings $meta.EXPECT_STDOUT
    $compileArgs = @()
    if ($meta.EXPECT_COMPILE_ARGS) {
        $compileArgs = $meta.EXPECT_COMPILE_ARGS -split '\s+' | Where-Object { $_ }
    }
    $supportsShared = $UseSharedBinaries -and -not $expectCompileFail -and $compileArgs.Count -eq 0 -and $resolvedSharedOutDir

    $exeName = [System.IO.Path]::GetFileNameWithoutExtension($test.Name) + ".exe"
    $outExe = Join-Path $workDir $exeName

    if (Test-Path -LiteralPath $outExe) {
        Remove-Item -Force -LiteralPath $outExe
    }

    $compileOutput = ""
    $compileExit = 0
    $exeToRun = $outExe
    $sharedExe = $null

    if ($supportsShared) {
        $sharedExe = Join-Path $resolvedSharedOutDir $exeName
        if (Test-Path -LiteralPath $sharedExe) {
            $sharedHits++
            $exeToRun = $sharedExe
        } else {
            $sharedMisses++
        }
    }

    if (-not $supportsShared -or -not $sharedExe -or -not (Test-Path -LiteralPath $sharedExe)) {
        $compile = Invoke-Compiler -CompilerPath $compiler -SourcePath $test.FullName -OutputPath $outExe -CompilerArgs $compileArgs
        $compileOutput = $compile.Output
        $compileExit = $compile.ExitCode
    }

    $ok = $true
    $reason = ""

    if ($expectCompileFail) {
        if ($compileExit -eq 0) {
            $ok = $false
            $reason = "expected compile failure but compilation succeeded"
        }
    } else {
        if ($compileExit -ne 0) {
            $ok = $false
            $reason = "compile failed (exit $compileExit): $compileOutput"
        } elseif (-not (Test-Path -LiteralPath $exeToRun)) {
            $ok = $false
            $reason = "compile succeeded but output executable missing"
        } else {
            $runOutput = & $exeToRun 2>&1
            $runExit = $LASTEXITCODE
            $normStdout = Normalize-LineEndings ($runOutput -join "`n")

            if ($runExit -ne $expectExit) {
                $ok = $false
                $reason = "expected exit $expectExit, got $runExit"
            } elseif ($expectStdout -and $normStdout -ne $expectStdout) {
                $ok = $false
                $reason = "stdout mismatch. expected '$expectStdout' got '$normStdout'"
            }
        }
    }

    if ($ok) {
        $passed++
        Write-Host ("PASS {0}" -f $test.Name)
    } else {
        $failed++
        Write-Host ("FAIL {0}: {1}" -f $test.Name, $reason)
    }
}

Write-Host ""
Write-Host ("Summary: {0} passed, {1} failed" -f $passed, $failed)
if ($UseSharedBinaries) {
    Write-Host ("Shared binaries: {0} hits, {1} misses" -f $sharedHits, $sharedMisses)
}

if ($failed -gt 0) {
    exit 1
}

if ($UseSharedBinaries -and $RequireSharedHits -and $sharedHits -eq 0) {
    Write-Host "Shared mode failed: no shared binary hits were used."
    exit 1
}

exit 0
} finally {
    if (Test-Path -LiteralPath $workDir) {
        Remove-Item -Recurse -Force -LiteralPath $workDir -ErrorAction SilentlyContinue
    }
    Remove-LegacyTempDirs -Root $scriptDir
}
