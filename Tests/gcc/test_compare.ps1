$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'assessment.ps1')
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../build'))
$work = Join-Path $root ('compare-fixtures-' + [guid]::NewGuid().ToString('N'))
$before = Join-Path $work 'before'
$after = Join-Path $work 'after'
[void](New-Item -ItemType Directory -Path $before, $after)
function Write-Rows($Directory, $Rows) {
    $Rows | ForEach-Object { $_ | ConvertTo-Json -Compress } | Set-Content -Encoding ASCII -LiteralPath (Join-Path $Directory 'results.jsonl')
}
Write-Rows $before @(
    @{path='a';status='PASS_RUN'}, @{path='b';status='PASS_COMPILE'},
    @{path='c';status='FAIL_COMPILE'}, @{path='d';status='UNSUPPORTED'}
)
Write-Rows $after @(
    @{path='a';status='FAIL_RUN'}, @{path='c';status='PASS_COMPILE'},
    @{path='d';status='PASS_RUN'}, @{path='e';status='PASS_RUN'}
)
$command = @((Join-Path $PSHOME 'powershell.exe'), '-NoProfile', '-ExecutionPolicy', 'Bypass', '-File',
    (Join-Path $PSScriptRoot 'compare.ps1'), '-Baseline', $before, '-Current', $after)
$run = Invoke-GccProcess $command 5
if ($run.exit -ne 1) { throw "Comparison must fail on regression/missing coverage: $($run.output)" }
$report = Get-Content -Raw -LiteralPath (Join-Path $after 'comparison.json') | ConvertFrom-Json
if ($report.denominator -ne 3 -or $report.current_passes -ne 1 -or $report.current_percent -ne 33.33 -or
    $report.regressions.Count -ne 1 -or $report.missing -notcontains 'b' -or
    $report.promoted_to_checked -notcontains 'd' -or $report.added -notcontains 'e') {
    throw 'Comparison changed the denominator or lost coverage changes'
}
Write-Rows $after @(
    @{path='a';status='PASS_RUN'}, @{path='b';status='PASS_COMPILE'}, @{path='c';status='UNSUPPORTED'}
)
$run = Invoke-GccProcess $command 5
if ($run.exit -ne 1) { throw 'Removing a previously failing check must fail the coverage gate' }
Write-Output 'PASS fixed denominator, regressions, missing cases, promotions, and removed checks'
