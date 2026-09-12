param(
    [string]$ProjectRoot = (Get-Location).Path,
    [string]$Configuration = 'Release',
    [string]$Platform = 'x64',
    [Parameter(Mandatory = $true)][string]$SolutionPath,
    [string]$OutputDirectory = '',
    # Restrict the manifest to one application and its ProjectReference
    # closure. Solutions that also contain tests or tools export a manifest
    # the native driver can consume without extra solution files.
    [string]$OnlyTarget = '',
    # Extra include directories prepended to every exported project. CPC
    # builds use this for the compiler's own SDK tree, which is not part of
    # the project being exported.
    [string[]]$ExtraIncludeDirectory = @()
)

$ErrorActionPreference = 'Stop'
$ProjectRoot = [IO.Path]::GetFullPath($ProjectRoot)
$solution = if ([IO.Path]::IsPathRooted($SolutionPath)) { [IO.Path]::GetFullPath($SolutionPath) } else { [IO.Path]::GetFullPath((Join-Path $ProjectRoot $SolutionPath)) }
$selection = "$Configuration|$Platform"

# Conditions are evaluated with MSBuild semantics: properties expand first,
# unknown properties become the empty string, and comparisons are
# case-insensitive. Anything outside the supported grammar is rejected so an
# unfamiliar project cannot silently export a different build graph.
$script:conditionProperties = @{}
$script:conditionDirectory = $ProjectRoot

function Expand-ConditionText([string]$Text, [hashtable]$Properties) {
    if (-not $Text.Contains('$(')) { return $Text }
    return [regex]::Replace($Text, '\$\(([^)]+)\)', {
        param($match)
        $key = $match.Groups[1].Value
        if ($Properties.ContainsKey($key)) { return [string]$Properties[$key] }
        return ''
    })
}

function Get-ConditionTokens([string]$Text) {
    $tokens = New-Object System.Collections.ArrayList
    $i = 0
    $length = $Text.Length
    while ($i -lt $length) {
        $ch = $Text[$i]
        if ([char]::IsWhiteSpace($ch)) { $i++; continue }
        if ($ch -eq "'" -or $ch -eq '"') {
            $quote = $ch
            $j = $i + 1
            $builder = New-Object System.Text.StringBuilder
            $closed = $false
            while ($j -lt $length) {
                if ($Text[$j] -eq $quote) {
                    if ($j + 1 -lt $length -and $Text[$j + 1] -eq $quote) {
                        [void]$builder.Append($quote)
                        $j += 2
                        continue
                    }
                    $closed = $true
                    break
                }
                [void]$builder.Append($Text[$j])
                $j++
            }
            if (-not $closed) { throw "Unterminated string in condition: $Text" }
            [void]$tokens.Add([pscustomobject]@{ Kind = 'string'; Text = $builder.ToString() })
            $i = $j + 1
            continue
        }
        if ($ch -eq '=' -and $i + 1 -lt $length -and $Text[$i + 1] -eq '=') {
            [void]$tokens.Add([pscustomobject]@{ Kind = 'operator'; Text = '==' })
            $i += 2
            continue
        }
        if ($ch -eq '!' -and $i + 1 -lt $length -and $Text[$i + 1] -eq '=') {
            [void]$tokens.Add([pscustomobject]@{ Kind = 'operator'; Text = '!=' })
            $i += 2
            continue
        }
        if ($ch -eq '(') { [void]$tokens.Add([pscustomobject]@{ Kind = 'lparen' }); $i++; continue }
        if ($ch -eq ')') { [void]$tokens.Add([pscustomobject]@{ Kind = 'rparen' }); $i++; continue }
        if ($ch -eq ',') { [void]$tokens.Add([pscustomobject]@{ Kind = 'comma' }); $i++; continue }
        if ($ch -eq '!') { [void]$tokens.Add([pscustomobject]@{ Kind = 'bang' }); $i++; continue }
        $j = $i
        while ($j -lt $length) {
            $c = $Text[$j]
            if ([char]::IsWhiteSpace($c)) { break }
            if ($c -eq "'" -or $c -eq '"' -or $c -eq '(' -or $c -eq ')' -or $c -eq ',' -or $c -eq '!' -or $c -eq '=') { break }
            $j++
        }
        if ($j -eq $i) { throw "Unexpected character '$ch' in condition: $Text" }
        [void]$tokens.Add([pscustomobject]@{ Kind = 'bare'; Text = $Text.Substring($i, $j - $i) })
        $i = $j
    }
    return $tokens
}

