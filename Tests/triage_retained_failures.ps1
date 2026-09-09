[CmdletBinding()]
param(
    [string]$ResultsPath = '',
    [string]$OutBase = '',
    [ValidateRange(5, 100)][int]$Top = 25
)

$ErrorActionPreference = 'Stop'
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$rootDir = (Resolve-Path (Join-Path $scriptDir '..')).Path
if (-not $OutBase) {
    $OutBase = Join-Path $rootDir 'build/compiler-bug-triage'
}
$OutBase = [IO.Path]::GetFullPath($OutBase)
$buildRoot = (Resolve-Path (Join-Path $rootDir 'build')).Path + [IO.Path]::DirectorySeparatorChar
if (-not $OutBase.StartsWith($buildRoot, [StringComparison]::OrdinalIgnoreCase)) {
    throw "Triage output must stay under build/: $OutBase"
}

if (-not $ResultsPath) {
    $candidates = @(Get-ChildItem -LiteralPath (Join-Path $rootDir 'build') -Filter results.jsonl -Recurse -File -ErrorAction SilentlyContinue |
        Where-Object { $_.FullName -match 'pedantic-gcc-[0-9a-f]{32}' })
    if (-not $candidates.Count) {
        throw 'No retained GCC results.jsonl found under build/. Pass -ResultsPath explicitly.'
    }
    $ResultsPath = ($candidates | Sort-Object LastWriteTime -Descending | Select-Object -First 1).FullName
}
$ResultsPath = (Resolve-Path -LiteralPath $ResultsPath).Path
$resultDir = Split-Path -Parent $ResultsPath

$rows = @(Get-Content -LiteralPath $ResultsPath | ConvertFrom-Json)
$counts = @{}
foreach ($row in $rows) {
    $counts[$row.status] = 1 + [int]$counts[$row.status]
}

function Get-SourceArea {
    param([string]$Path)
    $parts = $Path.Split('/')
    if ($parts[0] -eq 'g++.dg' -or $parts[0] -eq 'g++.old-deja') {
        if ($parts.Count -ge 2) { return $parts[0] + '/' + $parts[1] }
        return $parts[0]
    }
    if ($parts[0] -eq 'c-c++-common' -and $parts.Count -ge 3) {
        return $parts[0] + '/' + $parts[1]
    }
    return $parts[0]
}

function Get-FirstDiagnostic {
    param($Row)
    $output = ''
    if ($Row.status -eq 'FAIL_COMPILE' -and $Row.compile) {
        $output = [string]$Row.compile.output
    } elseif ($Row.status -like 'FAIL_RUN*' -and $Row.run) {
        $output = [string]$Row.run.output
    }
    foreach ($line in ($output -split "`n")) {
        if ($line -match ':\s*error:\s*(.*)$') {
            $message = $matches[1].Trim()
            if ($message.Length -gt 140) { $message = $message.Substring(0, 137) + '...' }
            return $message
        }
        if ($line -match '^cpc:\s*error:\s*(.*)$') {
            $message = $matches[1].Trim()
            if ($message.Length -gt 140) { $message = $message.Substring(0, 137) + '...' }
            return $message
        }
    }
    if ($Row.status -like 'FAIL_RUN*') {
        $exit = if ($Row.run) { [string]$Row.run.exit } else { 'unknown' }
        return "<runtime $exit>"
    }
    return '<no diagnostic>'
}

$unresolved = @($rows | Where-Object {
    $_.status -like 'FAIL*' -or $_.status -eq 'CRASH'
})
$clusterKeys = @{}
$clusters = New-Object 'System.Collections.Generic.List[object]'
foreach ($row in $unresolved) {
    $area = Get-SourceArea $row.path
    $message = Get-FirstDiagnostic $row
    $key = $area + '|' + $message
    if (-not $clusterKeys.ContainsKey($key)) {
        $clusterKeys[$key] = @{
            area = $area
            message = $message
            count = 0
            paths = New-Object 'System.Collections.Generic.List[string]'
            statuses = @{}
        }
        $clusters.Add($clusterKeys[$key])
    }
    $cluster = $clusterKeys[$key]
    $cluster.count++
    $cluster.paths.Add($row.path)
    $cluster.statuses[$row.status] = 1 + [int]$cluster.statuses[$row.status]
}

