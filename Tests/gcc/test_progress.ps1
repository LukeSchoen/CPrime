$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'progress.ps1')
$work = Join-Path $PSScriptRoot ('../../build/progress-fixtures-' + [guid]::NewGuid().ToString('N'))
[void][IO.Directory]::CreateDirectory($work)
@('{"path":"a","status":"PASS_RUN"}', '{"path":"b","status":"FAIL_COMPILE"}',
  '{"path":"c","status":"UNSUPPORTED"}') | Set-Content (Join-Path $work 'results.jsonl')
$state = New-GccProgress $work
$timing = @{seconds=1.0; setup_seconds=0.1; startup_seconds=0.2; wait_seconds=0.7; cleanup_seconds=0.05}
Add-GccProgress $state @{path='a';status='FAIL_COMPILE';compile=$timing}
Add-GccProgress $state @{path='b';status='PASS_RUN';compile=$timing;run=$timing}
Add-GccProgress $state @{path='c';status='PASS_COMPILE'}
Write-GccProgress $state @{FAIL_COMPILE=1;PASS_RUN=1;PASS_COMPILE=1} 4 4.0 $work $false
$report = Get-Content -Raw (Join-Path $work 'progress.json') | ConvertFrom-Json
if ($report.completed -ne 3 -or $report.remaining -ne 1 -or $report.compared -ne 2 -or
    $report.gains -ne 1 -or $report.regressions -ne 1 -or $report.complete -or
    [Math]::Abs($report.driver_seconds - 0.85) -gt 0.00001 -or $report.compile_seconds -ne 2) {
    throw 'Incremental comparison or phase accounting failed'
}
Add-GccProgress $state @{path='added';status='PASS_RUN'}
Write-GccProgress $state @{} 4 5.0 $work $true
$report = Get-Content -Raw (Join-Path $work 'progress.json') | ConvertFrom-Json
if (-not $report.complete -or $report.remaining -ne 0 -or $report.compared -ne 2) {
    throw 'Final snapshot or added-case accounting failed'
}
Add-Content (Join-Path $work 'results.jsonl') '{"path":"a","status":"PASS_RUN"}'
$rejected = $false
try { $null = New-GccProgress $work } catch { $rejected = $true }
if (-not $rejected) { throw 'Duplicate baseline accepted' }
Write-Output 'PASS incremental deltas, partial coverage, timing accounting, duplicate rejection'
