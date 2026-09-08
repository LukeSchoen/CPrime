param(
    [string]$ProjectRoot = (Get-Location).Path,
    [string]$Configuration = 'Release',
    [string]$Platform = 'x64',
    [Parameter(Mandatory = $true)][string]$SolutionPath,
    [string]$OutputDirectory = ''
)

$ErrorActionPreference = 'Stop'
$ProjectRoot = [IO.Path]::GetFullPath($ProjectRoot)
$solution = if ([IO.Path]::IsPathRooted($SolutionPath)) { [IO.Path]::GetFullPath($SolutionPath) } else { [IO.Path]::GetFullPath((Join-Path $ProjectRoot $SolutionPath)) }
$selection = "$Configuration|$Platform"

function Test-Selection($Node) {
    # XPath returns these nodes in document order, so excluded parents still
    # suppress their children before any child condition is interpreted.
    foreach ($ancestor in $Node.SelectNodes('ancestor-or-self::*[@Condition]')) {
        $condition = $ancestor.GetAttribute('Condition')
        if (-not $condition) { continue }
        # Premake emits these exact configuration conditions. Reject unfamiliar
        # expressions rather than silently exporting a different build graph.
        if ($condition -match "^'\`$\(Configuration\)\|\`$\(Platform\)'\s*==\s*'([^']+)'$") {
            if ($Matches[1] -ne $selection) { return $false }
        } else { throw "Unsupported generated project condition: $condition" }
    }
    return $true
}

function Expand-ProjectValue([string]$Value, [hashtable]$Properties) {
    if (-not $Value.Contains('$(')) { return $Value }
    $expanded = [regex]::Replace($Value, '\$\(([^)]+)\)', {
        param($match)
        $key = $match.Groups[1].Value
        if (-not $Properties.ContainsKey($key)) { throw "Unresolved project property: $key" }
        return [string]$Properties[$key]
    })
    return $expanded
}

function Get-Items([string]$Value, [hashtable]$Properties) {
    foreach ($item in $Value.Split(';')) {
        if (-not $item.Trim() -or $item -match '^%\([^)]+\)$') { continue }
        $expanded = Expand-ProjectValue $item.Trim() $Properties
        if ($expanded) { $expanded }
    }
}

function Merge-ItemSetting([string]$Name, [string]$Value, [string]$Inherited, [hashtable]$Properties) {
    $expanded = Expand-ProjectValue $Value $Properties
    # MSBuild inherits a metadata list only where its placeholder occurs. An
    # explicit list without the placeholder replaces the project defaults.
    return [regex]::Replace($expanded, '%\((?:[^.)]+\.)?' + [regex]::Escape($Name) + '\)', {
        param($match)
        return $Inherited
    })
}

function Resolve-ProjectPath([string]$Value, [string]$Directory) {
    if ([IO.Path]::IsPathRooted($Value)) { return [IO.Path]::GetFullPath($Value) }
    return [IO.Path]::GetFullPath([IO.Path]::Combine($Directory, $Value))
}

