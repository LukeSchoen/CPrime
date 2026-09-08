param(
    [Parameter(Mandatory = $true)][string]$Results,
    [Parameter(Mandatory = $true)][string]$UpstreamPath
)
$ErrorActionPreference = 'Stop'
$upstream = (Resolve-Path -LiteralPath $UpstreamPath).Path
$metadata = Get-Content (Join-Path $Results 'metadata.json') -Raw | ConvertFrom-Json
$revision = $metadata.revision
if ((& git -C $upstream rev-parse HEAD) -ne $revision -or $LASTEXITCODE) { throw 'Upstream revision mismatch' }
if (& git -C $upstream status --porcelain) { throw 'Upstream checkout is modified' }
$destination = Join-Path $PSScriptRoot 'corpus'
if (Test-Path -LiteralPath $destination) { throw 'Corpus already exists; refusing to overwrite it' }
$testRoot = Join-Path $upstream 'gcc/testsuite'
$rows = @(Get-Content (Join-Path $Results 'results.jsonl') | ForEach-Object { $_ | ConvertFrom-Json })
$failures = @($rows | Where-Object { $_.status -like 'FAIL_*' -or $_.status -in @('CRASH', 'TIMEOUT') })
if (-not $failures.Count) { throw 'No failures to retain' }
$known = @{}; $pending = New-Object 'Collections.Generic.Queue[string]'
foreach ($row in $failures) {
    if ($row.reasons.Count) { throw "Unverified case cannot enter checked corpus: $($row.path)" }
    if ($known.ContainsKey($row.path)) { throw "Duplicate case: $($row.path)" }
    $known[$row.path] = 'case'; $pending.Enqueue($row.path)
}

function Export-UpstreamFile([string]$Relative, [string]$Target) {
    # Copy bytes unchanged, including files omitted by the sparse checkout.
    $existing = Join-Path $testRoot $Relative
    if (Test-Path -LiteralPath $existing -PathType Leaf) {
        Copy-Item -LiteralPath $existing -Destination $Target
        return
    }
    $process = New-Object Diagnostics.Process
    $process.StartInfo.FileName = 'git.exe'
    $process.StartInfo.WorkingDirectory = $upstream
    $process.StartInfo.Arguments = 'show ' + $revision + ':gcc/testsuite/' + $Relative
    $process.StartInfo.UseShellExecute = $false
    $process.StartInfo.CreateNoWindow = $true
    $process.StartInfo.RedirectStandardOutput = $true
    if (-not $process.Start()) { throw 'Could not start git' }
    $stream = [IO.File]::Create($Target)
    try { $process.StandardOutput.BaseStream.CopyTo($stream) } finally { $stream.Dispose() }
    $process.WaitForExit()
    $code = $process.ExitCode; $process.Dispose()
    if ($code) { throw "Missing upstream fixture: $Relative" }
}

$entries = @()
while ($pending.Count) {
    $relative = $pending.Dequeue()
    if ($relative -notmatch '^[a-zA-Z0-9_+./-]+$') { throw "Unsafe upstream path: $relative" }
    $target = [IO.Path]::GetFullPath((Join-Path $destination $relative))
    if (-not $target.StartsWith($destination + '\', [StringComparison]::OrdinalIgnoreCase)) { throw 'Path escapes corpus' }
    New-Item -ItemType Directory -Force (Split-Path -Parent $target) | Out-Null
    Export-UpstreamFile $relative $target
    $entries += [ordered]@{path=$relative; role=$known[$relative]; sha256=(Get-FileHash $target).Hash.ToLowerInvariant()}
    # Relative quoted includes are upstream fixtures, including included .C files.
    foreach ($match in [regex]::Matches([IO.File]::ReadAllText($target), '(?m)^\s*#\s*include\s*"([^"\r\n]+)"')) {
        $include = $match.Groups[1].Value
        $dependency = [IO.Path]::GetFullPath((Join-Path (Split-Path -Parent (Join-Path $testRoot $relative)) $include))
        if (-not $dependency.StartsWith($testRoot + '\', [StringComparison]::OrdinalIgnoreCase)) { throw "Fixture outside testsuite: $include" }
        $dependency = $dependency.Substring($testRoot.Length + 1).Replace('\', '/')
        if (-not $known.ContainsKey($dependency)) { $known[$dependency]='support'; $pending.Enqueue($dependency) }
    }
}
foreach ($license in @('COPYING', 'COPYING3', 'COPYING.RUNTIME')) {
    Copy-Item -LiteralPath (Join-Path $upstream $license) -Destination (Join-Path $destination $license)
}
$manifest = [ordered]@{
    revision=$revision
    source='https://github.com/gcc-mirror/gcc'
    cases=@($entries | Where-Object role -eq 'case' | Sort-Object path)
    support=@($entries | Where-Object role -eq 'support' | Sort-Object path)
}
$manifest | ConvertTo-Json -Depth 5 | Set-Content (Join-Path $PSScriptRoot 'corpus.json')
Write-Host ('Retained {0} failing cases and {1} supporting files' -f $manifest.cases.Count, $manifest.support.Count)
