param(
    [string]$CpcPath = "",
    [string]$YasmPath = "",
    [int]$Iterations = 20,
    [int]$Warmups = 3,
    [string]$OutDir = "",
    [string]$SamplesCsvPath = "",
    [string]$SummaryCsvPath = ""
)

$ErrorActionPreference = "Stop"
if (Get-Variable -Name PSNativeCommandUseErrorActionPreference -Scope Global -ErrorAction SilentlyContinue) {
    $global:PSNativeCommandUseErrorActionPreference = $false
}

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$rootDir = Resolve-Path (Join-Path $scriptDir "..")

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

function Invoke-TimedCommand {
    param(
        [string]$Exe,
        [string[]]$CommandArgs
    )

    $sw = [System.Diagnostics.Stopwatch]::StartNew()
    $savedEap = $ErrorActionPreference
    $ErrorActionPreference = "Continue"
    try {
        $output = & $Exe @CommandArgs 2>&1
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

function Compare-BinaryStats {
    param(
        [string]$Expected,
        [string]$Actual
    )

    if (-not (Test-Path -LiteralPath $Expected) -or -not (Test-Path -LiteralPath $Actual)) {
        return [pscustomobject]@{
            Comparable = $false
            Exact = $false
            ExpectedBytes = 0
            ActualBytes = 0
            DiffBytes = $null
            LengthDelta = $null
            FirstDiffs = ""
        }
    }

    $a = [System.IO.File]::ReadAllBytes($Expected)
    $b = [System.IO.File]::ReadAllBytes($Actual)
    $min = [Math]::Min($a.Length, $b.Length)
    $diff = [Math]::Abs($a.Length - $b.Length)
    $first = New-Object System.Collections.Generic.List[string]
    for ($i = 0; $i -lt $min; $i++) {
        if ($a[$i] -ne $b[$i]) {
            $diff++
            if ($first.Count -lt 8) {
                $first.Add(("0x{0:x}:0x{1:x2}->0x{2:x2}" -f $i, $a[$i], $b[$i]))
            }
        }
    }

    return [pscustomobject]@{
        Comparable = $true
        Exact = ($diff -eq 0)
        ExpectedBytes = $a.Length
        ActualBytes = $b.Length
        DiffBytes = $diff
        LengthDelta = ($b.Length - $a.Length)
        FirstDiffs = ($first -join "; ")
    }
}

function Write-CaseFile {
    param(
        [string]$Path,
        [string]$Text
    )
    Set-Content -LiteralPath $Path -Value $Text -NoNewline
}

if ($Iterations -lt 1) { throw "Iterations must be at least 1." }
if ($Warmups -lt 0) { throw "Warmups must be 0 or greater." }

$cpc = Resolve-Executable -ExplicitPath $CpcPath -Name "cpc.exe" -Candidates @(
    (Join-Path $rootDir "cpc.exe")
)
$yasm = Resolve-Executable -ExplicitPath $YasmPath -Name "yasm.exe" -Candidates @(
    (Join-Path $rootDir "third-party\yasm\yasm.exe")
)

if (-not $OutDir) { $OutDir = Join-Path $scriptDir "out" }
if (-not $SamplesCsvPath) { $SamplesCsvPath = Join-Path $OutDir "asm-compare-samples.csv" }
if (-not $SummaryCsvPath) { $SummaryCsvPath = Join-Path $OutDir "asm-compare-summary.csv" }

New-Item -ItemType Directory -Force -Path $OutDir | Out-Null
$workDir = Join-Path ([System.IO.Path]::GetTempPath()) ("cprime-asm-compare-" + $PID + "-" + ([guid]::NewGuid().ToString("N")))
New-Item -ItemType Directory -Force -Path $workDir | Out-Null

try {
    $cases = @(
        [pscustomobject]@{
            Name = "basic"
            File = "basic.c"
            Text = @'
int add(int a, int b) { return a + b; }
int main(void) { return add(19, 23) == 42 ? 0 : 1; }
'@
        },
        [pscustomobject]@{
            Name = "globals"
            File = "globals.c"
            Text = @'
int g = 7;
int nums[5] = { 1, 2, 3, 4, 5 };
char msg[] = "abc";
int zeroes[8];
int main(void)
{
  zeroes[3] = nums[0] + nums[4] + msg[1];
  if (zeroes[3] != 104) return 1;
  if (g != 7) return 2;
  return 0;
}
'@
        },
        [pscustomobject]@{
            Name = "function_pointer"
            File = "function_pointer.c"
            Text = @'
int square(int x) { return x * x; }
int cubeish(int x) { return square(x) + x; }
int main(void)
{
  int (*fn)(int) = cubeish;
  int i;
  int sum = 0;
  for (i = 0; i < 6; ++i)
    sum += fn(i);
  return sum == 70 ? 0 : 1;
}
'@
        },
        [pscustomobject]@{
            Name = "struct_return"
            File = "struct_return.c"
            Text = @'
typedef struct { int x; int y; } Point;
Point base = { 11, 31 };
Point make(int n)
{
  Point p;
  p.x = base.x + n;
  p.y = base.y - n;
  return p;
}
int main(void)
{
  Point p = make(3);
  return p.x == 14 && p.y == 28 ? 0 : 1;
}
'@
        },
        [pscustomobject]@{
            Name = "member_initializer"
            File = "member_initializer.cpp"
            Text = @'
int logv;
struct Member
{
  int value;
  Member(int x);
};
Member::Member(int x)
{
  value = x;
  logv = logv * 10 + x;
}
struct Owner
{
  Member first;
  Member second;
  int tail;
  Owner();
};
Owner::Owner() : first(4), second(5), tail(6)
{
  logv = logv * 10 + tail;
}
int main(void)
{
  Owner owner;
  return owner.first.value == 4 && owner.second.value == 5 && owner.tail == 6 && logv == 456 ? 0 : 1;
}
'@
        }
    )

    $sampleRows = @()
    $summaryRows = @()

    Write-Host "Using CPC:  $cpc"
    Write-Host "Using Yasm: $yasm"
    Write-Host "Running asm comparison: $($cases.Count) cases, $Warmups warmups, $Iterations samples"
    Write-Host ""

    foreach ($case in $cases) {
        $caseDir = Join-Path $workDir $case.Name
        New-Item -ItemType Directory -Force -Path $caseDir | Out-Null
        $src = Join-Path $caseDir $case.File
        $asm = Join-Path $caseDir ($case.Name + ".s")
        $directExe = Join-Path $caseDir ($case.Name + "-direct.exe")
        $cpcObj = Join-Path $caseDir ($case.Name + "-cpc.o")
        $yasmObj = Join-Path $caseDir ($case.Name + "-yasm.o")
        $cpcExe = Join-Path $caseDir ($case.Name + "-cpc.exe")
        $yasmExe = Join-Path $caseDir ($case.Name + "-yasm.exe")
        Write-CaseFile -Path $src -Text $case.Text

        $direct = Invoke-TimedCommand -Exe $cpc -CommandArgs @($src, "-o", $directExe)
        $emitAsm = Invoke-TimedCommand -Exe $cpc -CommandArgs @("-Sbytes", $src, "-o", $asm)
        if ($direct.ExitCode -ne 0 -or $emitAsm.ExitCode -ne 0) {
            $summaryRows += [pscustomobject]@{
                case = $case.Name
                status = "setup_failed"
                cpc_asm_avg_ms = $null
                yasm_asm_avg_ms = $null
                cpc_speed_percent_of_yasm = $null
                yasm_valid = $false
                yasm_warning_count = $null
                cpc_exact_direct = $false
                yasm_exact_direct = $false
                cpc_vs_yasm_exact = $false
                cpc_direct_diff_bytes = $null
                yasm_direct_diff_bytes = $null
                cpc_vs_yasm_diff_bytes = $null
                note = ($direct.Output + "`n" + $emitAsm.Output).Trim()
            }
            Write-Host "$($case.Name): setup failed"
            continue
        }

        $totalRuns = $Warmups + $Iterations
        for ($i = 1; $i -le $totalRuns; $i++) {
            $kind = if ($i -le $Warmups) { "warmup" } else { "sample" }
            $sampleIndex = if ($kind -eq "sample") { $i - $Warmups } else { $i }

            $cpcSampleObj = Join-Path $caseDir ("{0}-cpc-{1}-{2}.o" -f $case.Name, $kind, $sampleIndex)
            $cpcResult = Invoke-TimedCommand -Exe $cpc -CommandArgs @("-c", $asm, "-o", $cpcSampleObj)
            $sampleRows += [pscustomobject]@{
                timestamp = (Get-Date).ToString("o")
                case = $case.Name
                assembler = "cpc"
                kind = $kind
                iteration = $sampleIndex
                elapsed_ms = $cpcResult.ElapsedMs
                exit_code = $cpcResult.ExitCode
                output_size_bytes = if (Test-Path -LiteralPath $cpcSampleObj) { (Get-Item -LiteralPath $cpcSampleObj).Length } else { 0 }
                warning_count = ([regex]::Matches($cpcResult.Output, "(?im)\bwarning\b")).Count
                output = $cpcResult.Output
            }

            $yasmSampleObj = Join-Path $caseDir ("{0}-yasm-{1}-{2}.o" -f $case.Name, $kind, $sampleIndex)
            $yasmResult = Invoke-TimedCommand -Exe $yasm -CommandArgs @("-p", "gas", "-f", "elf64", $asm, "-o", $yasmSampleObj)
            $sampleRows += [pscustomobject]@{
                timestamp = (Get-Date).ToString("o")
                case = $case.Name
                assembler = "yasm"
                kind = $kind
                iteration = $sampleIndex
                elapsed_ms = $yasmResult.ElapsedMs
                exit_code = $yasmResult.ExitCode
                output_size_bytes = if (Test-Path -LiteralPath $yasmSampleObj) { (Get-Item -LiteralPath $yasmSampleObj).Length } else { 0 }
                warning_count = ([regex]::Matches($yasmResult.Output, "(?im)\bwarning\b")).Count
                output = $yasmResult.Output
            }
        }

        $cpcAssemble = Invoke-TimedCommand -Exe $cpc -CommandArgs @("-c", $asm, "-o", $cpcObj)
        $yasmAssemble = Invoke-TimedCommand -Exe $yasm -CommandArgs @("-p", "gas", "-f", "elf64", $asm, "-o", $yasmObj)
        $cpcLink = Invoke-TimedCommand -Exe $cpc -CommandArgs @($cpcObj, "-o", $cpcExe)
        $yasmLink = Invoke-TimedCommand -Exe $cpc -CommandArgs @($yasmObj, "-o", $yasmExe)

        $cpcVsDirect = Compare-BinaryStats -Expected $directExe -Actual $cpcExe
        $yasmVsDirect = Compare-BinaryStats -Expected $directExe -Actual $yasmExe
        $cpcVsYasm = Compare-BinaryStats -Expected $cpcExe -Actual $yasmExe

        $samples = @($sampleRows | Where-Object { $_.case -eq $case.Name -and $_.kind -eq "sample" -and $_.exit_code -eq 0 })
        $cpcAvg = ($samples | Where-Object { $_.assembler -eq "cpc" } | Measure-Object -Property elapsed_ms -Average).Average
        $yasmAvg = ($samples | Where-Object { $_.assembler -eq "yasm" } | Measure-Object -Property elapsed_ms -Average).Average
        $yasmWarnings = (@($sampleRows | Where-Object { $_.case -eq $case.Name -and $_.assembler -eq "yasm" } | Select-Object -First 1).warning_count)
        $yasmValid = $yasmAssemble.ExitCode -eq 0 -and $yasmAssemble.Output -notmatch "directive .* not recognized|error:"

        $summaryRows += [pscustomobject]@{
            case = $case.Name
            status = "ok"
            cpc_asm_avg_ms = [math]::Round($cpcAvg, 3)
            yasm_asm_avg_ms = [math]::Round($yasmAvg, 3)
            cpc_speed_percent_of_yasm = if ($cpcAvg -and $yasmAvg) { [math]::Round(100.0 * $yasmAvg / $cpcAvg, 1) } else { $null }
            yasm_valid = $yasmValid
            yasm_warning_count = $yasmWarnings
            cpc_exact_direct = $cpcVsDirect.Exact
            yasm_exact_direct = $yasmVsDirect.Exact
            cpc_vs_yasm_exact = $cpcVsYasm.Exact
            cpc_direct_diff_bytes = $cpcVsDirect.DiffBytes
            yasm_direct_diff_bytes = $yasmVsDirect.DiffBytes
            cpc_vs_yasm_diff_bytes = $cpcVsYasm.DiffBytes
            cpc_first_diffs = $cpcVsDirect.FirstDiffs
            yasm_first_diffs = $yasmVsDirect.FirstDiffs
            cpc_vs_yasm_first_diffs = $cpcVsYasm.FirstDiffs
            cpc_link_exit = $cpcLink.ExitCode
            yasm_link_exit = $yasmLink.ExitCode
            yasm_note = $yasmAssemble.Output
        }

        $speed = if ($cpcAvg -and $yasmAvg) { 100.0 * $yasmAvg / $cpcAvg } else { 0 }
        Write-Host ("{0}: cpc {1:n3} ms, yasm {2:n3} ms, cpc speed {3:n1}% of yasm, cpc exact {4}, yasm exact {5}, yasm valid {6}" -f `
            $case.Name, $cpcAvg, $yasmAvg, $speed, $cpcVsDirect.Exact, $yasmVsDirect.Exact, $yasmValid)
    }

    $sampleRows | Export-Csv -NoTypeInformation -LiteralPath $SamplesCsvPath
    $summaryRows | Export-Csv -NoTypeInformation -LiteralPath $SummaryCsvPath

    Write-Host ""
    Write-Host "Wrote $SamplesCsvPath"
    Write-Host "Wrote $SummaryCsvPath"

    $failedCpcExact = @($summaryRows | Where-Object { $_.status -eq "ok" -and -not $_.cpc_exact_direct }).Count
    if ($failedCpcExact) {
        Write-Host "CPC assembly path byte-exact failures: $failedCpcExact"
        exit 1
    }
    exit 0
} finally {
    if (Test-Path -LiteralPath $workDir) {
        Remove-Item -Recurse -Force -LiteralPath $workDir -ErrorAction SilentlyContinue
    }
}
