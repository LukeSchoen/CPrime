param(
    [ValidateSet('Prime', 'Clang')][string]$Toolchain = 'Prime',
    [Alias('CpcPath')][string]$CompilerPath = '',
    [string]$ProjectRoot = (Get-Location).Path,
    [string]$ManifestPath = '',
    [string]$OutDir = '',
    [string]$ExePath = '',
    [ValidateRange(1, 64)][int]$Jobs = $(if ($Toolchain -eq 'Prime') { 1 } else { [Environment]::ProcessorCount }),
    [ValidateRange(1, 3600)][int]$CompileTimeoutSeconds = 60,
    [switch]$Unity,
    [switch]$SkipLink,
    [switch]$AllowWarnings
)

$ErrorActionPreference = 'Stop'
if ($Toolchain -eq 'Prime' -and $Jobs -ne 1) {
    throw 'Cprime builds require -Jobs 1. Optimize compiler work instead of enabling parallel compilation.'
}
$buildTimer = [Diagnostics.Stopwatch]::StartNew()
$ProjectRoot = [IO.Path]::GetFullPath($ProjectRoot)
if (-not $CompilerPath) {
    $CompilerPath = if ($Toolchain -eq 'Prime') { Join-Path $PSScriptRoot 'cpc.exe' }
        else { Join-Path $ProjectRoot 'CommonLib\Assets\Programs\Clang\clang.exe' }
}
$CompilerPath = [IO.Path]::GetFullPath($CompilerPath)
if (-not $ManifestPath) { $ManifestPath = Join-Path $ProjectRoot 'builds\manifest\Release-x64.json' }
if (-not $OutDir) { $OutDir = Join-Path $ProjectRoot ('builds\' + $Toolchain.ToLowerInvariant()) }
$OutDir = [IO.Path]::GetFullPath($OutDir)
if (-not (Test-Path -LiteralPath $CompilerPath -PathType Leaf)) { throw "Compiler not found: $CompilerPath" }
if (-not (Test-Path -LiteralPath $ManifestPath -PathType Leaf)) {
    throw "CodeClip manifest not found: $ManifestPath. Run CodeClip.exe to regenerate the selected project."
}
$manifest = Get-Content -Raw -LiteralPath $ManifestPath | ConvertFrom-Json
if ($manifest.schemaVersion -notin @(1, 2) -or $manifest.platform -ne 'x64') { throw 'Unsupported build manifest schema or platform' }
$applications = @($manifest.projects | Where-Object { $_.kind -eq 'Application' })
if ($applications.Count -ne 1) { throw 'The manifest must select one application project.' }
if (-not $ExePath) {
    $ExePath = if ($manifest.schemaVersion -ge 2) { $applications[0].targetPath }
        else { Join-Path $ProjectRoot ('builds\' + $applications[0].targetName + '.exe') }
}
$ExePath = [IO.Path]::GetFullPath($ExePath)
New-Item -ItemType Directory -Force -Path $OutDir, (Split-Path $ExePath -Parent) | Out-Null
if (-not $SkipLink -and (Test-Path -LiteralPath $ExePath -PathType Leaf)) { Remove-Item -LiteralPath $ExePath -Force }

function Quote-Native([string]$Argument) {
    # CommandLineToArgvW quoting, including spaces, embedded quotes, and a
    # trailing backslash. Quoting is applied to complete arguments.
    return '"' + [regex]::Replace([regex]::Replace($Argument, '(\\*)"', '$1$1\"'), '(\\+)$', '$1$1') + '"'
}

function Start-BuildTool([string]$ToolPath, [string[]]$Arguments, [string]$Directory) {
    $start = [Diagnostics.ProcessStartInfo]::new()
    $start.FileName = $ToolPath
    $start.Arguments = ($Arguments | ForEach-Object { Quote-Native $_ }) -join ' '
    $start.WorkingDirectory = $Directory
    $start.UseShellExecute = $false
    $start.CreateNoWindow = $true
    $start.RedirectStandardOutput = $true
    $start.RedirectStandardError = $true
    $process = [Diagnostics.Process]::new()
    $process.StartInfo = $start
    [void]$process.Start()
    # Own the process handle from Start(), even when a small translation unit
    # finishes before a Start-Process cmdlet could return it to its caller.
    return @{
        Process = $process
        Output = $process.StandardOutput.ReadToEndAsync()
        Error = $process.StandardError.ReadToEndAsync()
    }
}

function Invoke-BuildTool([string]$ToolPath, [string[]]$Arguments, [string]$Directory, [string]$Label) {
    $started = Start-BuildTool $ToolPath $Arguments $Directory
    try {
        $timedOut = -not $started.Process.WaitForExit($CompileTimeoutSeconds * 1000)
        if ($timedOut) { $started.Process.Kill(); $started.Process.WaitForExit() }
        $stdout = $started.Output.GetAwaiter().GetResult()
        $stderr = $started.Error.GetAwaiter().GetResult()
        $stdout | Set-Content -LiteralPath (Join-Path $OutDir ($Label + '.log'))
        $stderr | Set-Content -LiteralPath (Join-Path $OutDir ($Label + '.err.log'))
        if ($timedOut -or $started.Process.ExitCode -ne 0) {
            Write-Host $stdout
            Write-Host $stderr
            throw "$Label failed$(if ($timedOut) { ' (timed out)' })"
        }
    } finally { $started.Process.Dispose() }
}

function Find-MsvcToolchain([string]$RequestedVersion, [switch]$Required) {
    if (-not $RequestedVersion) { $RequestedVersion = [string]$env:VCToolsVersion }
    $RequestedVersion = $RequestedVersion.TrimEnd('\', '/')
    $roots = [Collections.Generic.List[string]]::new()
    if ($env:VCToolsInstallDir) { $roots.Add($env:VCToolsInstallDir.TrimEnd('\', '/')) }
    # A developer shell may publish tools on PATH without VCToolsInstallDir.
    $command = Get-Command 'cl.exe' -CommandType Application -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($command) {
        $directory = Split-Path $command.Source -Parent
        for ($level = 0; $level -lt 3 -and $directory; ++$level) { $directory = Split-Path $directory -Parent }
        if ($directory) { $roots.Add($directory) }
    }
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (Test-Path -LiteralPath $vswhere) {
        $installations = @(& $vswhere -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -sort -property installationPath)
        foreach ($installation in $installations) {
            $version = $RequestedVersion
            if (-not $version) {
                $versionFile = Join-Path $installation 'VC\Auxiliary\Build\Microsoft.VCToolsVersion.default.txt'
                if (-not (Test-Path -LiteralPath $versionFile -PathType Leaf)) { continue }
                $version = (Get-Content -LiteralPath $versionFile -Raw).Trim()
            }
            $roots.Add((Join-Path $installation "VC\Tools\MSVC\$version"))
        }
    }
    foreach ($candidate in $roots | Select-Object -Unique) {
        if ($RequestedVersion -and (Split-Path $candidate -Leaf) -ne $RequestedVersion) {
            $candidate = Join-Path (Split-Path $candidate -Parent) $RequestedVersion
        }
        $libraryDirectory = Join-Path $candidate 'lib\x64'
        if (-not (Test-Path -LiteralPath $libraryDirectory -PathType Container)) { continue }
        return @{
            Root = $candidate; Version = (Split-Path $candidate -Leaf)
            Tools = Join-Path $candidate 'bin\Hostx64\x64'; Libraries = @($libraryDirectory)
        }
    }
    if ($Required -or $RequestedVersion) { throw "Visual C++ tools $RequestedVersion with x64 libraries were not found." }
    return $null
}

function Find-MsvcTool([string]$Name) {
    $command = Get-Command $Name -CommandType Application -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($command) { return $command.Source }
    $toolchain = Find-MsvcToolchain
    if ($toolchain) {
        $tool = Join-Path $toolchain.Tools $Name
        if (Test-Path -LiteralPath $tool -PathType Leaf) { return $tool }
    }
    throw "Required build tool $Name was not found. Install the Visual C++ build tools or make the tool available on PATH."
}

function Find-WindowsSdk([string]$RequestedVersion, [switch]$Required) {
    $kitRoot = $env:WindowsSdkDir
    if (-not $kitRoot) {
        $kits = Get-ItemProperty 'HKLM:\SOFTWARE\Microsoft\Windows Kits\Installed Roots' -ErrorAction SilentlyContinue
        if ($kits) { $kitRoot = $kits.KitsRoot10 }
    }
    if (-not $kitRoot) { $kitRoot = Join-Path ${env:ProgramFiles(x86)} 'Windows Kits\10' }
    if (-not $RequestedVersion) { $RequestedVersion = [string]$env:WindowsSDKVersion }
    $RequestedVersion = $RequestedVersion.TrimEnd('\', '/')
    $versions = @(Get-ChildItem -LiteralPath (Join-Path $kitRoot 'Lib') -Directory -ErrorAction SilentlyContinue |
        Where-Object { $_.Name -match '^\d+\.\d+\.\d+\.\d+$' } | Sort-Object { [version]$_.Name } -Descending)
    foreach ($version in $versions) {
        # MSBuild's 10.0 selection means the latest installed Windows 10 SDK.
        if ($RequestedVersion -and $RequestedVersion -ne '10.0' -and $version.Name -ne $RequestedVersion) { continue }
        $um = Join-Path $version.FullName 'um\x64'
        $ucrt = Join-Path $version.FullName 'ucrt\x64'
        if (-not (Test-Path -LiteralPath $um -PathType Container) -or
            -not (Test-Path -LiteralPath $ucrt -PathType Container)) { continue }
        return @{
            Version = $version.Name
            Compiler = Join-Path $kitRoot "bin\$($version.Name)\x64\rc.exe"
            Includes = @((Join-Path $kitRoot "Include\$($version.Name)\um"), (Join-Path $kitRoot "Include\$($version.Name)\shared"))
            Libraries = @($um, $ucrt)
        }
    }
    if ($Required -or $RequestedVersion) { throw "Windows SDK $RequestedVersion with x64 libraries was not found." }
    return $null
}

function Find-ResourceTools($Sdk) {
    if (-not $Sdk -or -not (Test-Path -LiteralPath $Sdk.Compiler -PathType Leaf)) {
        throw 'The selected Windows SDK resource compiler rc.exe was not found.'
    }
    return @{ Compiler = $Sdk.Compiler; Converter = (Find-MsvcTool 'cvtres.exe'); Includes = $Sdk.Includes }
}

function Get-EffectiveSettings($ProjectSettings, $SourceSettings) {
    $settings = @{}
    foreach ($group in @($ProjectSettings, $SourceSettings)) {
        if ($null -eq $group) { continue }
        foreach ($property in $group.PSObject.Properties) { $settings[$property.Name] = $property.Value }
    }
    return $settings
}

function Get-CompileFlags($Project, $Source) {
    $settings = Get-EffectiveSettings $Project.compileSettings $Source.settings
    if ($Toolchain -eq 'Clang') {
        if ([IO.Path]::GetExtension($Source.path) -ne '.c') {
            $standard = switch ($settings.LanguageStandard) {
                'stdcpp14' { 'c++14' }; 'stdcpp17' { 'c++17' }; 'stdcpp20' { 'c++20' }; 'stdcpplatest' { 'c++2b' }
                default { 'c++17' }
            }
            '-std=' + $standard
        }
        '-fms-extensions'
        if ($settings.BufferSecurityCheck -eq 'false') { '-fno-stack-protector' }
    }
    switch ($settings.Optimization) {
        'Disabled' { '-O0' }; 'MinSpace' { '-Os' }; 'MaxSpeed' { '-O2' }; 'Full' { '-O2' }
    }
    if ($settings.TreatWarningAsError -eq 'true' -and -not $AllowWarnings) { '-Werror' }
    foreach ($undefine in ([string]$settings.UndefinePreprocessorDefinitions).Split(';')) { if ($undefine) { '-U' + $undefine } }
    foreach ($include in ([string]$settings.ForcedIncludeFiles).Split(';')) {
        if ($include) {
            '-include'
            if ([IO.Path]::IsPathRooted($include)) { [IO.Path]::GetFullPath($include) }
            else { [IO.Path]::GetFullPath((Join-Path $Project.directory $include)) }
        }
    }
    # Version 2 records effective per-source overrides after MSBuild
    # inheritance/replacement; absent overrides use the project defaults.
    # Version 1 recorded only per-source additions.
    if ($manifest.schemaVersion -eq 1) {
        foreach ($include in $Project.includeDirectories) { '-I' + $include }
        foreach ($define in $Project.defines) { '-D' + $define }
        foreach ($include in $Source.includeDirectories) { '-I' + $include }
        foreach ($define in $Source.defines) { '-D' + $define }
    } else {
        $includes = if ($Source.PSObject.Properties['includeDirectories']) { $Source.includeDirectories } else { $Project.includeDirectories }
        $defines = if ($Source.PSObject.Properties['defines']) { $Source.defines } else { $Project.defines }
        foreach ($include in $includes) { '-I' + $include }
        foreach ($define in $defines) { '-D' + $define }
    }
}

$jobsToRun = @()
$objects = @()
$projectBuilds = @()
$projectsByPath = @{}
$index = 0
foreach ($project in $manifest.projects) {
    # The generated graph is authoritative. No exclusions, renamed sources,
    # application macros, or replacement implementations belong here.
    $groups = [ordered]@{}
    $projectBuild = @{
        Project = $project; Objects = [Collections.ArrayList]::new(); Resources = [Collections.ArrayList]::new()
        Label = 'project_{0:d4}' -f $projectBuilds.Count
    }
    $projectBuilds += $projectBuild
    if ($project.projectFile) { $projectsByPath[$project.projectFile] = $projectBuild }
    foreach ($source in $project.sources) {
        if (-not (Test-Path -LiteralPath $source.path -PathType Leaf)) { throw "Selected source is missing: $($source.path)" }
        $flags = @(Get-CompileFlags $project $source)
        $key = if ($Unity -and [IO.Path]::GetExtension($source.path) -in @('.cpp', '.cxx', '.cc')) {
            $unityGroup = if ($source.PSObject.Properties['unityGroup']) { [string]$source.unityGroup } else { '' }
            ($flags -join "`n") + "`nunity-group:" + $unityGroup
        } else { 'single:' + $index }
        if (-not $groups.Contains($key)) { $groups[$key] = [Collections.ArrayList]::new() }
        [void]$groups[$key].Add(@{ Source = $source.path; Flags = $flags; Index = $index++ })
    }
    foreach ($group in $groups.Values) {
        # Larger serial Prime units reuse guarded headers and template state.
        # Keep grouping restricted to identical effective compiler settings.
        $chunkSize = if ($Unity -and $group.Count -gt 1) {
            if ($Toolchain -eq 'Prime') { 32 } else { 4 }
        } else { 1 }
        for ($start = 0; $start -lt $group.Count; $start += $chunkSize) {
            $end = [Math]::Min($start + $chunkSize, $group.Count) - 1
            $chunk = @($group[$start..$end])
            $label = 'source_{0:d4}' -f $chunk[0].Index
            $inputSource = $chunk[0].Source
            if ($chunk.Count -gt 1) {
                $inputSource = Join-Path $OutDir ($label + '.cpp')
                $chunk | ForEach-Object { '#include "' + $_.Source.Replace('\', '/') + '"' } |
                    Set-Content -LiteralPath $inputSource -Encoding UTF8
            }
            $object = Join-Path $OutDir ($label + '.obj')
            if (Test-Path -LiteralPath $object) { Remove-Item -LiteralPath $object -Force }
            $objects += $object
            [void]$projectBuild.Objects.Add($object)
            $jobsToRun += @{
                Label = $label; Source = $inputSource; Object = $object; Directory = $project.directory
                Flags = $chunk[0].Flags; Inputs = @($chunk | ForEach-Object { $_.Source })
            }
        }
    }
}
if (-not $jobsToRun.Count) { throw 'The manifest contains no compile sources.' }
$jobsToRun | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $OutDir 'compile_inputs.json') -Encoding UTF8
$active = [Collections.ArrayList]::new()
$script:failed = $false
$timer = [Diagnostics.Stopwatch]::StartNew()

function Complete-Compiles([switch]$Drain) {
    do {
        foreach ($job in @($active)) {
            $timedOut = -not $job.Process.HasExited -and $job.Timer.Elapsed.TotalSeconds -ge $CompileTimeoutSeconds
            if (-not $job.Process.HasExited -and -not $timedOut) { continue }
            if ($timedOut) { $job.Process.Kill() }
            $job.Process.WaitForExit()
            $job.Process.Refresh()
            $code = if ($timedOut) { -999 } else { $job.Process.ExitCode }
            $diagnostics = $job.Error.GetAwaiter().GetResult()
            $job.Output.GetAwaiter().GetResult() | Set-Content -LiteralPath (Join-Path $OutDir ($job.Label + '.log'))
            $diagnostics | Set-Content -LiteralPath $job.ErrorLog
            if ($null -eq $diagnostics) { $diagnostics = '' }
            $warnings = [regex]::Matches($diagnostics, 'warning:').Count
            Write-Host ('{0} {1,7:n3}s exit={2} warnings={3} {4}' -f $job.Label, $job.Timer.Elapsed.TotalSeconds, $code, $warnings, $job.Inputs[0])
            if ($code -ne 0 -or -not (Test-Path -LiteralPath $job.Object -PathType Leaf)) {
                $script:failed = $true
                if ($timedOut) { Write-Host "Compile timed out after ${CompileTimeoutSeconds}s" }
                ($diagnostics -split "`n" | Where-Object { $_ -match 'error:' } | Select-Object -First 3) | Write-Host
            }
            $job.Process.Dispose()
            [void]$active.Remove($job)
        }
        if ($active.Count -and ($Drain -or $active.Count -ge $Jobs)) { Start-Sleep -Milliseconds 10 }
    } while ($active.Count -and ($Drain -or $active.Count -ge $Jobs))
}

foreach ($job in $jobsToRun) {
    Complete-Compiles
    $arguments = @($job.Flags) + @('-c', $job.Source, '-o', $job.Object)
    $job.ErrorLog = Join-Path $OutDir ($job.Label + '.err.log')
    $job.Timer = [Diagnostics.Stopwatch]::StartNew()
    $started = Start-BuildTool $CompilerPath $arguments $job.Directory
    $job.Process = $started.Process
    $job.Output = $started.Output
    $job.Error = $started.Error
    [void]$active.Add($job)
}
Complete-Compiles -Drain
Write-Host ('Compile elapsed: {0:n3}s; {1} source files' -f $timer.Elapsed.TotalSeconds, $index)
if ($script:failed) { Write-Host 'Executable: NOT PRODUCED'; exit 1 }
if ($SkipLink) { exit 0 }

$linkedBuilds = @($projectBuilds)
$linkedResourceObject = $null
if ($manifest.schemaVersion -ge 2) {
    $linked = [ordered]@{}
    function Visit-ProjectReferences($Build, [string[]]$Ancestors) {
        $key = $Build.Project.projectFile
        if ($Ancestors -contains $key) { throw "Project reference cycle involving $key" }
        if ($linked.Contains($key)) { return }
        $linked[$key] = $Build
        foreach ($reference in $Build.Project.projectReferences) {
            if (-not $projectsByPath.ContainsKey($reference.projectFile)) { throw "Referenced project is missing from the manifest: $($reference.projectFile)" }
            if (-not $reference.linkLibraryDependencies) { continue }
            $dependency = $projectsByPath[$reference.projectFile]
            if ($reference.useLibraryDependencyInputs) { $dependency.UseObjects = $true }
            Visit-ProjectReferences $dependency (@($Ancestors) + @($key))
        }
    }
    $appBuild = $projectsByPath[$applications[0].projectFile]
    Visit-ProjectReferences $appBuild @()
    # A dependency archive must follow every selected project which uses it.
    # Keep declaration order when several projects are ready at the same time.
    $linkedBuilds = @()
    $remaining = @($linked.Values)
    while ($remaining.Count) {
        $ready = @($remaining | Where-Object {
            $candidate = $_.Project.projectFile
            -not @($remaining | Where-Object { @($_.Project.projectReferences | Where-Object {
                $_.linkLibraryDependencies -and $_.projectFile -eq $candidate
            }).Count }).Count
        })
        if (-not $ready.Count) { throw 'Project dependency graph could not be ordered' }
        foreach ($build in $ready) { $linkedBuilds += $build }
        $remaining = @($remaining | Where-Object { $ready -notcontains $_ })
    }

    $resourceTools = $null
    foreach ($build in $projectBuilds) {
        $project = $build.Project
        $sdk = Find-WindowsSdk $project.windowsSdkVersion -Required:([bool]$project.resources.Count)
        $build.Sdk = $sdk
        $build.Msvc = Find-MsvcToolchain $project.msvcToolsVersion
        if ($project.resources.Count) { $resourceTools = Find-ResourceTools $sdk }
        if ($project.kind -notin @('Application', 'StaticLibrary')) { throw "Unsupported project kind: $($project.kind)" }
        foreach ($resource in $project.resources) {
            $label = $build.Label + '_resource_' + $build.Resources.Count
            $resFile = Join-Path $OutDir ($label + '.res')
            $resourceArgs = @('/nologo', '/fo', $resFile)
            $includes = if ($resource.PSObject.Properties['includeDirectories']) { $resource.includeDirectories } else { $project.resourceIncludeDirectories }
            $defines = if ($resource.PSObject.Properties['defines']) { $resource.defines } else { $project.resourceDefines }
            foreach ($include in @($includes) + @($resourceTools.Includes)) { if ($include) { $resourceArgs += '/I', $include } }
            foreach ($define in $defines) { $resourceArgs += '/D', $define }
            $settings = Get-EffectiveSettings $project.resourceSettings $resource.settings
            if ($settings.Culture) { $resourceArgs += '/l', $settings.Culture }
            if ($settings.CodePage) { $resourceArgs += '/c', $settings.CodePage }
            $resourceArgs += $resource.path
            Invoke-BuildTool $resourceTools.Compiler $resourceArgs $project.directory $label
            [void]$build.Resources.Add($resFile)
        }
        $archiveObjects = @($build.Objects)
        if ($build.Resources.Count -and $project.kind -eq 'StaticLibrary') {
            $resourceObject = Join-Path $OutDir ($build.Label + '_resources.obj')
            Invoke-BuildTool $resourceTools.Converter (@('/NOLOGO', '/MACHINE:X64', ('/OUT:' + $resourceObject)) + @($build.Resources)) $project.directory ($build.Label + '_resources')
            if ($Toolchain -eq 'Prime') {
                $archiveResource = Join-Path $OutDir ($build.Label + '_resources.o')
                Invoke-BuildTool $CompilerPath @('-r', $resourceObject, '-o', $archiveResource) $project.directory ($build.Label + '_resource_object')
                $resourceObject = $archiveResource
            }
            $archiveObjects += $resourceObject
        }
        if ($project.kind -eq 'StaticLibrary') {
            $build.Archive = Join-Path $OutDir ($build.Label + '_' + $project.targetName + '.lib')
            if (Test-Path -LiteralPath $build.Archive) { Remove-Item -LiteralPath $build.Archive -Force }
            if ($Toolchain -eq 'Prime') {
                Invoke-BuildTool $CompilerPath (@('-ar', 'rcs', $build.Archive) + $archiveObjects) $project.directory ($build.Label + '_archive')
            } else {
                Invoke-BuildTool (Find-MsvcTool 'lib.exe') (@('/NOLOGO', ('/OUT:' + $build.Archive)) + $archiveObjects) $project.directory ($build.Label + '_archive')
            }
            Write-Host "Library: $($build.Archive)"
        }
    }
    $linkedResources = @($linkedBuilds | Where-Object { $_.Project.kind -eq 'Application' -or $_.UseObjects } |
        ForEach-Object { $_.Resources })
    if ($linkedResources.Count) {
        # Resource directories must be merged by the resource tool. Merely
        # concatenating .rsrc sections loses resources from later projects.
        $linkedResourceObject = Join-Path $OutDir 'linked_resources.obj'
        Invoke-BuildTool $resourceTools.Converter (@('/NOLOGO', '/MACHINE:X64', ('/OUT:' + $linkedResourceObject)) + $linkedResources) $applications[0].directory 'linked_resources'
    }
}

$linkArguments = @()
foreach ($build in $linkedBuilds) {
    if ($build.Archive -and -not $build.UseObjects) { $linkArguments += $build.Archive }
    else { $linkArguments += @($build.Objects) }
}
if ($linkedResourceObject) { $linkArguments += $linkedResourceObject }
foreach ($directory in @($linkedBuilds | ForEach-Object { $_.Project.libraryDirectories } | Select-Object -Unique)) {
    $linkArguments += '-L' + $directory
}
# Native inputs retain their compiler's DEFAULTLIB directives. Resolve those
# against the selected MSVC toolset as well as the selected Windows SDK.
foreach ($directory in @($linkedBuilds | ForEach-Object { $_.Msvc.Libraries; $_.Sdk.Libraries } |
        Where-Object { $_ } | Select-Object -Unique)) {
    $linkArguments += '-L' + $directory
}
foreach ($library in @($linkedBuilds | ForEach-Object { $_.Project.linkLibraries } | Select-Object -Unique)) {
    if ($library -match '[/\\]') { $linkArguments += $library }
    elseif ($Toolchain -eq 'Prime') { $linkArguments += '-l' + [IO.Path]::GetFileNameWithoutExtension($library) }
    else { $linkArguments += '-Xlinker', $library }
}
if ($applications[0].subsystem) {
    if ($Toolchain -eq 'Prime') { $linkArguments += '-Wl,-subsystem=' + $applications[0].subsystem.ToLowerInvariant() }
    else { $linkArguments += '-Xlinker', ('/SUBSYSTEM:' + $applications[0].subsystem) }
}
if ($applications[0].entryPoint) {
    if ($Toolchain -eq 'Prime') { $linkArguments += '-Wl,-e=' + $applications[0].entryPoint }
    else { $linkArguments += '-Xlinker', ('/ENTRY:' + $applications[0].entryPoint) }
}
$linkArguments += '-o', $ExePath
@{ Compiler = $CompilerPath; Arguments = $linkArguments; Directory = $applications[0].directory } |
    ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $OutDir 'link_inputs.json') -Encoding UTF8
try {
    Invoke-BuildTool $CompilerPath $linkArguments $applications[0].directory 'link'
    if (-not (Test-Path -LiteralPath $ExePath -PathType Leaf)) { throw 'The linker did not produce the selected executable' }
} catch {
    if (Test-Path -LiteralPath $ExePath -PathType Leaf) { Remove-Item -LiteralPath $ExePath -Force }
    Write-Host 'Executable: NOT PRODUCED'
    throw
}
Write-Host "Executable: $ExePath"
Write-Host ('Build elapsed: {0:n3}s; {1} translation units; unity={2}' -f $buildTimer.Elapsed.TotalSeconds, $jobsToRun.Count, [bool]$Unity)
