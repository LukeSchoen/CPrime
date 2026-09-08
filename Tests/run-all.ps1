param(
    [string[]]$Suite = @(),
    [string]$CompilerPath = '',
    [string]$RuntimeRoot = '',
    [switch]$UseSharedBinaries,
    [switch]$List
)
$ErrorActionPreference = 'Stop'
# Only discover language suite roots, not ABI fixtures or benchmark inputs.
$roots = @('c_compat', 'debug', 'payload') + @(
    Get-ChildItem -LiteralPath (Join-Path $PSScriptRoot 'features') -Directory |
        Sort-Object Name | ForEach-Object { 'features/' + $_.Name }
)
$available = @($roots | Where-Object {
    $suiteRoot = Join-Path $PSScriptRoot $_
    @(foreach ($kind in @('pass', 'fail')) {
        $directory = Join-Path $suiteRoot $kind
        if (Test-Path -LiteralPath $directory) {
            Get-ChildItem -LiteralPath $directory -File |
                Where-Object { $_.Name -match '^test_.*\.(c|cpp)$' }
        }
    }).Count -gt 0
})
if (-not $Suite.Count) { $Suite = $available }
foreach ($name in $Suite) {
    if ($name -notin $available) { throw "Unknown language suite: $name" }
}
if ($List) { $Suite; exit 0 }
$failed = @()
foreach ($name in $Suite) {
    & (Join-Path $PSScriptRoot 'run.ps1') `
        -Suite $name -CompilerPath $CompilerPath -RuntimeRoot $RuntimeRoot -UseSharedBinaries:$UseSharedBinaries
    if ($LASTEXITCODE -ne 0) { $failed += $name }
}
Write-Host ('Suite summary: {0} passed, {1} failed' -f ($Suite.Count - $failed.Count), $failed.Count)
if ($failed.Count) { Write-Host ('Failed suites: ' + ($failed -join ', ')); exit 1 }
exit 0
