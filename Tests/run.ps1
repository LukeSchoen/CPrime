param(
    [string]$Suite = "c_compat",
    [Alias("TccPath")]
    [string]$CompilerPath = "",
    [switch]$UseSharedBinaries,
    [string]$SharedOutDir = "",
    [switch]$RequireSharedHits,
    [string]$BuildManifestPath = $env:CPRIME_TEST_BUILD_MANIFEST
)

$ErrorActionPreference = "Stop"
if (Get-Variable -Name PSNativeCommandUseErrorActionPreference -Scope Global -ErrorAction SilentlyContinue) {
    $global:PSNativeCommandUseErrorActionPreference = $false
}

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$rootDir = Resolve-Path (Join-Path $scriptDir "..")
$script:testBuildManifest = $null

function Resolve-CompilerPath {
    param([string]$ExplicitPath)

    $candidates = @()
    if ($ExplicitPath) {
        $candidates += $ExplicitPath
    }

    $candidates += @(
        (Join-Path $rootDir "cpc.exe"),
        (Join-Path $rootDir "win32\\cpc.exe")
    )

    foreach ($candidate in $candidates) {
        if (-not $candidate) { continue }
        $resolved = Resolve-Path -LiteralPath $candidate -ErrorAction SilentlyContinue
        if ($resolved) {
            return $resolved.Path
        }
    }

    throw "Unable to find cpc.exe. Build first or pass -CompilerPath explicitly."
}

function Parse-Metadata {
    param([string]$FilePath)

    $meta = @{
        EXPECT_EXIT = "0"
        EXPECT_STDOUT = ""
        EXPECT_COMPILE_FAIL = "0"
        EXPECT_COMPILE_ARGS = ""
        EXPECT_COMPILE_ONLY = "0"
        EXPECT_SOURCES = ""
        EXPECT_MANIFEST_SOURCE = ""
    }

    foreach ($line in Get-Content -LiteralPath $FilePath -TotalCount 12) {
        if ($line -match '^\s*//\s*(EXPECT_[A-Z_]+)\s*:\s*(.*)$') {
            $meta[$matches[1]] = $matches[2].Trim()
        }
    }

    return $meta
}

function Normalize-LineEndings {
    param([string]$Text)
    return (($Text -replace "`r`n", "`n") -replace "`r", "`n")
}

function Split-CompilerArgs {
    param([string]$Text)
    $argument = New-Object Text.StringBuilder
    $quoted = $false
    $started = $false
    for ($i = 0; $i -lt $Text.Length; ++$i) {
        $character = $Text[$i]
        if ($character -eq '\' -and $i + 1 -lt $Text.Length -and
            ($Text[$i + 1] -eq '\' -or $Text[$i + 1] -eq '"')) {
            [void]$argument.Append($Text[++$i])
        } elseif ($character -eq '"') {
            $quoted = -not $quoted
        } elseif ([char]::IsWhiteSpace($character) -and -not $quoted) {
            if ($started) { $argument.ToString(); [void]$argument.Clear(); $started = $false }
            continue
        } else {
            [void]$argument.Append($character)
        }
        $started = $true
    }
    if ($quoted) { throw 'Unterminated quote in EXPECT_COMPILE_ARGS.' }
    if ($started) { $argument.ToString() }
}

function Resolve-TestSources {
    param([System.IO.FileInfo]$Test, [hashtable]$Metadata)

    $sources = @($Test.FullName)
    if ($Metadata.EXPECT_SOURCES) {
        $additional = ConvertFrom-Json -InputObject $Metadata.EXPECT_SOURCES
        if ($Metadata.EXPECT_SOURCES -notmatch '^\s*\[') {
            throw "$($Test.Name): EXPECT_SOURCES must be a JSON array of relative source paths."
        }
        foreach ($source in $additional) {
            if ($source -isnot [string] -or [IO.Path]::IsPathRooted($source)) {
                throw "$($Test.Name): EXPECT_SOURCES entries must be relative source paths."
            }
            $path = [IO.Path]::GetFullPath((Join-Path $Test.DirectoryName $source))
            if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
                throw "$($Test.Name): additional source is missing: $path"
            }
            $sources += $path
        }
    }
    return $sources
}