function Get-SourceEntries([string]$ItemType, [hashtable]$Defaults, $Document, [hashtable]$Properties, [string]$Directory) {
    foreach ($item in $Document.SelectNodes("//*[local-name()='$ItemType'][@Include]")) {
        if (-not (Test-Selection $item)) { continue }
        $settings = $Defaults.Clone()
        $overrides = @{}
        foreach ($setting in $item.ChildNodes) {
            if ($setting.NodeType -eq 'Element' -and (Test-Selection $setting)) {
                $settings[$setting.LocalName] = Merge-ItemSetting $setting.LocalName $setting.InnerText ([string]$settings[$setting.LocalName]) $Properties
                $overrides[$setting.LocalName] = $settings[$setting.LocalName]
            }
        }
        if ($settings.ExcludedFromBuild -eq 'true') { continue }
        $source = Resolve-ProjectPath (Expand-ProjectValue $item.GetAttribute('Include') $Properties) $Directory
        if (-not [IO.File]::Exists($source)) { throw "Selected source is missing: $source" }
        # Omitted metadata uses the project default; present metadata is the
        # complete effective value, including an intentional empty list.
        $entry = [ordered]@{ path = $source }
        if ($overrides.ContainsKey('PreprocessorDefinitions')) {
            $sourceDefines = @(Get-Items ([string]$settings.PreprocessorDefinitions) $Properties)
            if ($Properties.CharacterSet -eq 'Unicode') { $sourceDefines += 'UNICODE', '_UNICODE' }
            $entry.defines = @($sourceDefines | Select-Object -Unique)
            $overrides.Remove('PreprocessorDefinitions')
        }
        if ($overrides.ContainsKey('AdditionalIncludeDirectories')) {
            $entry.includeDirectories = @(Get-Items ([string]$settings.AdditionalIncludeDirectories) $Properties |
                ForEach-Object { Resolve-ProjectPath $_ $Directory })
            $overrides.Remove('AdditionalIncludeDirectories')
        }
        $overrides.Remove('ExcludedFromBuild')
        if ($overrides.Count) { $entry.settings = $overrides }
        $entry
    }
}

