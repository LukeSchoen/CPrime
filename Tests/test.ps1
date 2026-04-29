param(
    [string]$Suite = "c_compat",
    [Alias("TccPath")]
    [string]$CompilerPath = ""
)

$ErrorActionPreference = "Stop"

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
        [string]$TempDir
    )

    $stdoutFile = Join-Path $TempDir "compile_stdout.txt"
    $stderrFile = Join-Path $TempDir "compile_stderr.txt"

    if (Test-Path -LiteralPath $stdoutFile) { Remove-Item -LiteralPath $stdoutFile -Force }
    if (Test-Path -LiteralPath $stderrFile) { Remove-Item -LiteralPath $stderrFile -Force }

    $args = @($SourcePath, "-o", $OutputPath)
    $proc = Start-Process -FilePath $CompilerPath -ArgumentList $args -NoNewWindow -Wait -PassThru `
        -RedirectStandardOutput $stdoutFile -RedirectStandardError $stderrFile

    $stdout = ""
    $stderr = ""
    if (Test-Path -LiteralPath $stdoutFile) { $stdout = Get-Content -Raw -LiteralPath $stdoutFile }
    if (Test-Path -LiteralPath $stderrFile) { $stderr = Get-Content -Raw -LiteralPath $stderrFile }

    return @{
        ExitCode = $proc.ExitCode
        Output = (Normalize-LineEndings ($stdout + $stderr)).Trim()
    }
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

$tests = @()
if (Test-Path -LiteralPath $passDir) {
    $tests += Get-ChildItem -LiteralPath $passDir -Filter test_*.c | Sort-Object Name
}
if (Test-Path -LiteralPath $failDir) {
    $tests += Get-ChildItem -LiteralPath $failDir -Filter test_*.c | Sort-Object Name
}

if ($tests.Count -eq 0) {
    throw "No test files found under $testsRoot"
}

$passed = 0
$failed = 0

Write-Host "Using compiler: $compiler"
Write-Host "Running suite: $Suite"
Write-Host ""

foreach ($test in $tests) {
    $meta = Parse-Metadata -FilePath $test.FullName
    $expectCompileFail = ($meta.EXPECT_COMPILE_FAIL -eq "1")
    $expectExit = [int]$meta.EXPECT_EXIT
    $expectStdout = Normalize-LineEndings $meta.EXPECT_STDOUT

    $exeName = [System.IO.Path]::GetFileNameWithoutExtension($test.Name) + ".exe"
    $outExe = Join-Path $workDir $exeName

    if (Test-Path -LiteralPath $outExe) {
        Remove-Item -Force -LiteralPath $outExe
    }

    $compile = Invoke-Compiler -CompilerPath $compiler -SourcePath $test.FullName -OutputPath $outExe -TempDir $workDir
    $compileOutput = $compile.Output
    $compileExit = $compile.ExitCode

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
        } elseif (-not (Test-Path -LiteralPath $outExe)) {
            $ok = $false
            $reason = "compile succeeded but output executable missing"
        } else {
            $runOutput = & $outExe 2>&1
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

if ($failed -gt 0) {
    exit 1
}

exit 0
