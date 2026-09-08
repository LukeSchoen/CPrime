param(
    [Parameter(Mandatory = $true)][string]$Baseline,
    [Parameter(Mandatory = $true)][string]$Current,
    [string]$Out = ''
)
$ErrorActionPreference = 'Stop'
function Read-Rows([string]$Directory) {
    $rows = @{}
    foreach ($line in [IO.File]::ReadLines((Join-Path $Directory 'results.jsonl'))) {
        $row = $line | ConvertFrom-Json
        if ($rows.ContainsKey($row.path)) { throw "Duplicate case: $($row.path)" }
        $rows[$row.path] = $row
    }
    return $rows
}
function Test-Pass([string]$Status) {
    return $Status -in @('PASS_COMPILE', 'PASS_ASSEMBLE', 'PASS_LINK', 'PASS_RUN', 'PASS_PREPROCESS')
}
$before = Read-Rows $Baseline
$after = Read-Rows $Current
$newPasses = @(); $regressions = @(); $missing = @(); $added = @(); $crashes = @(); $timeouts = @()
$promoted = @(); $unverified = @()
$baselinePasses = 0; $currentPasses = 0; $checked = 0
foreach ($path in @($before.Keys | Sort-Object)) {
    $old = $before[$path]
    if ($old.status -eq 'UNSUPPORTED' -or $old.status.StartsWith('PROBE_')) { continue }
    ++$checked
    if (Test-Pass $old.status) { ++$baselinePasses }
    if (-not $after.ContainsKey($path)) { $missing += $path; continue }
    $new = $after[$path]
    if ($new.status -eq 'UNSUPPORTED' -or $new.status.StartsWith('PROBE_')) { $unverified += $path }
    if (Test-Pass $new.status) {
        ++$currentPasses
        if (-not (Test-Pass $old.status)) { $newPasses += $path }
    } elseif (Test-Pass $old.status) {
        $regressions += [ordered]@{ path = $path; before = $old.status; after = $new.status }
    }
    if ($new.status -in @('CRASH', 'FAIL_RUN_CRASH')) { $crashes += $path }
    if ($new.status -in @('TIMEOUT', 'FAIL_RUN_TIMEOUT')) { $timeouts += $path }
}
foreach ($path in @($after.Keys | Sort-Object)) {
    if (-not $before.ContainsKey($path)) { $added += $path }
    elseif (($before[$path].status -eq 'UNSUPPORTED' -or $before[$path].status.StartsWith('PROBE_')) -and
            $after[$path].status -ne 'UNSUPPORTED' -and -not $after[$path].status.StartsWith('PROBE_')) {
        $promoted += $path
    }
}
if (-not $checked) { throw 'Baseline contains no checked cases' }
$report = [ordered]@{
    baseline = [IO.Path]::GetFullPath($Baseline); current = [IO.Path]::GetFullPath($Current);
    denominator = $checked; baseline_passes = $baselinePasses; current_passes = $currentPasses;
    baseline_percent = [Math]::Round(100.0 * $baselinePasses / $checked, 2);
    current_percent = [Math]::Round(100.0 * $currentPasses / $checked, 2);
    new_passes = $newPasses; regressions = $regressions; missing = $missing; added = $added;
    crashes = $crashes; timeouts = $timeouts;
    promoted_to_checked = $promoted; no_longer_verified = $unverified;
    note = 'Fixed baseline checked denominator; missing cases remain non-passes. This is restricted adapter progress, not full compatibility.'
}
if (-not $Out) { $Out = Join-Path $Current 'comparison.json' }
[IO.File]::WriteAllText([IO.Path]::GetFullPath($Out), ($report | ConvertTo-Json -Depth 10))
Write-Output ("{0}/{1} ({2}%), {3} new passes, {4} regressions, {5} missing" -f $currentPasses, $checked,
    $report.current_percent, $newPasses.Count, $regressions.Count, $missing.Count)
if ($regressions.Count -or $missing.Count -or $unverified.Count) { exit 1 }