function ConvertTo-ConditionText($Value) {
    if ($Value -is [bool]) { if ($Value) { return 'true' } else { return 'false' } }
    return [string]$Value
}

function ConvertTo-ConditionBool($Value, [string]$Text) {
    if ($Value -is [bool]) { return $Value }
    $string = [string]$Value
    if ($string -ieq 'true') { return $true }
    if ($string -ieq 'false') { return $false }
    throw "Condition value '$string' is not boolean: $Text"
}

function Read-ConditionOperand($Tokens, [ref]$Index, [string]$Text, [string]$Directory) {
    if ($Index.Value -ge $Tokens.Count) { throw "Unexpected end of condition: $Text" }
    $token = $Tokens[$Index.Value]
    if ($token.Kind -eq 'bare' -and $Index.Value + 1 -lt $Tokens.Count -and $Tokens[$Index.Value + 1].Kind -eq 'lparen') {
        $name = $token.Text
        $Index.Value += 2
        $arguments = New-Object System.Collections.ArrayList
        if ($Tokens[$Index.Value].Kind -ne 'rparen') {
            while ($true) {
                $argument = Read-ConditionOperand $Tokens $Index $Text $Directory
                if ($argument -is [bool]) { throw "Condition function argument must be a string: $Text" }
                [void]$arguments.Add([string]$argument)
                if ($Tokens[$Index.Value].Kind -eq 'comma') { $Index.Value++; continue }
                break
            }
        }
        if ($Tokens[$Index.Value].Kind -ne 'rparen') { throw "Expected ')' in condition: $Text" }
        $Index.Value++
        if ($arguments.Count -ne 1) { throw "Condition function $name expects one argument: $Text" }
        $path = [string]$arguments[0]
        if ($name -ieq 'Exists') {
            if (-not [IO.Path]::IsPathRooted($path)) { $path = [IO.Path]::GetFullPath([IO.Path]::Combine($Directory, $path)) }
            return ([IO.File]::Exists($path) -or [IO.Directory]::Exists($path))
        }
        if ($name -ieq 'HasTrailingSlash') { return ($path.EndsWith('\') -or $path.EndsWith('/')) }
        throw "Unsupported condition function: $name"
    }
    $Index.Value++
    if ($token.Kind -eq 'string') { return $token.Text }
    if ($token.Kind -eq 'bare') {
        if ($token.Text -ieq 'true') { return $true }
        if ($token.Text -ieq 'false') { return $false }
        throw "Unsupported condition token '$($token.Text)': $Text"
    }
    throw "Unexpected token in condition: $Text"
}

function Test-ConditionFactor($Tokens, [ref]$Index, [string]$Text, [string]$Directory) {
    if ($Index.Value -ge $Tokens.Count) { throw "Unexpected end of condition: $Text" }
    $token = $Tokens[$Index.Value]
    if ($token.Kind -eq 'bang') {
        $Index.Value++
        return -not (ConvertTo-ConditionBool (Test-ConditionFactor $Tokens $Index $Text $Directory) $Text)
    }
    if ($token.Kind -eq 'lparen') {
        $Index.Value++
        $value = Test-ConditionOr $Tokens $Index $Text $Directory
        if ($Index.Value -ge $Tokens.Count -or $Tokens[$Index.Value].Kind -ne 'rparen') { throw "Expected ')' in condition: $Text" }
        $Index.Value++
        return $value
    }
    $left = Read-ConditionOperand $Tokens $Index $Text $Directory
    if ($Index.Value -lt $Tokens.Count -and $Tokens[$Index.Value].Kind -eq 'operator') {
        $operator = $Tokens[$Index.Value].Text
        $Index.Value++
        $right = Read-ConditionOperand $Tokens $Index $Text $Directory
        $equal = (ConvertTo-ConditionText $left) -ieq (ConvertTo-ConditionText $right)
        if ($operator -eq '==') { return $equal }
        return (-not $equal)
    }
    return $left
}

function Test-ConditionAnd($Tokens, [ref]$Index, [string]$Text, [string]$Directory) {
    $value = Test-ConditionFactor $Tokens $Index $Text $Directory
    while ($Index.Value -lt $Tokens.Count -and $Tokens[$Index.Value].Kind -eq 'bare' -and $Tokens[$Index.Value].Text -ieq 'and') {
        $Index.Value++
        $next = Test-ConditionFactor $Tokens $Index $Text $Directory
        $value = ((ConvertTo-ConditionBool $value $Text) -and (ConvertTo-ConditionBool $next $Text))
    }
    return $value
}

function Test-ConditionOr($Tokens, [ref]$Index, [string]$Text, [string]$Directory) {
    $value = Test-ConditionAnd $Tokens $Index $Text $Directory
    while ($Index.Value -lt $Tokens.Count -and $Tokens[$Index.Value].Kind -eq 'bare' -and $Tokens[$Index.Value].Text -ieq 'or') {
        $Index.Value++
        $next = Test-ConditionAnd $Tokens $Index $Text $Directory
        $value = ((ConvertTo-ConditionBool $value $Text) -or (ConvertTo-ConditionBool $next $Text))
    }
    return $value
}

function Test-Condition([string]$Condition, [hashtable]$Properties, [string]$Directory) {
    $expanded = Expand-ConditionText $Condition $Properties
    $tokens = Get-ConditionTokens $expanded
    if (-not $tokens.Count) { throw "Empty condition: $Condition" }
    $index = 0
    $value = Test-ConditionOr $tokens ([ref]$index) $expanded $Directory
    if ($index -ne $tokens.Count) { throw "Unsupported condition expression: $Condition" }
    return ConvertTo-ConditionBool $value $expanded
}

function Test-Selection($Node) {
    # XPath returns these nodes in document order, so excluded parents still
    # suppress their children before any child condition is interpreted.
    foreach ($ancestor in $Node.SelectNodes('ancestor-or-self::*[@Condition]')) {
        $condition = $ancestor.GetAttribute('Condition')
        if (-not $condition) { continue }
        if (-not (Test-Condition $condition $script:conditionProperties $script:conditionDirectory)) { return $false }
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
$skippedProjects = @{}
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
    $script:conditionProperties = $properties
    $script:conditionDirectory = $directory
    $available = @($document.SelectNodes("//*[local-name()='ProjectConfiguration']") |
        ForEach-Object { $_.GetAttribute('Include') })
    if ($available -notcontains $selection) {
        # With a named target the solution may carry projects (tests, tools)
        # outside the requested configuration. They are only an error if the
        # target actually references them.
        if ($OnlyTarget) { $skippedProjects[$projectPath] = $name; continue }
        throw "$name has no $selection configuration"
    }
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
        ForEach-Object { Resolve-ProjectPath $_ $directory }) +
        @($ExtraIncludeDirectory | ForEach-Object { [IO.Path]::GetFullPath($_) })
    $includes = @($includes | Select-Object -Unique)
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
            ForEach-Object { Resolve-ProjectPath $_ $directory }) +
            @($ExtraIncludeDirectory | ForEach-Object { [IO.Path]::GetFullPath($_) })
        resourceSettings = @{ Culture = $resource.Culture; CodePage = $resource.CodePage }
        subsystem = $link.SubSystem; entryPoint = $link.EntryPointSymbol
        linkLibraries = @(Get-Items ([string]$link.AdditionalDependencies) $properties |
            ForEach-Object { if ($_ -match '[/\\]') { Resolve-ProjectPath $_ $directory } else { $_ } })
        libraryDirectories = @(Get-Items ([string]$link.AdditionalLibraryDirectories) $properties |
            ForEach-Object { Resolve-ProjectPath $_ $directory })
    }
}
if (-not $projects.Count) { throw "No C++ projects in $solution" }
if ($OnlyTarget) {
    $byName = @{}
    $byFile = @{}
    foreach ($project in $projects) { $byName[$project['name']] = $project; $byFile[$project['projectFile']] = $project }
    if (-not $byName.ContainsKey($OnlyTarget)) { throw "Target project '$OnlyTarget' is not in $solution" }
    $keep = New-Object 'System.Collections.Generic.HashSet[string]'
    $pending = New-Object 'System.Collections.Generic.Queue[object]'
    $pending.Enqueue($byName[$OnlyTarget])
    while ($pending.Count -gt 0) {
        $project = $pending.Dequeue()
        if (-not $keep.Add([string]$project['projectFile'])) { continue }
        foreach ($reference in $project['projectReferences']) {
            $referenced = [string]$reference['projectFile']
            if ($byFile.ContainsKey($referenced)) { $pending.Enqueue($byFile[$referenced]) }
            elseif ($skippedProjects.ContainsKey($referenced)) { throw "$($skippedProjects[$referenced]) has no $selection configuration but is required by $OnlyTarget" }
        }
    }
    $projects = @($projects | Where-Object { $keep.Contains([string]$_['projectFile']) })
}
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
