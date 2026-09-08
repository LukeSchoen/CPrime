param(
    [switch]$Fetch,
    [Alias('CompilerPath')][string]$Compiler = '',
    [string]$RuntimeRoot = '',
    [string[]]$Select = @(),
    [string]$Baseline = '',
    [ValidateRange(1, 60)][int]$ProgressSeconds = 15,
    [switch]$Probe,
    [ValidateRange(0.001, 5)][double]$Timeout = 5,
    [switch]$ContinueAfterTimeout,
    [ValidateRange(0, 2147483647)][int]$Limit = 0,
    [string]$Out = ''
)
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'assessment.ps1')
. (Join-Path $PSScriptRoot 'provenance.ps1')
. (Join-Path $PSScriptRoot 'progress.ps1')
$elapsed = [Diagnostics.Stopwatch]::StartNew()
$progress = New-GccProgress $Baseline
$lastProgress = 0.0
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$revision = '5f6257c26b814de1a14c71b2d3a49291765b6577'
$upstream = Join-Path $root 'build/gcc-upstream'
$trees = @('g++.dg', 'g++.old-deja', 'c-c++-common')
function Invoke-Git([string[]]$Arguments) {
    $output = & git @Arguments
    if ($LASTEXITCODE -ne 0) { throw "git failed: $($Arguments -join ' ')" }
    $output
}
if ($Fetch) {
    if (-not (Test-Path -LiteralPath $upstream)) {
        Invoke-Git @('clone', '--filter=blob:none', '--no-checkout', 'https://github.com/gcc-mirror/gcc.git', $upstream)
    }
    Invoke-Git (@('-C', $upstream, 'sparse-checkout', 'set') + @($trees | ForEach-Object { 'gcc/testsuite/' + $_ }) + 'gcc/testsuite/lib')
    Invoke-Git @('-C', $upstream, 'fetch', '--depth=1', 'origin', $revision)
    Invoke-Git @('-C', $upstream, 'checkout', '--detach', $revision)
}
if ((Invoke-Git @('-C', $upstream, 'rev-parse', 'HEAD')) -cne $revision) { throw 'Upstream revision differs from pin; run -Fetch' }
if (Invoke-Git @('-C', $upstream, 'status', '--porcelain')) { throw 'Upstream checkout is modified; refusing an ambiguous baseline' }
if (-not $Compiler) { $Compiler = Join-Path $root 'cpc.exe' }
$Compiler = (Resolve-Path -LiteralPath $Compiler).Path
if ($RuntimeRoot) { $RuntimeRoot = (Resolve-Path -LiteralPath $RuntimeRoot).Path }
if (-not $Out) { $Out = Join-Path $root 'build/gcc-results' }
$Out = [IO.Path]::GetFullPath($Out)
[void][IO.Directory]::CreateDirectory($Out)
$testRoot = Join-Path $upstream 'gcc/testsuite'
function Get-Relative([string]$Path) { $Path.Substring($testRoot.Length + 1).Replace('\', '/') }
$exactFiles = @()
foreach ($selection in $Select) {
    $candidate = [IO.Path]::GetFullPath((Join-Path $testRoot $selection))
    if ($candidate.StartsWith($testRoot + '\', [StringComparison]::OrdinalIgnoreCase) -and
        (Test-Path -LiteralPath $candidate -PathType Leaf) -and
        [IO.Path]::GetExtension($candidate) -cin @('.C', '.cc', '.cpp', '.c')) { $exactFiles += $candidate }
}
[string[]]$files = if ($Select.Count -and $exactFiles.Count -eq $Select.Count) {
    @($exactFiles | Select-Object -Unique)
} else { @(foreach ($tree in $trees) {
    Get-ChildItem -LiteralPath (Join-Path $testRoot $tree) -Recurse -File | Where-Object { $_.Extension -cin @('.C', '.cc', '.cpp', '.c') } | ForEach-Object {
        $relative = Get-Relative $_.FullName
        if (-not $Select.Count -or @($Select | Where-Object { $relative.StartsWith($_, [StringComparison]::Ordinal) }).Count) { $_.FullName }
    }
}) }
[Array]::Sort($files, [StringComparer]::Ordinal)
if ($Limit) { $files = @($files | Select-Object -First $Limit) }
$counts = @{}
$slowFailures = New-Object 'Collections.Generic.List[object]'
$utf8 = New-Object Text.UTF8Encoding($false)
$metadata = [ordered]@{ revision = $revision; compiler = $Compiler; compiler_sha256 = (Get-FileHash -LiteralPath $Compiler -Algorithm SHA256).Hash.ToLowerInvariant(); selected = $files.Count; serial = $true; probe = [bool]$Probe; note = 'CPC default language mode; no claim of GCC diagnostic or standard-mode conformance' }
$metadata.timeout_seconds = $Timeout
$metadata.timeout_scope = 'each compiler or test process, including startup; cleanup recorded separately'
$metadata.continue_after_timeout = [bool]$ContinueAfterTimeout
$metadata.concurrency = 1
$metadata.baseline = $Baseline
$metadata.progress_sha256 = (Get-FileHash -LiteralPath (Join-Path $PSScriptRoot 'progress.ps1')).Hash
$metadata.runtime_root = $RuntimeRoot
$metadata.language = 'c++'
$metadata.language_standard = 'compiler default; standard-version matrix not implemented'
$metadata.runtime_identity_status = if ($RuntimeRoot) { 'explicit runtime tree hashed' } else { 'implicit compiler search; unresolved' }
$inputs = [ordered]@{ sources = @(Get-GccInputIdentity $testRoot $files); runtime = @() }
if ($RuntimeRoot) {
    $runtimeFiles = @(foreach ($directory in @('include', 'lib')) {
        Get-ChildItem -LiteralPath (Join-Path $RuntimeRoot $directory) -Recurse -File | ForEach-Object { $_.FullName }
    })
    $inputs.runtime = @(Get-GccInputIdentity $RuntimeRoot $runtimeFiles)
}
[IO.File]::WriteAllText((Join-Path $Out 'inputs.json'), ($inputs | ConvertTo-Json -Depth 10), $utf8)
$metadata.inputs_sha256 = (Get-FileHash -LiteralPath (Join-Path $Out 'inputs.json') -Algorithm SHA256).Hash.ToLowerInvariant()
$metadata.provenance_sha256 = (Get-FileHash -LiteralPath (Join-Path $PSScriptRoot 'provenance.ps1') -Algorithm SHA256).Hash.ToLowerInvariant()
$metadata.source_commit = Invoke-Git @('-C', $root, 'rev-parse', 'HEAD')
$metadata.working_tree = @(Invoke-Git @('-C', $root, 'status', '--porcelain'))
$metadata.runner_sha256 = (Get-FileHash -LiteralPath $PSCommandPath -Algorithm SHA256).Hash
$metadata.assessment_sha256 = (Get-FileHash -LiteralPath (Join-Path $PSScriptRoot 'assessment.ps1') -Algorithm SHA256).Hash
[IO.File]::WriteAllText((Join-Path $Out 'metadata.json'), ($metadata | ConvertTo-Json -Depth 10), $utf8)
$log = New-Object IO.StreamWriter((Join-Path $Out 'results.jsonl'), $false, $utf8)
try {
    $index = 0
    foreach ($path in $files) {
        $assessment = Get-GccAssessment ([IO.File]::ReadAllText($path))
        $relative = Get-Relative $path
        $reasons = @($assessment.Reasons)
        $parent = [IO.Path]::GetDirectoryName($path)
        if ($relative.StartsWith('c-c++-common/') -and (Get-Relative $parent) -cnotin @('c-c++-common', 'c-c++-common/cpp')) {
            $reasons += 'shared source requires its specialized GCC driver'
        }
        while ($parent -ne $testRoot) {
            if ([IO.Path]::GetFileName($parent) -cnotin @('g++.dg', 'g++.old-deja') -and @(Get-ChildItem -LiteralPath $parent -Filter '*.exp' -File).Count) {
                $reasons += 'special upstream driver: ' + (Get-Relative $parent)
            }
            $parent = [IO.Path]::GetDirectoryName($parent)
        }
        $action = $assessment.Action
        $row = [ordered]@{ path = $relative; action = $action; reasons = @($reasons); expects_error = $assessment.Negative }
        if ($reasons.Count -and -not $Probe) { $row.status = 'UNSUPPORTED' }
        else {
            # Keep each case's output independent. Windows scanners can retain
            # a just-exited executable briefly; reusing one path couples the
            # next unrelated case to that transient lock.
            $extension = if ($action -in @('run', 'link') -and -not $reasons.Count) { '.exe' } else { '.o' }
            $artifact = Join-Path $Out ('case-{0:D5}{1}' -f $index, $extension)
            if (Test-Path -LiteralPath $artifact) { Remove-Item -LiteralPath $artifact }
            $runtimeOptions = @()
            if ($RuntimeRoot) { $runtimeOptions = @('-B' + $RuntimeRoot) }
            $command = @($Compiler) + $runtimeOptions + @('-x', 'c++') + $assessment.Options + @($path, '-o', $artifact)
            if ($action -eq 'preprocess' -and -not $reasons.Count) { $command += '-E' }
            elseif ($action -notin @('run', 'link') -or $reasons.Count) { $command += '-c' }
            $row.command = @($command)
            $row.compile = Invoke-GccProcess $command $Timeout
            $row.status = Get-GccCompileStatus $row.compile ([bool]$reasons.Count) $action $artifact
            if ($row.status -eq 'RUN') {
                $row.run = Invoke-GccProcess @($artifact) $Timeout
                $row.status = Get-GccRunStatus $row.run
            }
        }
        $counts[$row.status]++
        $log.WriteLine(($row | ConvertTo-Json -Depth 12 -Compress))
        $log.Flush()
        Add-GccProgress $progress $row
        if ($elapsed.Elapsed.TotalSeconds - $lastProgress -ge $ProgressSeconds) {
            Write-GccProgress $progress $counts $files.Count $elapsed.Elapsed.TotalSeconds $Out $false
            $lastProgress = $elapsed.Elapsed.TotalSeconds
        }
        if ($row.status -in @('TIMEOUT', 'FAIL_RUN_TIMEOUT')) {
            $phase = if ($row.status -eq 'TIMEOUT') { 'compile' } else { 'run' }
            $slowFailures.Add([ordered]@{ path = $relative; phase = $phase; status = $row.status;
                budget_seconds = $Timeout; seconds = $row[$phase].seconds;
                command = $(if ($phase -eq 'compile') { $row.command } else { @($artifact) });
                disposition = 'CPC defect until workload evidence justifies removing this case from the fast suite' })
            Write-Warning "Time budget exceeded: $relative ($phase). Fix correctness/performance; do not increase the budget."
            if (-not $ContinueAfterTimeout) { ++$index; break }
        }
        ++$index
    }
} finally { $log.Dispose() }
Write-GccProgress $progress $counts $files.Count $elapsed.Elapsed.TotalSeconds $Out ($index -eq $files.Count)
[IO.File]::WriteAllText((Join-Path $Out 'slow-failures.json'), (ConvertTo-Json -InputObject @($slowFailures.ToArray()) -Depth 12), $utf8)
[IO.File]::WriteAllText((Join-Path $Out 'execution.json'), (ConvertTo-Json -InputObject ([ordered]@{
    selected = $files.Count; recorded = $index; remaining = $files.Count - $index;
    stopped_early = $index -lt $files.Count; slow_failures = $slowFailures.Count
})), $utf8)
$summary = $counts | ConvertTo-Json
[IO.File]::WriteAllText((Join-Path $Out 'summary.json'), $summary, $utf8)
Write-Output $summary
$gateExit = Get-GccGateExitCode $counts
if ($gateExit -ne 0) { Write-Warning 'Validation failed or no verified checks passed; probes and unsupported entries are not passes.' }
exit $gateExit
