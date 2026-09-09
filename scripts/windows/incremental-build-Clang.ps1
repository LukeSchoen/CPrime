# Shared project-build cache. Only successful outputs receive a state record.
$script:fileStamps = @{}
$script:includeDirectoryStamps = @{}
$script:directoryTimes = @{}
$script:buildStates = @{}
$script:fastInputs = @{}
# Cache records are plain data. Avoid building a PowerShell object/property for
# every repeated header and include directory in every translation unit.
Add-Type -AssemblyName System.Web.Extensions
$script:stateJson = [Web.Script.Serialization.JavaScriptSerializer]::new()
$script:stateJson.MaxJsonLength = [int]::MaxValue
$script:buildEnvironment = @('INCLUDE', 'LIB', 'LIBRARY_PATH', 'CPATH', 'C_INCLUDE_PATH', 'CPLUS_INCLUDE_PATH',
    'VCToolsInstallDir', 'VCToolsVersion', 'WindowsSdkDir', 'WindowsSDKVersion', 'PATH')
$script:cacheContext = (@($CompilerPath, (Get-Item -LiteralPath $CompilerPath).LastWriteTimeUtc.Ticks,
    (Get-Item -LiteralPath $CompilerPath).Length,
    (Get-FileHash -LiteralPath $PSCommandPath).Hash,
    (Get-FileHash -LiteralPath (Join-Path $PSScriptRoot '../../build_project_Clang.ps1')).Hash) +
    @($script:buildEnvironment | ForEach-Object { $_ + '=' + [Environment]::GetEnvironmentVariable($_) })) -join "`n"
$script:fastCheckSource = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../src/tools/check_project_build.c'))
$script:fastChecker = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../build/check_project_build.exe'))
$script:fastSnapshot = Join-Path $OutDir $(if ($Unity) { 'check-unity.bin' } else { 'check-separate.bin' })
if ($Toolchain -eq 'Prime') {
    # Invalidate an old fast result as soon as the full driver is entered.
    # Create the file before recording directory times; overwrite it in place.
    [IO.File]::WriteAllBytes($script:fastSnapshot, [byte[]]@())
}

function Get-InputStamp([string]$Path) {
    if (-not $script:fileStamps.ContainsKey($Path)) {
        $item = [IO.FileInfo]::new($Path)
        $script:fileStamps[$Path] = if ($item.Exists) { "$($item.Length):$($item.LastWriteTimeUtc.Ticks)" } else { 'missing' }
    }
    $script:fileStamps[$Path]
}

function Get-BuildKey([string]$Tool, [string[]]$Arguments, [string]$Directory) {
    $script:fastInputs[$Tool] = Get-InputStamp $Tool
    $sha = [Security.Cryptography.SHA256]::Create()
    try {
        $text = @($script:cacheContext, $Tool, (Get-InputStamp $Tool), $Directory) + $Arguments
        [Convert]::ToBase64String($sha.ComputeHash([Text.Encoding]::UTF8.GetBytes(($text -join "`n"))))
    } finally { $sha.Dispose() }
}

function Get-IncludeDirectoryStamp([string]$Path) {
    if (-not $script:includeDirectoryStamps.ContainsKey($Path)) {
        $script:directoryTimes[$Path] = [IO.Directory]::GetLastWriteTimeUtc($Path).ToFileTimeUtc()
        $names = if ([IO.Directory]::Exists($Path)) {
            foreach ($file in [IO.Directory]::EnumerateFiles($Path)) {
                if ([IO.Path]::GetExtension($file) -in @('', '.h', '.hpp', '.hxx', '.inc', '.inl')) { [IO.Path]::GetFileName($file) }
            }
        }
        # Do not use directory mtime: writing a log or executable alongside
        # headers would make every build dirty. Watch header-name additions.
        $script:includeDirectoryStamps[$Path] = ($names | Sort-Object) -join "`n"
    }
    $script:includeDirectoryStamps[$Path]
}

function Test-BuildState([string]$Output, [string]$Key) {
    if ($Rebuild -or $Toolchain -ne 'Prime' -or -not [IO.File]::Exists("$Output.state.json")) { return $false }
    try {
        $state = $script:stateJson.DeserializeObject([IO.File]::ReadAllText("$Output.state.json"))
        if ($state.Key -ne $Key -or $state.Output -ne (Get-InputStamp $Output)) { return $false }
        foreach ($input in $state.Inputs) {
            $path = [string]$input.Path
            if (-not $script:fileStamps.ContainsKey($path)) { $null = Get-InputStamp $path }
            if ($input.Stamp -eq 'missing' -or $input.Stamp -ne $script:fileStamps[$path]) { return $false }
        }
        foreach ($directory in $state.Directories) {
            # Reuse the saved header-name list while the directory is unchanged.
            # A changed directory still needs a name scan: unrelated output
            # writes must not force recompilation, but new headers must.
            $path = [string]$directory.Path
            if (-not $script:includeDirectoryStamps.ContainsKey($path)) {
                if ($null -ne $directory.Time -and
                    [IO.Directory]::GetLastWriteTimeUtc($path).ToFileTimeUtc() -eq $directory.Time) {
                    $script:directoryTimes[$path] = [long]$directory.Time
                    $script:includeDirectoryStamps[$path] = $directory.Stamp
                } else { $null = Get-IncludeDirectoryStamp $path }
            }
            if ($directory.Stamp -cne $script:includeDirectoryStamps[$path]) { return $false }
        }
        $script:buildStates[$Output] = $state
        return $true
    } catch { return $false }
}

