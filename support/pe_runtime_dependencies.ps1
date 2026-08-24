function Get-PeImageInfo {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Path
    )

    $fullPath = [System.IO.Path]::GetFullPath($Path)
    $bytes = [System.IO.File]::ReadAllBytes($fullPath)
    if ($bytes.Length -lt 64) { throw "PE image is too small: $fullPath" }

    function Read-U16([int]$Offset) {
        if ($Offset -lt 0 -or $Offset + 2 -gt $bytes.Length) {
            throw "Invalid PE offset in $fullPath"
        }
        return [BitConverter]::ToUInt16($bytes, $Offset)
    }
    function Read-U32([int]$Offset) {
        if ($Offset -lt 0 -or $Offset + 4 -gt $bytes.Length) {
            throw "Invalid PE offset in $fullPath"
        }
        return [BitConverter]::ToUInt32($bytes, $Offset)
    }
    function Read-AsciiZ([int]$Offset) {
        $end = $Offset
        while ($end -lt $bytes.Length -and $bytes[$end] -ne 0) { ++$end }
        if ($end -ge $bytes.Length) { throw "Unterminated PE string in $fullPath" }
        return [Text.Encoding]::ASCII.GetString($bytes, $Offset, $end - $Offset)
    }

    if ((Read-U16 0) -ne 0x5a4d) { throw "Not a PE image: $fullPath" }
    $peOffset = [int](Read-U32 0x3c)
    if ((Read-U32 $peOffset) -ne 0x00004550) { throw "Invalid PE signature: $fullPath" }

    $coffOffset = $peOffset + 4
    $machine = Read-U16 $coffOffset
    $sectionCount = Read-U16 ($coffOffset + 2)
    $optionalSize = Read-U16 ($coffOffset + 16)
    $optionalOffset = $coffOffset + 20
    $optionalMagic = Read-U16 $optionalOffset
    if ($optionalMagic -eq 0x20b) {
        $dataDirectoryOffset = $optionalOffset + 112
    } elseif ($optionalMagic -eq 0x10b) {
        $dataDirectoryOffset = $optionalOffset + 96
    } else {
        throw ("Unsupported PE optional-header magic 0x{0:x}: {1}" -f $optionalMagic, $fullPath)
    }

    $sections = @()
    $sectionOffset = $optionalOffset + $optionalSize
    for ($index = 0; $index -lt $sectionCount; ++$index) {
        $offset = $sectionOffset + 40 * $index
        $sections += [pscustomobject]@{
            VirtualSize = Read-U32 ($offset + 8)
            Rva = Read-U32 ($offset + 12)
            RawSize = Read-U32 ($offset + 16)
            RawOffset = Read-U32 ($offset + 20)
        }
    }

    function Convert-RvaToOffset([uint32]$Rva) {
        foreach ($section in $sections) {
            $size = [Math]::Max([uint32]$section.VirtualSize, [uint32]$section.RawSize)
            if ($Rva -ge $section.Rva -and $Rva -lt ($section.Rva + $size)) {
                return [int]($section.RawOffset + ($Rva - $section.Rva))
            }
        }
        if ($Rva -lt $sectionOffset) { return [int]$Rva }
        throw ("Unmapped RVA 0x{0:x} in {1}" -f $Rva, $fullPath)
    }

    $imports = @()
    $importRva = Read-U32 ($dataDirectoryOffset + 8)
    $importSize = Read-U32 ($dataDirectoryOffset + 12)
    if ($importRva -ne 0 -and $importSize -ne 0) {
        $descriptorOffset = Convert-RvaToOffset $importRva
        $descriptorEnd = [Math]::Min($bytes.Length, $descriptorOffset + $importSize)
        while ($descriptorOffset + 20 -le $descriptorEnd) {
            $nameRva = Read-U32 ($descriptorOffset + 12)
            if ($nameRva -eq 0) { break }
            $imports += Read-AsciiZ (Convert-RvaToOffset $nameRva)
            $descriptorOffset += 20
        }
    }

    $machineName = switch ($machine) {
        0x014c { "x86" }
        0x8664 { "x64" }
        0xaa64 { "ARM64" }
        default { "0x{0:x}" -f $machine }
    }
    return [pscustomobject]@{
        Path = $fullPath
        Machine = $machine
        MachineName = $machineName
        Imports = @($imports | Sort-Object -Unique)
    }
}

function Test-PeRuntimeDependencies {
    param(
        [Parameter(Mandatory = $true)]
        [string]$ImagePath,
        [string[]]$SearchDirectories = @(),
        [switch]$ThrowOnError
    )

    $root = Get-PeImageInfo -Path $ImagePath
    $imageDirectory = Split-Path $root.Path -Parent
    $localDirectories = @($imageDirectory)
    foreach ($directory in $SearchDirectories) {
        if ($directory) { $localDirectories += [System.IO.Path]::GetFullPath($directory) }
    }
    $localDirectories = @($localDirectories | Select-Object -Unique)
    $systemDirectory = [Environment]::SystemDirectory
    $windowsDirectory = [Environment]::GetFolderPath([Environment+SpecialFolder]::Windows)

    $pending = New-Object System.Collections.Queue
    $pending.Enqueue($root.Path)
    $visited = @{}
    $resolvedLocal = @{}
    $errors = @()

    while ($pending.Count -gt 0) {
        $currentPath = [string]$pending.Dequeue()
        $currentKey = $currentPath.ToLowerInvariant()
        if ($visited.ContainsKey($currentKey)) { continue }
        $visited[$currentKey] = $true

        $current = Get-PeImageInfo -Path $currentPath
        if ($current.Machine -ne $root.Machine) {
            $errors += ("Architecture mismatch: {0} is {1}; expected {2}" -f `
                $current.Path, $current.MachineName, $root.MachineName)
        }

        foreach ($import in $current.Imports) {
            if ($import -match '^(api-ms-win-|ext-ms-win-)') { continue }

            $resolved = $null
            foreach ($directory in $localDirectories) {
                $candidate = Join-Path $directory $import
                if (Test-Path -LiteralPath $candidate -PathType Leaf) {
                    $resolved = [System.IO.Path]::GetFullPath($candidate)
                    break
                }
            }

            if ($resolved) {
                $resolvedKey = $resolved.ToLowerInvariant()
                $resolvedLocal[$resolvedKey] = $resolved
                $pending.Enqueue($resolved)
                continue
            }

            $systemCandidates = @(
                (Join-Path $systemDirectory $import),
                (Join-Path $windowsDirectory $import)
            )
            if (-not ($systemCandidates | Where-Object { Test-Path -LiteralPath $_ -PathType Leaf } | Select-Object -First 1)) {
                $errors += ("Missing dependency: {0} imports {1}" -f `
                    (Split-Path $current.Path -Leaf), $import)
            }
        }
    }

    $result = [pscustomobject]@{
        Image = $root.Path
        Machine = $root.MachineName
        LocalImages = @($resolvedLocal.Values | Sort-Object)
        Errors = @($errors | Sort-Object -Unique)
        Success = $errors.Count -eq 0
    }
    if ($ThrowOnError -and -not $result.Success) {
        throw ("PE runtime dependency validation failed:`n  " + ($result.Errors -join "`n  "))
    }
    return $result
}