function Get-ManifestCompileContext {
    param([string]$SourcePath)

    if (-not $BuildManifestPath -or -not (Test-Path -LiteralPath $BuildManifestPath -PathType Leaf)) {
        throw 'This test requires a CodeClip manifest. Pass -BuildManifestPath or set CPRIME_TEST_BUILD_MANIFEST to the generated manifest JSON path.'
    }
    if (-not $script:testBuildManifest) {
        $script:testBuildManifest = Get-Content -Raw -LiteralPath $BuildManifestPath | ConvertFrom-Json
        if ($script:testBuildManifest.generator -ne 'CodeClip' -or
            $script:testBuildManifest.schemaVersion -notin @(1, 2) -or
            $script:testBuildManifest.platform -ne 'x64') {
            throw "Unsupported CodeClip manifest: $BuildManifestPath"
        }
    }
    $manifest = $script:testBuildManifest
    $manifestRoot = [string]$manifest.projectRoot
    if (-not [IO.Path]::IsPathRooted($manifestRoot)) {
        $manifestRoot = Join-Path (Split-Path -Parent ([IO.Path]::GetFullPath($BuildManifestPath))) $manifestRoot
    }
    $selectedPath = if ([IO.Path]::IsPathRooted($SourcePath)) { [IO.Path]::GetFullPath($SourcePath) }
        else { [IO.Path]::GetFullPath((Join-Path $manifestRoot $SourcePath)) }
    $selected = @()
    foreach ($project in $manifest.projects) {
        foreach ($source in $project.sources) {
            $path = if ([IO.Path]::IsPathRooted($source.path)) { [IO.Path]::GetFullPath($source.path) }
                else { [IO.Path]::GetFullPath((Join-Path $project.directory $source.path)) }
            if ($path -eq $selectedPath) { $selected += @{ Project = $project; Source = $source } }
        }
    }
    if ($selected.Count -ne 1) {
        throw "EXPECT_MANIFEST_SOURCE must select exactly one source in the CodeClip manifest: $SourcePath (found $($selected.Count))."
    }
    if (-not (Test-Path -LiteralPath $selectedPath -PathType Leaf)) {
        throw "Selected manifest source is missing: $selectedPath"
    }
    $project = $selected[0].Project
    $source = $selected[0].Source
    # These probes inherit the selected translation unit's preprocessing
    # environment. Optimization and warning policy remain test metadata.
    if ($manifest.schemaVersion -eq 1) {
        $includes = @($project.includeDirectories) + @($source.includeDirectories)
        $defines = @($project.defines) + @($source.defines)
    } else {
        $includes = if ($source.PSObject.Properties['includeDirectories']) { $source.includeDirectories } else { $project.includeDirectories }
        $defines = if ($source.PSObject.Properties['defines']) { $source.defines } else { $project.defines }
    }
    foreach ($include in $includes) {
        if ($include) {
            if (-not [IO.Path]::IsPathRooted($include)) { $include = Join-Path $project.directory $include }
            '-I' + [IO.Path]::GetFullPath($include)
        }
    }
    foreach ($define in $defines) { if ($define) { '-D' + $define } }
    $settings = @{}
    foreach ($group in @($project.compileSettings, $source.settings)) {
        if ($null -eq $group) { continue }
        foreach ($property in $group.PSObject.Properties) { $settings[$property.Name] = $property.Value }
    }
    foreach ($undefine in ([string]$settings.UndefinePreprocessorDefinitions).Split(';')) {
        if ($undefine) { '-U' + $undefine }
    }
    foreach ($include in ([string]$settings.ForcedIncludeFiles).Split(';')) {
        if ($include) {
            '-include'
            if ([IO.Path]::IsPathRooted($include)) { [IO.Path]::GetFullPath($include) }
            else { [IO.Path]::GetFullPath((Join-Path $project.directory $include)) }
        }
    }
}