function Read-BuildDependencies([string]$Path, [string]$Directory) {
    if (-not [IO.File]::Exists($Path)) { throw "Compiler did not produce dependencies: $Path" }
    # CPC emits one escaped prerequisite per continuation line. This also
    # accepts ordinary Make dependency output, with spaces and drive letters.
    $text = [IO.File]::ReadAllText($Path) -replace '\\\r?\n', ' '
    $text = $text -replace '^.*?:\s+', ''
    foreach ($match in [regex]::Matches($text, '(?:\\[\s#]|[^\s])+')) {
        $name = $match.Value -replace '\\([\s#])', '$1'
        $name = $name.Replace('$$', '$')
        if (-not [IO.Path]::IsPathRooted($name)) { $name = Join-Path $Directory $name }
        [IO.Path]::GetFullPath($name)
    }
}

function Save-BuildState([string]$Output, [string]$Key, [string[]]$Inputs, [string[]]$Directories = @()) {
    $script:fileStamps.Remove($Output)
    $seen = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
    $records = foreach ($path in $Inputs) {
        if ($path -ne $Output -and $seen.Add($path)) { @{ Path = $path; Stamp = Get-InputStamp $path } }
    }
    $seen.Clear()
    $dirs = foreach ($path in $Directories) {
        if ($seen.Add($path)) {
            $stamp = Get-IncludeDirectoryStamp $path
            @{ Path = $path; Stamp = $stamp; Time = $script:directoryTimes[$path] }
        }
    }
    $state = @{ Key = $Key; Output = Get-InputStamp $Output; Inputs = @($records); Directories = @($dirs) }
    $script:buildStates[$Output] = $state
    $state |
        ConvertTo-Json -Depth 5 -Compress | Set-Content -LiteralPath "$Output.state.json" -Encoding UTF8
}

function Write-ChangedText([string]$Path, [string]$Text) {
    if (-not [IO.File]::Exists($Path) -or [IO.File]::ReadAllText($Path) -cne $Text) {
        [IO.File]::WriteAllText($Path, $Text, [Text.UTF8Encoding]::new($false))
    }
}

function Invoke-CachedTool([string]$Tool, [string[]]$Arguments, [string]$Directory,
    [string]$Label, [string]$Output, [string[]]$Inputs, [string]$Depfile = '', [string[]]$Directories = @()) {
    $key = Get-BuildKey $Tool $Arguments $Directory
    if (Test-BuildState $Output $key) { return }
    [IO.File]::Delete("$Output.state.json")
    [IO.File]::Delete($Output)
    Invoke-BuildTool $Tool $Arguments $Directory $Label
    if (-not [IO.File]::Exists($Output)) { throw "$Label did not produce $Output" }
    if ($Depfile) { $Inputs += @(Read-BuildDependencies $Depfile $Directory) }
    Save-BuildState $Output $key $Inputs $Directories
}

