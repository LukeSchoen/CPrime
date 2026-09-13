function Read-GccCorpus([string]$Directory) {
    $manifest = Get-Content -LiteralPath (Join-Path $Directory 'corpus.json') -Raw | ConvertFrom-Json
    $base = [IO.Path]::GetFullPath((Join-Path $Directory 'corpus'))
    # An empty checked set is the valid terminal inventory.  Still validate
    # every retained support entry: an empty manifest is not permission to
    # bypass path containment or upstream-integrity checks.
    $seen = @{}
    foreach ($entry in (@($manifest.cases) + @($manifest.support))) {
        $path = [IO.Path]::GetFullPath((Join-Path $base $entry.path))
        if (-not $path.StartsWith($base + '\', [StringComparison]::OrdinalIgnoreCase)) { throw 'Corpus path escapes its root' }
        if ($seen.ContainsKey($path)) { throw "Duplicate corpus path: $($entry.path)" }
        $seen[$path] = $true
        if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { throw "Missing corpus file: $($entry.path)" }
        if ((Get-FileHash -LiteralPath $path).Hash -ine $entry.sha256) { throw "Modified upstream file: $($entry.path)" }
    }
    return $manifest
}