function Invoke-Compiler {
    param(
        [string]$CompilerPath,
        [string[]]$SourcePaths,
        [string]$OutputPath,
        [string[]]$CompilerArgs
    )

    $arguments = @($CompilerArgs) + $SourcePaths
    # CPC response files escape backslashes and quotes independently of the
    # shell. Keep manifest-sized settings out of Windows process and wrapper
    # command lines, preserving each original argument exactly.
    $responsePath = $OutputPath + '.rsp'
    $response = @($arguments | ForEach-Object {
        '"' + $_.Replace('\', '\\').Replace('"', '\"') + '"'
    }) -join "`n"
    [IO.File]::WriteAllText($responsePath, $response, (New-Object Text.UTF8Encoding($false)))
    $savedEap = $ErrorActionPreference
    $ErrorActionPreference = "Continue"
    try {
        # A sole @file selects CPC's line-per-job batch mode. Keep the output
        # option outside so this is ordinary response-file expansion.
        $output = & $CompilerPath ("@" + $responsePath) '-o' $OutputPath 2>&1
        $exitCode = $LASTEXITCODE
    } finally {
        $ErrorActionPreference = $savedEap
        Remove-Item -LiteralPath $responsePath -ErrorAction SilentlyContinue
    }

    return @{
        ExitCode = $exitCode
        Output = (Normalize-LineEndings (($output | Out-String))).Trim()
    }
}

function Resolve-SharedOutDir {
    param([string]$ExplicitPath)

    if ($ExplicitPath) {
        $resolvedExplicit = Resolve-Path -LiteralPath $ExplicitPath -ErrorAction SilentlyContinue
        if ($resolvedExplicit) { return $resolvedExplicit.Path }
        return $null
    }

    $default = Join-Path $scriptDir "batch\out"
    $resolvedDefault = Resolve-Path -LiteralPath $default -ErrorAction SilentlyContinue
    if ($resolvedDefault) { return $resolvedDefault.Path }
    return $null
}

$compiler = Resolve-CompilerPath -ExplicitPath $CompilerPath
$testsRoot = Join-Path $scriptDir $Suite

if (-not (Test-Path -LiteralPath $testsRoot)) {
    throw "Suite directory does not exist: $testsRoot"
}

$passDir = Join-Path $testsRoot "pass"
$failDir = Join-Path $testsRoot "fail"

$workDir = Join-Path ([System.IO.Path]::GetTempPath()) ("cprime-language-tests-" + $PID + "-" + ([guid]::NewGuid().ToString("N")))

New-Item -ItemType Directory -Force -Path $workDir | Out-Null

try {
$tests = @()
if (Test-Path -LiteralPath $passDir) {
    $tests += Get-ChildItem -LiteralPath $passDir -Filter test_*.c | Sort-Object Name
    $tests += Get-ChildItem -LiteralPath $passDir -Filter test_*.cpp | Sort-Object Name
}
if (Test-Path -LiteralPath $failDir) {
    $tests += Get-ChildItem -LiteralPath $failDir -Filter test_*.c | Sort-Object Name
    $tests += Get-ChildItem -LiteralPath $failDir -Filter test_*.cpp | Sort-Object Name
}

if ($tests.Count -eq 0) {
    throw "No test files found under $testsRoot"
}

$passed = 0
$failed = 0
$sharedHits = 0
$sharedMisses = 0
$resolvedSharedOutDir = $null
if ($UseSharedBinaries) {
    $resolvedSharedOutDir = Resolve-SharedOutDir -ExplicitPath $SharedOutDir
}

Write-Host "Using compiler: $compiler"
Write-Host "Running suite: $Suite"
Write-Host ""

foreach ($test in $tests) {
    $meta = Parse-Metadata -FilePath $test.FullName
    $compileOnly = ($meta.EXPECT_COMPILE_ONLY -eq '1')
    $expectCompileFail = ($meta.EXPECT_COMPILE_FAIL -eq "1")
    $expectExit = [int]$meta.EXPECT_EXIT
    $expectStdout = Normalize-LineEndings $meta.EXPECT_STDOUT
    $compileArgs = @()
    try {
        $sourcePaths = @(Resolve-TestSources -Test $test -Metadata $meta)
        if ($compileOnly -and $sourcePaths.Count -ne 1) {
            throw 'EXPECT_COMPILE_ONLY supports one translation unit per test.'
        }
        if ($meta.EXPECT_MANIFEST_SOURCE) {
            $compileArgs += @(Get-ManifestCompileContext -SourcePath $meta.EXPECT_MANIFEST_SOURCE)
        }
        if ($meta.EXPECT_COMPILE_ARGS) {
            $compileArgs += @(Split-CompilerArgs $meta.EXPECT_COMPILE_ARGS)
        }
    } catch {
        $failed++
        Write-Host ("FAIL {0}: test setup failed: {1}" -f $test.Name, $_.Exception.Message)
        continue
    }
    if ($compileOnly) { $compileArgs += '-c' }
    $supportsShared = $UseSharedBinaries -and -not $expectCompileFail -and -not $compileOnly -and -not $meta.EXPECT_MANIFEST_SOURCE -and $sourcePaths.Count -eq 1 -and $compileArgs.Count -eq 0 -and $resolvedSharedOutDir

    $extension = if ($compileOnly) { '.obj' } else { '.exe' }
    $exeName = [System.IO.Path]::GetFileNameWithoutExtension($test.Name) + $extension
    $outExe = Join-Path $workDir $exeName

    if (Test-Path -LiteralPath $outExe) {
        Remove-Item -Force -LiteralPath $outExe
    }

    $compileOutput = ""
    $compileExit = 0
    $exeToRun = $outExe
    $sharedExe = $null

    if ($supportsShared) {
        $sharedExe = Join-Path $resolvedSharedOutDir $exeName
        if (Test-Path -LiteralPath $sharedExe) {
            $sharedHits++
            $exeToRun = $sharedExe
        } else {
            $sharedMisses++
        }
    }

    if (-not $supportsShared -or -not $sharedExe -or -not (Test-Path -LiteralPath $sharedExe)) {
        $compile = Invoke-Compiler -CompilerPath $compiler -SourcePaths $sourcePaths -OutputPath $outExe -CompilerArgs $compileArgs
        $compileOutput = $compile.Output
        $compileExit = $compile.ExitCode
    }

    $ok = $true
    $reason = ""

    if ($expectCompileFail) {
        if ($compileExit -eq 0) {
            $ok = $false
            $reason = "expected compile failure but compilation succeeded"
        }
    } else {
        if ($compileExit -ne 0) {
            $ok = $false
            $reason = "compile failed (exit $compileExit): $compileOutput"
        } elseif (-not (Test-Path -LiteralPath $exeToRun)) {
            $ok = $false
            $reason = "compile succeeded but output $extension missing"
        } elseif (-not $compileOnly) {
            $runOutput = & $exeToRun 2>&1
            $runExit = $LASTEXITCODE
            $normStdout = Normalize-LineEndings ($runOutput -join "`n")

            if ($runExit -ne $expectExit) {
                $ok = $false
                $reason = "expected exit $expectExit, got $runExit"
            } elseif ($expectStdout -and $normStdout -ne $expectStdout) {
                $ok = $false
                $reason = "stdout mismatch. expected '$expectStdout' got '$normStdout'"
            }
        }
    }

    if ($ok) {
        $passed++
        Write-Host ("PASS {0}" -f $test.Name)
    } else {
        $failed++
        Write-Host ("FAIL {0}: {1}" -f $test.Name, $reason)
    }
}

Write-Host ""
Write-Host ("Summary: {0} passed, {1} failed" -f $passed, $failed)
if ($UseSharedBinaries) {
    Write-Host ("Shared binaries: {0} hits, {1} misses" -f $sharedHits, $sharedMisses)
}

if ($failed -gt 0) {
    exit 1
}

if ($UseSharedBinaries -and $RequireSharedHits -and $sharedHits -eq 0) {
    Write-Host "Shared mode failed: no shared binary hits were used."
    exit 1
}

exit 0
} finally {
    $tempRoot = [IO.Path]::GetFullPath([IO.Path]::GetTempPath()).TrimEnd('\', '/') + [IO.Path]::DirectorySeparatorChar
    $resolvedWorkDir = [IO.Path]::GetFullPath($workDir)
    if (-not $resolvedWorkDir.StartsWith($tempRoot, [StringComparison]::OrdinalIgnoreCase) -or
        [IO.Path]::GetFileName($resolvedWorkDir) -notlike 'cprime-language-tests-*') {
        throw "Refusing to remove a test work directory outside the expected temporary location: $resolvedWorkDir"
    }
    if (Test-Path -LiteralPath $workDir) {
        Remove-Item -Recurse -Force -LiteralPath $workDir -ErrorAction SilentlyContinue
    }
}