function Get-ResourceInputs([string]$Source, [string[]]$Includes, [string[]]$Definitions = @()) {
    # RC has no Make-dependency mode. Follow references in every conditional
    # branch, including quoted macro definitions. Binary assets may use any
    # extension or live outside the resource/include directory trees.
    $pending = [Collections.Generic.Queue[string]]::new()
    $seen = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
    $macroIncludes = $false
    $pending.Enqueue($Source)
    while ($pending.Count) {
        $file = $pending.Dequeue()
        if (-not $seen.Add($file)) { continue }
        $text = [IO.File]::ReadAllText($file)
        if ($file -eq $Source) { $text += "`n" + ($Definitions -join "`n") }
        if ($text -match '(?m)^\s*#\s*include\s+[^\s"<]') { $macroIncludes = $true }
        foreach ($match in [regex]::Matches($text, '"([^"\r\n]+)"|#\s*include\s*<([^>]+)>')) {
            $name = if ($match.Groups[1].Success) { $match.Groups[1].Value } else { $match.Groups[2].Value }
            foreach ($directory in @((Split-Path $file -Parent)) + $Includes) {
                try { $candidate = [IO.Path]::GetFullPath((Join-Path $directory $name)) } catch { continue }
                if ([IO.File]::Exists($candidate)) {
                    $candidate
                    if ([IO.Path]::GetExtension($candidate) -in @('.h', '.hpp', '.rc', '.rc2', '.inc')) { $pending.Enqueue($candidate) }
                    break
                }
            }
        }
    }
    if ($macroIncludes) {
        # A computed include name cannot be resolved by this lexical scan.
        # Conservatively include its search trees instead of caching blindly.
        foreach ($path in (@((Split-Path $Source -Parent)) + $Includes | Select-Object -Unique)) {
            if ([IO.Directory]::Exists($path)) {
                foreach ($file in [IO.Directory]::EnumerateFiles($path, '*', [IO.SearchOption]::AllDirectories)) {
                    if (-not $file.StartsWith($OutDir.TrimEnd('\') + '\', [StringComparison]::OrdinalIgnoreCase) -and
                        [IO.Path]::GetExtension($file) -in @('.h', '.hpp', '.inl', '.inc', '.rc', '.rc2')) { $file }
                }
            }
        }
    }
    $Source
}

function Ensure-FastBuildChecker {
    if ($Toolchain -ne 'Prime') { return }
    $checkerKey = Get-BuildKey $CompilerPath @('-O2', $script:fastCheckSource) (Split-Path $script:fastCheckSource -Parent)
    if (-not (Test-BuildState $script:fastChecker $checkerKey)) {
        # The helper is compiled serially by CPC, and only when its inputs
        # change. No PowerShell/C# runtime or resident watcher is required.
        Invoke-BuildTool $CompilerPath @('-O2', $script:fastCheckSource, '-o', $script:fastChecker) (Split-Path $script:fastCheckSource -Parent) 'build_checker'
        Save-BuildState $script:fastChecker $checkerKey @($script:fastCheckSource, $CompilerPath)
    }
}

function Save-FastBuildSnapshot {
    if ($Toolchain -ne 'Prime') { return }
    $files = @{}
    $directories = @{}
    foreach ($output in $script:buildStates.Keys) {
        $state = $script:buildStates[$output]
        $files[$output] = $state.Output
        $files["$output.state.json"] = Get-InputStamp "$output.state.json"
        foreach ($input in $state.Inputs) { $files[$input.Path] = $input.Stamp }
        foreach ($directory in $state.Directories) { $directories[$directory.Path] = $true }
    }
    foreach ($path in $script:fastInputs.Keys) { $files[$path] = $script:fastInputs[$path] }
    foreach ($path in $BuildInputs) {
        $files[[IO.Path]::GetFullPath($path)] = Get-InputStamp $path
    }
    foreach ($path in @($ManifestPath, $CompilerPath, $script:fastCheckSource, $script:fastChecker,
        (Join-Path $PSScriptRoot 'incremental-build-Clang.ps1'),
        (Join-Path $PSScriptRoot '../../build_project_Clang.ps1'))) {
        $files[[IO.Path]::GetFullPath($path)] = Get-InputStamp $path
    }
    # Outputs/logs can change a watched directory's timestamp without changing
    # its headers. Refresh that timestamp only if the header names still match.
    foreach ($path in $directories.Keys) {
        $oldTime = $script:directoryTimes[$path]
        $oldNames = $script:includeDirectoryStamps[$path]
        if ([IO.Directory]::GetLastWriteTimeUtc($path).ToFileTimeUtc() -ne $oldTime) {
            $script:includeDirectoryStamps.Remove($path)
            if ((Get-IncludeDirectoryStamp $path) -cne $oldNames) { $script:directoryTimes[$path] = $oldTime }
        }
    }
    $stream = [IO.File]::Open($script:fastSnapshot, [IO.FileMode]::Create, [IO.FileAccess]::Write, [IO.FileShare]::None)
    $writer = [IO.BinaryWriter]::new($stream)
    function Write-SnapshotString([string]$Value) {
        $bytes = [Text.Encoding]::UTF8.GetBytes($Value)
        $writer.Write([uint32]$bytes.Length); $writer.Write($bytes)
    }
    try {
        $writer.Write([Text.Encoding]::ASCII.GetBytes('CPCCHK02'))
        Write-SnapshotString $ExePath
        Write-SnapshotString $CompilerPath
        Write-SnapshotString ((@($CompilerPath) + @($linkArguments) | ForEach-Object { Quote-Native $_ }) -join ' ')
        Write-SnapshotString $applications[0].directory
        $writer.Write([uint32]$CompileTimeoutSeconds)
        $writer.Write([uint32]$allJobs.Count)
        $writer.Write([uint32]$script:buildEnvironment.Count)
        foreach ($name in $script:buildEnvironment) {
            Write-SnapshotString $name
            Write-SnapshotString ([Environment]::GetEnvironmentVariable($name))
        }
        $writer.Write([uint32]($files.Count + $directories.Count))
        foreach ($path in $files.Keys) {
            Write-SnapshotString $path
            $stamp = $files[$path]
            if ($stamp -eq 'missing') {
                $writer.Write([uint32]2); $writer.Write([uint64]0); $writer.Write([uint64]0)
            } else {
                $parts = $stamp.Split(':')
                $writer.Write([uint32]$(if ($path -eq $ExePath) { 3 } else { 0 }))
                $writer.Write([uint64]([long]$parts[1] - 504911232000000000L))
                $writer.Write([uint64]$parts[0])
            }
        }
        foreach ($path in $directories.Keys) {
            Write-SnapshotString $path
            $writer.Write([uint32]$(if ([IO.Directory]::Exists($path)) { 1 } else { 2 }))
            $writer.Write([uint64]$script:directoryTimes[$path]); $writer.Write([uint64]0)
        }
    } finally { $writer.Dispose() }
}