$projects = @()
foreach ($line in Get-Content -LiteralPath $solution) {
    if ($line -notmatch '^Project\("[^"]+"\) = "([^"]+)", "([^"]+\.vcxproj)"') { continue }
    $name = $Matches[1]
    $projectPath = Resolve-ProjectPath $Matches[2] $ProjectRoot
    $directory = Split-Path $projectPath -Parent
    [xml]$document = Get-Content -Raw -LiteralPath $projectPath
    $properties = @{
        Configuration = $Configuration; Platform = $Platform; ProjectName = $name
        ProjectDir = $directory + '\'; SolutionDir = $ProjectRoot + '\'; BuildTag = ''
    }
    $available = @($document.SelectNodes("//*[local-name()='ProjectConfiguration']") |
        ForEach-Object { $_.GetAttribute('Include') })
    if ($available -notcontains $selection) { throw "$name has no $selection configuration" }
    foreach ($group in $document.SelectNodes("/*[local-name()='Project']/*[local-name()='PropertyGroup']")) {
        if (-not (Test-Selection $group)) { continue }
        foreach ($property in $group.ChildNodes) {
            if ($property.NodeType -eq 'Element' -and (Test-Selection $property)) {
                $properties[$property.LocalName] = Expand-ProjectValue $property.InnerText $properties
            }
        }
    }
    $compile = @{}; $link = @{}; $resource = @{}
    foreach ($group in $document.SelectNodes("/*[local-name()='Project']/*[local-name()='ItemDefinitionGroup']")) {
        if (-not (Test-Selection $group)) { continue }
        foreach ($section in $group.ChildNodes) {
            if ($section.NodeType -ne 'Element' -or -not (Test-Selection $section)) { continue }
            $destination = if ($section.LocalName -eq 'ClCompile') { $compile }
                elseif ($section.LocalName -eq 'Link') { $link }
                elseif ($section.LocalName -eq 'ResourceCompile') { $resource }
                else { $null }
            if ($null -eq $destination) { continue }
            foreach ($setting in $section.ChildNodes) {
                if ($setting.NodeType -eq 'Element' -and (Test-Selection $setting)) {
                    $destination[$setting.LocalName] = Merge-ItemSetting $setting.LocalName $setting.InnerText ([string]$destination[$setting.LocalName]) $properties
                }
            }
        }
    }
    $includes = @(Get-Items ([string]$compile.AdditionalIncludeDirectories) $properties |
        ForEach-Object { Resolve-ProjectPath $_ $directory } | Select-Object -Unique)
    $defines = @(Get-Items ([string]$compile.PreprocessorDefinitions) $properties)
    if ($properties.CharacterSet -eq 'Unicode') { $defines += 'UNICODE', '_UNICODE' }
    $sources = @(Get-SourceEntries 'ClCompile' $compile $document $properties $directory)
    $resources = @(Get-SourceEntries 'ResourceCompile' $resource $document $properties $directory)
    $references = @()
    foreach ($reference in $document.SelectNodes("//*[local-name()='ProjectReference'][@Include]")) {
        if (-not (Test-Selection $reference)) { continue }
        $referenceSettings = @{}
        foreach ($setting in $reference.ChildNodes) {
            if ($setting.NodeType -eq 'Element' -and (Test-Selection $setting)) { $referenceSettings[$setting.LocalName] = $setting.InnerText }
        }
        $references += [ordered]@{
            projectFile = Resolve-ProjectPath (Expand-ProjectValue $reference.GetAttribute('Include') $properties) $directory
            linkLibraryDependencies = ($referenceSettings.LinkLibraryDependencies -ne 'false')
            useLibraryDependencyInputs = ($referenceSettings.UseLibraryDependencyInputs -eq 'true')
        }
    }
    $compileSettings = $compile.Clone()
    $compileSettings.Remove('PreprocessorDefinitions'); $compileSettings.Remove('AdditionalIncludeDirectories')
    $resourceDefines = @(Get-Items ([string]$resource.PreprocessorDefinitions) $properties)
    if ($properties.CharacterSet -eq 'Unicode') { $resourceDefines += 'UNICODE', '_UNICODE' }
    $targetName = if ($properties.TargetName) { $properties.TargetName } else { $name }
    $targetExtension = if ($properties.TargetExt) { $properties.TargetExt }
        elseif ($properties.ConfigurationType -eq 'StaticLibrary') { '.lib' }
        elseif ($properties.ConfigurationType -eq 'DynamicLibrary') { '.dll' }
        else { '.exe' }
    $targetDirectory = if ($properties.OutDir) { Resolve-ProjectPath $properties.OutDir $directory }
        else { Join-Path $ProjectRoot 'build' }
    $projects += [ordered]@{
        name = $name; projectFile = $projectPath; directory = $directory
        kind = $properties.ConfigurationType
        windowsSdkVersion = $properties.WindowsTargetPlatformVersion
        msvcToolsVersion = $properties.VCToolsVersion
        targetName = $targetName; targetExtension = $targetExtension
        targetPath = Join-Path $targetDirectory ($targetName + $targetExtension)
        sources = $sources; includeDirectories = $includes; defines = @($defines | Select-Object -Unique)
        compileSettings = $compileSettings; projectReferences = $references
        resources = $resources; resourceDefines = @($resourceDefines | Select-Object -Unique)
        resourceIncludeDirectories = @(Get-Items ([string]$resource.AdditionalIncludeDirectories) $properties |
            ForEach-Object { Resolve-ProjectPath $_ $directory })
        resourceSettings = @{ Culture = $resource.Culture; CodePage = $resource.CodePage }
        subsystem = $link.SubSystem; entryPoint = $link.EntryPointSymbol
        linkLibraries = @(Get-Items ([string]$link.AdditionalDependencies) $properties |
            ForEach-Object { if ($_ -match '[/\\]') { Resolve-ProjectPath $_ $directory } else { $_ } })
        libraryDirectories = @(Get-Items ([string]$link.AdditionalLibraryDirectories) $properties |
            ForEach-Object { Resolve-ProjectPath $_ $directory })
    }
}
if (-not $projects.Count) { throw "No C++ projects in $solution" }
$manifest = [ordered]@{
    schemaVersion = 2; generator = 'CPrime'; projectRoot = $ProjectRoot
    configuration = $Configuration; platform = $Platform; projects = $projects
}
$output = if ($OutputDirectory) { [IO.Path]::GetFullPath($OutputDirectory) } else { Join-Path $ProjectRoot 'build\manifest' }
New-Item -ItemType Directory -Force -Path $output | Out-Null
$destination = Join-Path $output "$Configuration-$Platform.json"
$temporary = $destination + '.tmp'
$manifest | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $temporary -Encoding UTF8
Move-Item -LiteralPath $temporary -Destination $destination -Force
Write-Host "Generated build manifest: $destination"
