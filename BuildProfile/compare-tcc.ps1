param(
    [string]$CpcPath = "",
    [string]$TccPath = "",
    [int]$Iterations = 10,
    [int]$Warmups = 2,
    [string]$CasesRoot = "",
    [string]$OutDir = "",
    [string]$CsvPath = "",
    [string]$TccBuildCompiler = "",
    [switch]$BuildTcc
)

$ErrorActionPreference = "Stop"
if (Get-Variable -Name PSNativeCommandUseErrorActionPreference -Scope Global -ErrorAction SilentlyContinue) {
    $global:PSNativeCommandUseErrorActionPreference = $false
}

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$rootDir = Resolve-Path (Join-Path $scriptDir "..")
$vendorTccDir = Join-Path $rootDir "third-party\tcc"
$vendorTccWin32Dir = Join-Path $vendorTccDir "win32"

function Resolve-Executable {
    param(
        [string]$ExplicitPath,
        [string[]]$Candidates,
        [string]$Name
    )

    $all = @()
    if ($ExplicitPath) { $all += $ExplicitPath }
    $all += $Candidates

    foreach ($candidate in $all) {
        if (-not $candidate) { continue }
        $resolved = Resolve-Path -LiteralPath $candidate -ErrorAction SilentlyContinue
        if ($resolved) { return $resolved.Path }
    }

    throw "Unable to find $Name."
}

