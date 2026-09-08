param(
    [Parameter(Mandatory = $true)][string[]]$ExePath,
    [string]$JsonPath = ''
)
$ErrorActionPreference = 'Stop'
function Resolve-PEOffset($Sections, [uint32]$Rva) {
    foreach ($section in $Sections) {
        if ($Rva -ge $section.RVA -and $Rva -lt [long]$section.RVA + $section.Bytes) {
            return [int]($section.Offset + $Rva - $section.RVA)
        }
    }
    throw "RVA outside file-backed PE sections: $Rva"
}
$results = foreach ($path in $ExePath) {
    $file = (Resolve-Path -LiteralPath $path).Path
    $bytes = [IO.File]::ReadAllBytes($file)
    if ($bytes.Length -lt 64 -or [BitConverter]::ToUInt16($bytes, 0) -ne 0x5a4d) {
        throw "Not a PE executable: $file"
    }
    $pe = [BitConverter]::ToInt32($bytes, 60)
    if ($pe -lt 0 -or $pe + 24 -gt $bytes.Length -or [BitConverter]::ToUInt32($bytes, $pe) -ne 0x4550) {
        throw "Invalid PE header: $file"
    }
    $count = [BitConverter]::ToUInt16($bytes, $pe + 6)
    $table = $pe + 24 + [BitConverter]::ToUInt16($bytes, $pe + 20)
    if ($table + 40 * $count -gt $bytes.Length) { throw "Invalid section table: $file" }
    $end = $table + 40 * $count
    $sections = @(for ($i = 0; $i -lt $count; ++$i) {
        $offset = $table + 40 * $i
        $size = [BitConverter]::ToUInt32($bytes, $offset + 16)
        $position = [BitConverter]::ToUInt32($bytes, $offset + 20)
        if ([long]$position + $size -gt $bytes.Length) { throw "Invalid section data: $file" }
        $end = [Math]::Max($end, [long]$position + $size)
        [pscustomobject]@{
            Name = [Text.Encoding]::ASCII.GetString($bytes, $offset, 8).Trim([char]0)
            Bytes = $size
            RVA = [BitConverter]::ToUInt32($bytes, $offset + 12)
            Offset = $position
        }
    })
    $optional = $pe + 24
    $magic = [BitConverter]::ToUInt16($bytes, $optional)
    $directories = if ($magic -eq 0x20b) { $optional + 112 } elseif ($magic -eq 0x10b) { $optional + 96 } else { throw "Unknown PE format: $file" }
    $importRva = [BitConverter]::ToUInt32($bytes, $directories + 8)
    $imports = @(if ($importRva) {
        $descriptor = Resolve-PEOffset $sections $importRva
        while ($descriptor + 20 -le $bytes.Length) {
            $nameRva = [BitConverter]::ToUInt32($bytes, $descriptor + 12)
            if (-not $nameRva) { break }
            $nameOffset = Resolve-PEOffset $sections $nameRva
            $nameEnd = $nameOffset
            while ($nameEnd -lt $bytes.Length -and $bytes[$nameEnd]) { ++$nameEnd }
            if ($nameEnd -eq $bytes.Length) { throw "Unterminated DLL name: $file" }
            [Text.Encoding]::ASCII.GetString($bytes, $nameOffset, $nameEnd - $nameOffset)
            $descriptor += 20
        }
    })
    $payloadBytes = 0
    $unpackedBytes = 0
    if ([Text.Encoding]::ASCII.GetString($bytes, $bytes.Length - 16, 8) -eq 'CPCPAY11') {
        $payload = [BitConverter]::ToUInt64($bytes, $bytes.Length - 8)
        if ($payload -lt $end -or $payload + 32 -gt $bytes.Length) { throw "Invalid payload: $file" }
        $payloadBytes = $bytes.Length - $payload
        $unpackedBytes = [BitConverter]::ToUInt64($bytes, $payload)
    }
    [pscustomobject]@{
        Path = $file
        SHA256 = (Get-FileHash -LiteralPath $file -Algorithm SHA256).Hash
        TotalBytes = $bytes.Length
        PEBytes = $end
        OverlayBytes = $bytes.Length - $end
        PortableBytes = $payloadBytes
        UnpackedPortableBytes = $unpackedBytes
        Sections = $sections
        Imports = $imports
    }
}
if ($JsonPath) { @($results) | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $JsonPath -Encoding UTF8 }
$results
