$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'assessment.ps1')
function Assert-Equal($Actual, $Expected, [string]$Name) {
    $a = ConvertTo-Json -InputObject $Actual -Depth 10 -Compress
    $b = ConvertTo-Json -InputObject $Expected -Depth 10 -Compress
    if ($a -cne $b) { throw "$Name expected $b, got $a" }
}
$assessment = Get-GccAssessment '// { dg-do run } { dg-options "-O2" }'
Assert-Equal $assessment.Action 'run' 'runtime action'
Assert-Equal $assessment.Options @('-O2') 'runtime options'
Assert-Equal $assessment.Reasons @() 'runtime reasons'
Assert-Equal $assessment.Negative $false 'runtime negativity'
$source = '// { dg-do run { target { ! c++98_only } } }'
$directives = @(Get-GccDirectives $source)
Assert-Equal $directives.Count 1 'nested count'
Assert-Equal $directives[0].Name 'dg-do' 'nested name'
Assert-Equal $directives[0].Arguments 'run { target { ! c++98_only } }' 'nested braces'
if (-not (Get-GccAssessment $source).Reasons.Count) { throw 'Ignored target selector' }
$assessment = Get-GccAssessment '// { dg-error "expected \"thing\" [}]" }'
if (-not $assessment.Negative -or -not $assessment.Reasons.Count) { throw 'Ignored diagnostic expectation' }
$assessment = Get-GccAssessment '// { dg-options "-std=c++20 -O2 -fconcepts" }'
Assert-Equal $assessment.Options @('-O2') 'supported options'
foreach ($reason in @('unverified option: -std=c++20', 'unverified option: -fconcepts')) {
    if ($assessment.Reasons -cnotcontains $reason) { throw "Missing reason: $reason" }
}
Assert-Equal (Get-GccAssessment '// { dg-options "-O0" } { dg-additional-options "-g" }').Options @('-O0', '-g') 'additional options'
if (-not (Get-GccAssessment '// { dg-final { scan-assembler "foo" } }').Reasons.Count) { throw 'Ignored assembly expectation' }
Assert-Equal @(Get-GccDirectives '// { dg-do run')[0].Name 'unclosed-directive' 'malformed directive'
Write-Output 'PASS all seven original GCC classifier cases'

Assert-Equal (Get-GccGateExitCode @{}) 1 'empty selection fails gate'
Assert-Equal (Get-GccGateExitCode @{ UNSUPPORTED = 10 }) 1 'unsupported-only selection fails gate'
Assert-Equal (Get-GccGateExitCode @{ PROBE_ACCEPTED = 10; PROBE_REJECTED = 3 }) 1 'probe-only selection fails gate'
Assert-Equal (Get-GccGateExitCode @{ PASS_RUN = 0 }) 1 'zero pass count fails gate'
Assert-Equal (Get-GccGateExitCode @{ PASS_RUN = 1; UNSUPPORTED = 10 }) 0 'restricted checked gate'
foreach ($failure in @('FAIL_COMPILE', 'FAIL_RUN', 'FAIL_NO_OUTPUT', 'CRASH', 'TIMEOUT')) {
    $counts = @{ PASS_COMPILE = 1 }; $counts[$failure] = 1
    Assert-Equal (Get-GccGateExitCode $counts) 1 "failure cannot be masked: $failure"
}
Write-Output 'PASS nonempty verified gate cases'