function Build-UpstreamTcc {
    param([string]$Compiler)

    $buildScript = Join-Path $vendorTccWin32Dir "build-tcc.bat"
    if (-not (Test-Path -LiteralPath $buildScript)) {
        throw "Missing upstream tcc build script: $buildScript"
    }

    $compilerCommand = $Compiler
    $oldPath = $env:PATH
    if ($Compiler) {
        $compilerPath = Resolve-Path -LiteralPath $Compiler -ErrorAction SilentlyContinue
        if ($compilerPath) {
            $compilerExe = Split-Path -Leaf $compilerPath.Path
            $compilerDir = Split-Path -Parent $compilerPath.Path
            $env:PATH = $compilerDir + [System.IO.Path]::PathSeparator + $env:PATH
            $compilerCommand = $compilerExe

            if ([System.IO.Path]::GetFileNameWithoutExtension($compilerExe) -ieq "clang") {
                $compilerCommand += " -fno-builtin -Dopen=_open -Dread=_read -Dclose=_close -Dlseek=_lseek -Dunlink=_unlink -Dfdopen=_fdopen -Dgetcwd=_getcwd -Dstricmp=_stricmp -Dstrlwr=_strlwr"
            }
        }
    }

    Push-Location $vendorTccWin32Dir
    try {
        if ($compilerCommand) {
            & cmd.exe /c build-tcc.bat -c "`"$compilerCommand`""
        } else {
            & cmd.exe /c build-tcc.bat
        }
        if ($LASTEXITCODE -ne 0) {
            throw "Upstream tcc build failed with exit code $LASTEXITCODE."
        }
    } finally {
        Pop-Location
        $env:PATH = $oldPath
    }
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
        [string[]]$BaseArgs,
        [string[]]$ExtraArgs
    )

    if (Test-Path -LiteralPath $OutputPath) {
        Remove-Item -Force -LiteralPath $OutputPath
    }

    $args = @($BaseArgs) + @($ExtraArgs) + @($SourcePath, "-o", $OutputPath)
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

if ($BuildTcc) {
    Build-UpstreamTcc -Compiler $TccBuildCompiler
}

$cpc = Resolve-Executable -ExplicitPath $CpcPath -Name "cpc.exe" -Candidates @(
    (Join-Path $rootDir "cpc.exe"),
    (Join-Path $rootDir "win32\cpc.exe")
)
$tcc = Resolve-Executable -ExplicitPath $TccPath -Name "vendored tcc.exe" -Candidates @(
    (Join-Path $vendorTccWin32Dir "tcc.exe")
)

if (-not $CasesRoot) { $CasesRoot = Join-Path $scriptDir "cases" }
if (-not $OutDir) { $OutDir = Join-Path $scriptDir "out" }
if (-not $CsvPath) { $CsvPath = Join-Path $OutDir "tcc-compare.csv" }

if (-not (Test-Path -LiteralPath $CasesRoot)) {
    throw "Cases directory does not exist: $CasesRoot"
}

New-Item -ItemType Directory -Force -Path $OutDir | Out-Null
$workDir = Join-Path ([System.IO.Path]::GetTempPath()) ("cprime-tcc-compare-" + $PID + "-" + ([guid]::NewGuid().ToString("N")))
New-Item -ItemType Directory -Force -Path $workDir | Out-Null

try {
    $cases = Get-ChildItem -LiteralPath $CasesRoot -Filter test_*.c -File -ErrorAction SilentlyContinue | Sort-Object Name
    if ($cases.Count -eq 0) { throw "No C profile cases found under $CasesRoot" }

    $compilers = @(
        [pscustomobject]@{ Name = "tcc"; Path = $tcc; BaseArgs = @("-B$vendorTccWin32Dir") },
        [pscustomobject]@{ Name = "cpc"; Path = $cpc; BaseArgs = @() }
    )

    $rows = @()
    $failed = 0

    Write-Host "Using upstream tcc: $tcc"
    Write-Host "Using CPC: $cpc"
    Write-Host "Running C-only speed comparison: $($cases.Count) cases, $Warmups warmups, $Iterations samples"
    Write-Host ""

    foreach ($case in $cases) {
        $meta = Parse-Metadata -FilePath $case.FullName
        $extraArgs = @()
        if ($meta.PROFILE_ARGS) {
            $extraArgs = $meta.PROFILE_ARGS -split '\s+' | Where-Object { $_ }
        }

        foreach ($compiler in $compilers) {
            $totalRuns = $Warmups + $Iterations
            for ($i = 1; $i -le $totalRuns; $i++) {
                $kind = if ($i -le $Warmups) { "warmup" } else { "sample" }
                $sampleIndex = if ($kind -eq "sample") { $i - $Warmups } else { $i }
                $outExe = Join-Path $workDir ("{0}-{1}-{2}-{3}.exe" -f $compiler.Name, $case.BaseName, $kind, $sampleIndex)
                $result = Invoke-CompileSample -Compiler $compiler.Path -SourcePath $case.FullName -OutputPath $outExe -BaseArgs $compiler.BaseArgs -ExtraArgs $extraArgs
                if ($result.ExitCode -ne 0) {
                    $failed++
                    Write-Host ("{0}/{1}: failed exit {2} {3}" -f $compiler.Name, $meta.PROFILE_NAME, $result.ExitCode, $result.CompilerOutput)
                }

                $rows += [pscustomobject]@{
                    timestamp = (Get-Date).ToString("o")
                    profile = "BuildProfileTccCompare"
                    compiler = $compiler.Name
                    case = $meta.PROFILE_NAME
                    file = $case.Name
                    kind = $kind
                    iteration = $sampleIndex
                    elapsed_ms = $result.ElapsedMs
                    exit_code = $result.ExitCode
                    output_size_bytes = $result.OutputSizeBytes
                }
            }
        }

        $samples = @($rows | Where-Object { $_.case -eq $meta.PROFILE_NAME -and $_.kind -eq "sample" -and $_.exit_code -eq 0 })
        $tccAvg = ($samples | Where-Object { $_.compiler -eq "tcc" } | Measure-Object -Property elapsed_ms -Average).Average
        $cpcAvg = ($samples | Where-Object { $_.compiler -eq "cpc" } | Measure-Object -Property elapsed_ms -Average).Average
        if ($tccAvg -and $cpcAvg) {
            $speedPercent = 100.0 * $tccAvg / $cpcAvg
            $slowdownPercent = 100.0 * ($cpcAvg / $tccAvg - 1.0)
            Write-Host ("{0}: tcc {1:n3} ms, cpc {2:n3} ms, cpc speed {3:n1}% of tcc, slowdown {4:n1}%" -f $meta.PROFILE_NAME, $tccAvg, $cpcAvg, $speedPercent, $slowdownPercent)
        }
    }

    $rows | Export-Csv -NoTypeInformation -LiteralPath $CsvPath
    Write-Host ""
    Write-Host "Wrote $CsvPath"

    if ($failed -gt 0) {
        Write-Host "C-only speed comparison failed: $failed compiler invocation(s) failed."
        exit 1
    }

    exit 0
} finally {
    if (Test-Path -LiteralPath $workDir) {
        Remove-Item -Recurse -Force -LiteralPath $workDir -ErrorAction SilentlyContinue
    }
}
