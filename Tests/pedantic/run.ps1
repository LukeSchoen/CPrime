param(
    [ValidateSet('all', 'gcc', 'checks', 'performance')][string]$Group = 'all',
    [string]$CompilerPath = '',
    [string]$RuntimeRoot = '',
    [switch]$List
)
$ErrorActionPreference = 'Stop'
if ($List) { 'gcc: retained upstream failure corpus'; 'checks: packaging, build driver, self-host'; 'performance: stress workloads'; exit 0 }
$failed = @()
if ($Group -in @('all', 'gcc')) {
    & (Join-Path $PSScriptRoot 'gcc/run.ps1') -CompilerPath $CompilerPath -RuntimeRoot $RuntimeRoot -ContinueAfterTimeout
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
