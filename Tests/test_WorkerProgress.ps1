param(
    [string]$CompilerPath = '',
    [string]$RuntimeRoot = ''
)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
if (-not $CompilerPath) { $CompilerPath = Join-Path $root 'cpc.exe' }
$work = Join-Path $root ('build/worker-progress-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Force $work | Out-Null
$exe = Join-Path $work 'worker-status.exe'
$source = Join-Path $root 'src/tools/worker_status.c'
& $CompilerPath -o $exe $source
if ($LASTEXITCODE -ne 0) { throw "worker-status did not build: $source" }

function Invoke-Status([string[]]$Arguments) {
    # Merge stderr into the captured text without letting a native failure
    # terminate the check; the exit code is asserted by the caller.
    $previous = $ErrorActionPreference
    $ErrorActionPreference = 'SilentlyContinue'
    try { $output = & $exe @Arguments 2>&1 | Out-String } finally { $ErrorActionPreference = $previous }
    [ordered]@{ exit = $LASTEXITCODE; text = $output }
}

# Synthetic corpus: three checked cases plus a support header, two outstanding
# first-party cases, and a log that spans a day so every window is meaningful.
$corpus = Join-Path $work 'corpus.json'
@'
{
  "revision": "fixture",
  "cases": [
    {"path": "g++/one.C", "role": "case", "sha256": "1"},
    {"path": "g++/two.C", "role": "case", "sha256": "2"},
    {"path": "g++/three.C", "role": "case", "sha256": "3"}
  ],
  "support": [
    {"path": "g++/support.h", "role": "support", "sha256": "4"}
  ]
}
'@ | Set-Content $corpus
$firstParty = Join-Path $work 'first-party-failures.txt'
@'
# fixture
features/A/pass/test_a.cpp
features/B/pass/test_b.cpp
'@ | Set-Content $firstParty
$t0 = [long]1789000000
$log = Join-Path $work 'log.tsv'
$rows = @(
    "# cprime worker progress log v1",
    "# utc`tepoch`tcycle`tevent`tgcc_left`tfp_left`ttotal_left`thead"
)
foreach ($row in @(@(0, 'aaa1111', 6), @(12, 'bbb2222', 5))) {
    $epoch = $t0 + $row[0] * 3600
    $utc = [DateTimeOffset]::FromUnixTimeSeconds($epoch).UtcDateTime.ToString('yyyy-MM-ddTHH:mm:ssZ')
    $rows += ("$utc`t$epoch`t$($row[0])`tcycle`t$($row[2])`t2`t$($row[2] + 2)`t$($row[1])")
}
[IO.File]::WriteAllLines($log, $rows)
$now = $t0 + 24 * 3600
$common = @('--root', $root, '--log', $log, '--corpus', $corpus, '--first-party', $firstParty, '--now', $now)

$report = Invoke-Status ($common + @('--head', 'ccc3333'))
if ($report.exit -ne 0) { throw "report failed: $($report.text)" }
$expected = @(
    'left     5 of 8 cases (GCC 3 + first-party 2); completed 3 (37.5%)',
    '24h 3.00/day | 6h 4.00/day over 12.00 hours',
    'trend    improving (6h 1.33x overall)'
)
foreach ($line in $expected) {
    if ($report.text -notmatch [regex]::Escape($line)) { throw "missing `"$line`" in:`n$($report.text)" }
}
if ((Invoke-Status (@('--root', $root, '--log', $log, '--last-cycle'))).text.Trim() -ne '12') {
    throw 'last cycle was not read back from the log'
}

# A finished cycle must append one machine-readable row and keep the header.
$appended = Invoke-Status ($common + @('--cycle', 13, '--event', 'cycle', '--head', 'ccc3333', '--append'))
if ($appended.exit -ne 0) { throw "append failed: $($appended.text)" }
$tail = [IO.File]::ReadAllLines($log)
$last = $tail[-1].Split("`t")
if ($last.Count -ne 8 -or $last[2] -ne '13' -or $last[4] -ne '3' -or $last[5] -ne '2' -or $last[6] -ne '5') {
    throw "appended row is not machine readable: $($tail[-1])"
}
if ((Invoke-Status (@('--root', $root, '--log', $log, '--last-cycle'))).text.Trim() -ne '13') {
    throw 'appended row did not advance the cycle counter'
}

# A completed target reports zero left and asks the worker to stop.
$empty = Join-Path $work 'empty.json'
'{"cases": [], "support": []}' | Set-Content $empty
$emptyList = Join-Path $work 'empty.txt'
'# none' | Set-Content $emptyList
$done = Invoke-Status (@('--root', $root, '--log', $log, '--corpus', $empty, '--first-party', $emptyList, '--now', $now))
if ($done.exit -ne 10 -or $done.text -notmatch 'TARGET REACHED') {
    throw "completed target did not exit 10:`n$($done.text)"
}

# Broken inputs must fail loudly instead of reporting a false zero.
$broken = Invoke-Status (@('--root', $root, '--log', $log, '--corpus', (Join-Path $work 'absent.json'),
    '--first-party', $firstParty, '--now', $now))
if ($broken.exit -ne 2) { throw 'missing corpus manifest did not fail with exit 2' }

# The real log stays machine readable: eight tab separated fields per row.
$real = Join-Path $root 'Tests/progress/log.tsv'
$data = @([IO.File]::ReadAllLines($real) | Where-Object { $_ -and -not $_.StartsWith('#') })
if (-not $data.Count) { throw 'repository progress log has no samples' }
foreach ($line in $data) {
    $fields = $line.Split("`t")
    if ($fields.Count -ne 8) { throw "progress log row is not eight fields: $line" }
    $total = [int]$fields[6]
    if ($total -ne ([int]$fields[4] + [int]$fields[5])) { throw "progress log row is inconsistent: $line" }
    if ([long]$fields[1] -le 0) { throw "progress log row has no epoch: $line" }
}
Write-Host "PASS worker progress: target math, window rates, durable append, failure exits, log format"