$sortedClusters = @($clusters | Sort-Object @{ Expression = { -$_.count } })
$topClusters = @($sortedClusters | Select-Object -First $Top)
$areaCounts = @{}
foreach ($cluster in $clusters) {
    $areaCounts[$cluster.area] = $cluster.count + [int]$areaCounts[$cluster.area]
}
$topAreas = @($areaCounts.GetEnumerator() |
    Sort-Object @{ Expression = { -$_.Value } } |
    Select-Object -First $Top)

$metadata = $null
$metadataPath = Join-Path $resultDir 'metadata.json'
if (Test-Path -LiteralPath $metadataPath) {
    $metadata = Get-Content -Raw -LiteralPath $metadataPath | ConvertFrom-Json
}

$utf8 = New-Object Text.UTF8Encoding($false)
[IO.Directory]::CreateDirectory((Split-Path -Parent $OutBase)) | Out-Null

$text = New-Object System.Text.StringBuilder
[void]$text.AppendLine('Retained compiler-bug triage')
[void]$text.AppendLine(('Generated: ' + (Get-Date).ToString('s')))
[void]$text.AppendLine(('Results: ' + $ResultsPath))
if ($metadata) {
    if ($metadata.compiler) { [void]$text.AppendLine('Compiler: ' + $metadata.compiler) }
    if ($metadata.compiler_sha256) { [void]$text.AppendLine('Compiler SHA256: ' + $metadata.compiler_sha256) }
    if ($metadata.source_commit) { [void]$text.AppendLine('Source commit: ' + $metadata.source_commit) }
}
[void]$text.AppendLine(('Selected retained rows: ' + $rows.Count))
[void]$text.AppendLine(('Unresolved retained rows: ' + $unresolved.Count))
[void]$text.AppendLine('Status counts:')
foreach ($key in ($counts.Keys | Sort-Object)) {
    [void]$text.AppendLine(('  {0}: {1}' -f $key, $counts[$key]))
}
[void]$text.AppendLine()
[void]$text.AppendLine('Top shared clusters (area + first diagnostic):')
for ($i = 0; $i -lt $topClusters.Count; ++$i) {
    $cluster = $topClusters[$i]
    [void]$text.AppendLine(('  {0,3}. [{1}] x{2}' -f ($i + 1), $cluster.area, $cluster.count))
    [void]$text.AppendLine(('       ' + $cluster.message))
    $statusText = @($cluster.statuses.GetEnumerator() |
        Sort-Object Name | ForEach-Object { $_.Name + '=' + $_.Value }) -join ', '
    [void]$text.AppendLine(('       statuses: ' + $statusText))
    for ($j = 0; $j -lt $cluster.paths.Count -and $j -lt 4; ++$j) {
        [void]$text.AppendLine(('         ' + $cluster.paths[$j]))
    }
    if ($cluster.paths.Count -gt 4) {
        [void]$text.AppendLine(('         ... +' + ($cluster.paths.Count - 4) + ' more'))
    }
}
[void]$text.AppendLine()
[void]$text.AppendLine('Top source areas:')
for ($i = 0; $i -lt $topAreas.Count; ++$i) {
    [void]$text.AppendLine(('  {0,3}. {1}: {2}' -f ($i + 1), $topAreas[$i].Name,
                            $topAreas[$i].Value))
}
[void]$text.AppendLine()
[void]$text.AppendLine('Full machine-readable inventory: ' + $OutBase + '.json')
[IO.File]::WriteAllText($OutBase + '.txt', $text.ToString(), $utf8)

$jsonClusters = @($sortedClusters | ForEach-Object {
    @{
        area = $_.area
        message = $_.message
        count = $_.count
        statuses = $_.statuses
        paths = @($_.paths)
    }
})
$json = [ordered]@{
    generated = (Get-Date).ToString('s')
    result_file = $ResultsPath
    compiler = if ($metadata) { $metadata.compiler } else { '' }
    compiler_sha256 = if ($metadata) { $metadata.compiler_sha256 } else { '' }
    source_commit = if ($metadata) { $metadata.source_commit } else { '' }
    selected = $rows.Count
    unresolved = $unresolved.Count
    counts = $counts
    top_clusters = $jsonClusters
    top_areas = @($topAreas | ForEach-Object {
        @{ area = $_.Name; count = $_.Value }
    })
}
[IO.File]::WriteAllText($OutBase + '.json',
    ($json | ConvertTo-Json -Depth 6), $utf8)

Write-Host ('Triage written to {0}.txt and {0}.json' -f $OutBase)
Write-Host ('Unresolved: {0}; top clusters: {1}' -f $unresolved.Count, $topClusters.Count)
