[CmdletBinding()]
param(
    [string[]]$Suite = @(),
    [string]$CompilerPath = '',
    [string]$RuntimeRoot = '',
    [ValidateSet('fast', 'pedantic', 'all')][string]$Tier = 'fast',
    [switch]$IncludeChecks,
    [switch]$List
)
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'tools/process.ps1')
$pedantic = Read-TestPedanticPaths $PSScriptRoot
# Only discover language suite roots, not ABI fixtures or benchmark inputs.
$roots = @('c_compat', 'debug', 'payload') + @(
    Get-ChildItem -LiteralPath (Join-Path $PSScriptRoot 'features') -Directory |
        Sort-Object Name | ForEach-Object { 'features/' + $_.Name }
)
$available = @()
$selected = @($roots | Where-Object {
    $suiteRoot = Join-Path $PSScriptRoot $_
    $files = @(foreach ($kind in @('pass', 'fail')) {
        $directory = Join-Path $suiteRoot $kind
        if (Test-Path -LiteralPath $directory) {
            Get-ChildItem -LiteralPath $directory -File |
                Where-Object { $_.Name -match '^test_.*\.(c|cpp)$' }
        }
    })
    if ($files.Count) { $available += $_ }
    @($files | Where-Object {
        $relative = $_.FullName.Substring($PSScriptRoot.Length + 1).Replace('\', '/')
        $Tier -eq 'all' -or $pedantic.ContainsKey($relative) -eq ($Tier -eq 'pedantic')
    }).Count -gt 0
})
if (Test-Path -LiteralPath (Join-Path $PSScriptRoot 'pedantic/gcc/run.ps1')) {
    $available += 'gcc'
    $selected += 'gcc'
}
if (-not $Suite.Count) { $Suite = $selected }
foreach ($name in $Suite) {
    if ($name -notin $available) { throw "Unknown language suite: $name" }
}
$Suite = @($Suite | Where-Object { $_ -in $selected })
if ($List) { $Suite; exit 0 }
$failed = @()
foreach ($name in $Suite) {
    if ($name -eq 'gcc') {
        & (Join-Path $PSScriptRoot 'pedantic/gcc/run.ps1') -Tier $Tier `
            -CompilerPath $CompilerPath -RuntimeRoot $RuntimeRoot -ContinueAfterTimeout
        if ($LASTEXITCODE -ne 0) { $failed += $name }
        continue
    }
    & (Join-Path $PSScriptRoot 'run.ps1') `
        -Suite $name -Tier $Tier -CompilerPath $CompilerPath -RuntimeRoot $RuntimeRoot
    if ($LASTEXITCODE -ne 0) { $failed += $name }
}
Write-Host ('Suite summary: {0} passed, {1} failed' -f ($Suite.Count - $failed.Count), $failed.Count)
if ($IncludeChecks) {
    & (Join-Path $PSScriptRoot 'run-checks.ps1') -Tier $Tier -CompilerPath $CompilerPath -RuntimeRoot $RuntimeRoot
    if ($LASTEXITCODE -ne 0) { $failed += 'fast subsystem checks' }
}
if ($failed.Count) { Write-Host ('Failed suites: ' + ($failed -join ', ')); exit 1 }
exit 0
