# Baseline is read once; only completed cases contribute to live deltas.
function New-GccProgress([string]$Baseline) {
    $before = @{}
    if ($Baseline) {
        foreach ($line in [IO.File]::ReadLines((Join-Path $Baseline 'results.jsonl'))) {
            $row = $line | ConvertFrom-Json
            if ($before.ContainsKey($row.path)) { throw "Duplicate baseline case: $($row.path)" }
            $before[$row.path] = $row.status
        }
    }
    @{ before = $before; completed = 0; compared = 0; gains = 0; regressions = 0;
       compile_seconds = 0.0; run_seconds = 0.0; setup_seconds = 0.0;
       startup_seconds = 0.0; wait_seconds = 0.0; cleanup_seconds = 0.0 }
}
function Add-GccProgress($State, $Row) {
    ++$State.completed
    if ($State.before.ContainsKey($Row.path)) {
        $old = $State.before[$Row.path]
        if ($old -ne 'UNSUPPORTED' -and -not $old.StartsWith('PROBE_')) {
            ++$State.compared
            $passes = @('PASS_COMPILE', 'PASS_ASSEMBLE', 'PASS_LINK', 'PASS_RUN', 'PASS_PREPROCESS')
            if ($old -notin $passes -and $Row.status -in $passes) { ++$State.gains }
            if ($old -in $passes -and $Row.status -notin $passes) { ++$State.regressions }
        }
    }
    foreach ($phase in @('compile', 'run')) {
        $result = $Row[$phase]
        if ($null -eq $result) { continue }
        $State[$phase + '_seconds'] += $result.seconds
        foreach ($part in @('setup', 'startup', 'wait', 'cleanup')) {
            $State[$part + '_seconds'] += $result[($part + '_seconds')]
        }
    }
}
function Write-GccProgress($State, $Counts, [int]$Selected, [double]$Elapsed, [string]$Out, [bool]$Complete) {
    $report = [ordered]@{ complete = $Complete; selected = $Selected; completed = $State.completed;
        remaining = $Selected - $State.completed; elapsed_seconds = $Elapsed;
        compared = $State.compared; gains = $State.gains; regressions = $State.regressions; statuses = $Counts }
    foreach ($key in @('compile_seconds', 'run_seconds', 'setup_seconds', 'startup_seconds', 'wait_seconds', 'cleanup_seconds')) {
        $report[$key] = $State[$key]
    }
    $report.driver_seconds = [Math]::Max(0.0, $Elapsed - $State.compile_seconds - $State.run_seconds - $State.cleanup_seconds)
    $report.note = 'Live deltas cover completed baseline checked cases only. Use compare.ps1 for final coverage. Wait includes child execution and scheduling, not pure compiler CPU time.'
    $target = Join-Path $Out 'progress.json'
    [IO.File]::WriteAllText(($target + '.tmp'), ($report | ConvertTo-Json -Depth 6))
    Move-Item -LiteralPath ($target + '.tmp') -Destination $target -Force
    Write-Host ('{0}/{1}, {2:n1}s, +{3} passes, {4} regressions' -f $State.completed, $Selected, $Elapsed, $State.gains, $State.regressions)
}
