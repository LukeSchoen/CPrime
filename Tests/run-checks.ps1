param(
    [ValidateSet('fast', 'pedantic', 'all')][string]$Tier = 'fast',
    [string[]]$Select = @(),
    [string]$CompilerPath = '',
    [string]$RuntimeRoot = '',
    [string]$Out = '',
    [switch]$List
)
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'tools/process.ps1')
$root = Split-Path -Parent $PSScriptRoot
$catalog = Get-Content (Join-Path $PSScriptRoot 'checks.json') -Raw | ConvertFrom-Json
$checks = @($catalog | Where-Object { $Tier -eq 'all' -or $_.tier -eq $Tier -or ($Select.Count -and -not $PSBoundParameters.ContainsKey('Tier')) })
foreach ($name in $Select) {
    if ($name -notin $checks.name) { throw "Unknown $Tier check: $name" }
}
if ($Select.Count) { $checks = @($checks | Where-Object name -In $Select) }
if (-not $checks.Count) { throw 'No checks selected' }
if ($List) { $checks | Select-Object name, seconds, path; exit 0 }
if (-not $CompilerPath) { $CompilerPath = Join-Path $root 'cpc.exe' }
$CompilerPath = (Resolve-Path -LiteralPath $CompilerPath).Path
if ($RuntimeRoot) { $RuntimeRoot = (Resolve-Path -LiteralPath $RuntimeRoot).Path }
if (-not $Out) { $Out = Join-Path $root ('build/checks-' + $Tier + '-' + [guid]::NewGuid().ToString('N')) }
New-Item -ItemType Directory -Force $Out | Out-Null
$results = @()
foreach ($check in $checks) {
    if ($check.tier -eq 'fast' -and $check.seconds -gt 5) { throw 'Fast gate budget exceeds five seconds' }
    $script = Join-Path $PSScriptRoot $check.path
    $command = @((Join-Path $PSHOME 'powershell.exe'), '-NoProfile', '-ExecutionPolicy', 'Bypass', '-File')
    if ([IO.Path]::GetExtension($script) -eq '.cmd') {
        $command += @((Join-Path $PSScriptRoot 'tools/run-command.ps1'), '-ScriptPath', $script)
    } else { $command += $script }
    if ($check.compiler) { $command += @('-CompilerPath', $CompilerPath) }
    if ($RuntimeRoot -and $check.runtime) { $command += @('-RuntimeRoot', $RuntimeRoot) }
    $result = Invoke-TestProcess $command $check.seconds
    $passed = $result.exit -eq 0 -and -not $result.timed_out -and $result.output_complete
    $result.output | Set-Content (Join-Path $Out ($check.name + '.log'))
    $results += [ordered]@{name=$check.name; passed=$passed; seconds=$result.seconds;
        budget_seconds=$check.seconds; exit=$result.exit; timed_out=$result.timed_out}
    $results | ConvertTo-Json -Depth 4 | Set-Content (Join-Path $Out 'summary.json')
    Write-Host ('{0} {1} ({2:N2}s / {3}s)' -f $(if ($passed) {'PASS'} else {'FAIL'}), $check.name, $result.seconds, $check.seconds)
}
$failed = @($results | Where-Object { -not $_.passed }).Count
Write-Host ("$Tier checks: {0} passed, {1} failed; logs: {2}" -f ($results.Count - $failed), $failed, $Out)
if ($failed) { exit 1 }
exit 0
