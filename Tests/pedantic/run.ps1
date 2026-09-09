param(
    [ValidateSet('all', 'language', 'gcc', 'checks', 'performance')][string]$Group = 'all',
    [string]$CompilerPath = '',
    [string]$RuntimeRoot = '',
    [switch]$List
)
$ErrorActionPreference = 'Stop'
if ($List) { 'language: established passing language tests'; 'gcc: established passing retained GCC tests'; 'checks: packaging, build driver, self-host'; 'performance: stress workloads'; exit 0 }
$failed = @()
if ($Group -in @('all', 'language')) {
    $suites = @(& (Join-Path $PSScriptRoot '../run-all.ps1') -Tier pedantic -List | Where-Object { $_ -ne 'gcc' })
    if ($suites.Count) {
        & (Join-Path $PSScriptRoot '../run-all.ps1') -Tier pedantic -Suite $suites -CompilerPath $CompilerPath -RuntimeRoot $RuntimeRoot
        if ($LASTEXITCODE -ne 0) { $failed += 'language' }
    }
}
if ($Group -in @('all', 'gcc')) {
    & (Join-Path $PSScriptRoot 'gcc/run.ps1') -Tier pedantic -CompilerPath $CompilerPath -RuntimeRoot $RuntimeRoot -ContinueAfterTimeout
    if ($LASTEXITCODE -ne 0) { $failed += 'gcc' }
}
if ($Group -in @('all', 'checks')) {
    & (Join-Path $PSScriptRoot '../run-checks.ps1') -Tier pedantic -CompilerPath $CompilerPath -RuntimeRoot $RuntimeRoot
    if ($LASTEXITCODE -ne 0) { $failed += 'checks' }
}
if ($Group -in @('all', 'performance')) {
    & (Join-Path $PSScriptRoot '../run.ps1') -Suite pedantic/performance -CompilerPath $CompilerPath -RuntimeRoot $RuntimeRoot
    if ($LASTEXITCODE -ne 0) { $failed += 'performance' }
}
if ($failed.Count) { Write-Host ('Failed pedantic groups: ' + ($failed -join ', ')); exit 1 }
exit 0
