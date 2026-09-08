# Content identity is independent of timestamps and directory enumeration order.
function Get-GccInputIdentity([string]$Root, [string[]]$Paths) {
    $base = [IO.Path]::GetFullPath($Root).TrimEnd('\', '/')
    $entries = New-Object 'Collections.Generic.List[object]'
    [string[]]$ordered = @($Paths)
    [Array]::Sort($ordered, [StringComparer]::Ordinal)
    foreach ($path in $ordered) {
        $full = [IO.Path]::GetFullPath($path)
        if (-not $full.StartsWith($base + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) {
            throw "Input is outside identity root: $full"
        }
        $entries.Add([ordered]@{
            path = $full.Substring($base.Length + 1).Replace('\', '/')
            sha256 = (Get-FileHash -LiteralPath $full -Algorithm SHA256).Hash.ToLowerInvariant()
        })
    }
    return @($entries.ToArray())
}