# Exercise process I/O, Windows quoting, exit codes and timeout cleanup without
# adding a compiler dependency to the assessment unit tests.
$temporary = [IO.Path]::GetFullPath([IO.Path]::GetTempPath())
$work = Join-Path $temporary ('cprime gcc runner ' + [guid]::NewGuid().ToString('N'))
try {
    [void][IO.Directory]::CreateDirectory($work)
    $script = Join-Path $work 'probe.ps1'
    [IO.File]::WriteAllText($script, @'
param([string]$Mode, [string]$Value)
if ($Mode -eq 'tree') {
    $child = Start-Process -FilePath (Join-Path $PSHOME 'powershell.exe') -ArgumentList '-NoProfile -Command Start-Sleep -Seconds 30' -WindowStyle Hidden -PassThru
    [IO.File]::WriteAllText($Value, [string]$child.Id)
    Start-Sleep -Seconds 30
    exit 0
}
if ($Mode -eq 'timeout') { Start-Sleep -Seconds 30; exit 0 }
if ($Mode -eq 'io') { [Console]::Out.Write(('x' * 100000)); [Console]::Error.Write(('y' * 100000)); exit 7 }
[Console]::Out.Write($Value)
'@)
    $shell = Join-Path $PSHOME 'powershell.exe'
    $prefix = @($shell, '-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', $script)
    foreach ($value in @('space in argument', 'embedded"quote', 'trailing slash\', 'both "quote" and slash\')) {
        $result = Invoke-GccProcess ($prefix + @('echo', $value)) 5
        Assert-Equal $result.exit 0 'quoted process exit'
        Assert-Equal $result.output $value 'quoted argument roundtrip'
    }
    $result = Invoke-GccProcess ($prefix + 'io') 5
    Assert-Equal $result.exit 7 'nonzero process exit'
    Assert-Equal $result.output.Length 200000 'concurrent output draining'
    $result = Invoke-GccProcess ($prefix + 'timeout') 0.2
    Assert-Equal $result.exit $null 'process timeout'
    Assert-Equal $result.timed_out $true 'deadline recorded'
    Assert-Equal $result.timeout_seconds 0.2 'budget recorded'
    $childPidPath = Join-Path $work 'child.pid'
    $treeResult = Invoke-GccProcess ($prefix + @('tree', $childPidPath)) 0.8
    Assert-Equal $treeResult.timed_out $true 'child process tree deadline'
    if (-not (Test-Path -LiteralPath $childPidPath)) { throw 'Child process fixture did not start' }
    $childProcess = Get-Process -Id ([int][IO.File]::ReadAllText($childPidPath)) -ErrorAction SilentlyContinue
    if ($childProcess -and -not $childProcess.HasExited) {
        $childProcess.Kill()
        throw 'Timed-out process left its child running'
    }
    Assert-Equal (Get-GccRunStatus $result) 'FAIL_RUN_TIMEOUT' 'runtime timeout is distinct'
    Assert-Equal (Get-GccRunStatus @{ exit = 3221225477 }) 'FAIL_RUN_CRASH' 'runtime crash is distinct'
    Assert-Equal (Get-GccRunStatus @{ exit = 0 }) 'PASS_RUN' 'successful runtime'
    $rejected = $false
    try { Invoke-GccProcess ($prefix + 'echo') 6 } catch { $rejected = $true }
    Assert-Equal $rejected $true 'cannot raise budget beyond five seconds'
    if ($result.seconds -gt 5) { throw 'Timeout did not terminate promptly' }
    foreach ($case in @(
        @{ Code = $null; Probe = $false; Expected = 'TIMEOUT' },
        @{ Code = -1073741819; Probe = $true; Expected = 'CRASH' },
        @{ Code = 256; Probe = $false; Expected = 'CRASH' },
        @{ Code = 1; Probe = $false; Expected = 'FAIL_COMPILE' },
        @{ Code = 0; Probe = $true; Expected = 'PROBE_ACCEPTED' },
        @{ Code = 1; Probe = $true; Expected = 'PROBE_REJECTED' },
        @{ Code = 0; Probe = $false; Expected = 'FAIL_NO_OUTPUT' }
    )) {
        Assert-Equal (Get-GccCompileStatus @{ exit = $case.Code } $case.Probe 'compile' (Join-Path $work 'missing.o')) $case.Expected 'compile classification'
    }
    Assert-Equal (Get-GccCompileStatus @{ exit = 0 } $false 'preprocess' '') 'PASS_PREPROCESS' 'preprocessing needs no object'
    Write-Output 'PASS process quoting, stdout/stderr capture, timeout and failure classifications'
} finally {
    $resolved = [IO.Path]::GetFullPath($work)
    if (-not $resolved.StartsWith($temporary.TrimEnd('\') + '\', [StringComparison]::OrdinalIgnoreCase)) { throw 'Unexpected fixture cleanup path' }
    if (Test-Path -LiteralPath $resolved) { Remove-Item -LiteralPath $resolved -Recurse }
}
