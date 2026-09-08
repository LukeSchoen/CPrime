$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'corpus.ps1')
$manifest = Read-GccCorpus $PSScriptRoot
$listed = @(& powershell -NoProfile -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot 'run.ps1') -List)
if ($LASTEXITCODE -ne 0 -or $listed.Count -ne $manifest.cases.Count) { throw 'Retained case inventory changed' }
foreach ($entry in $manifest.support) { if ($entry.path -in $listed) { throw 'Support file became an independent test' } }
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$work = Join-Path $root ('build/corpus-check-' + [guid]::NewGuid().ToString('N'))
try {
    New-Item -ItemType Directory -Force (Join-Path $work 'corpus') | Out-Null
    $file = Join-Path $work 'corpus/probe.C'
    'int value;' | Set-Content $file
    $fixture = @{cases=@(@{path='probe.C';sha256=(Get-FileHash $file).Hash});support=@()}
    $fixture | ConvertTo-Json -Depth 4 | Set-Content (Join-Path $work 'corpus.json')
    [void](Read-GccCorpus $work)
    'int other;' | Set-Content $file
    $rejected = $false
    try { [void](Read-GccCorpus $work) } catch { $rejected = $true }
    if (-not $rejected) { throw 'Changed upstream source was accepted' }
    $fixture.cases[0].path = '../escape.C'
    $fixture | ConvertTo-Json -Depth 4 | Set-Content (Join-Path $work 'corpus.json')
    $rejected = $false
    try { [void](Read-GccCorpus $work) } catch { $rejected = $true }
    if (-not $rejected) { throw 'Escaping corpus path was accepted' }
    Write-Host ("PASS {0} checked cases, support exclusion, source hashes, and path containment" -f $manifest.cases.Count)
} finally {
    $resolved = [IO.Path]::GetFullPath($work)
    if (-not $resolved.StartsWith($root + '\build\', [StringComparison]::OrdinalIgnoreCase)) { throw 'Unsafe fixture directory' }
    Remove-Item -LiteralPath $resolved -Recurse -Force
}
