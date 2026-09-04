param(
    [string]$CpcPath = "",
    [string]$ProjectRoot = "C:\Luke\Src\OT\cl",
    [string]$OutDir = "",
    [string]$ExePath = "",
    [string]$SourcesFile = "",
    [string]$RuntimeDllDir = "",
    [string]$AssetsDir = "",
    [int]$CompileTimeoutSeconds = 30,
    [int]$SmokeTestSeconds = 10,
    [switch]$LinkOnly,
    [switch]$SkipLink,
    [switch]$SkipSmokeTest,
    [switch]$Unity,
    [switch]$ExportSymbols,
    [ValidateRange(1, 64)]
    [int]$Jobs = [Environment]::ProcessorCount
)

$ErrorActionPreference = "Stop"
if (Get-Variable -Name PSNativeCommandUseErrorActionPreference -Scope Global -ErrorAction SilentlyContinue) {
    $global:PSNativeCommandUseErrorActionPreference = $false
}

if (-not $CpcPath) {
    $CpcPath = Join-Path $PSScriptRoot "cpc.exe"
}
$CpcPath = [System.IO.Path]::GetFullPath($CpcPath)
$ProjectRoot = [System.IO.Path]::GetFullPath($ProjectRoot)
if (-not $OutDir) {
    $OutDir = Join-Path $ProjectRoot "builds\prime"
}
$OutDir = [System.IO.Path]::GetFullPath($OutDir)
if (-not $ExePath) {
    $ExePath = Join-Path $ProjectRoot "builds\racer.exe"
}
$exe = [System.IO.Path]::GetFullPath($ExePath)
$runtimeOutputDir = Split-Path $exe -Parent
. (Join-Path $PSScriptRoot "support\pe_runtime_dependencies.ps1")

if (-not $AssetsDir) {
    $AssetsDir = Join-Path $ProjectRoot "CommonLib\Assets"
}
$AssetsDir = [System.IO.Path]::GetFullPath($AssetsDir)

if (-not (Test-Path -LiteralPath $CpcPath)) { throw "Compiler not found: $CpcPath" }
if (-not (Test-Path -LiteralPath $ProjectRoot)) { throw "Project root not found: $ProjectRoot" }
if (-not (Test-Path -LiteralPath $AssetsDir -PathType Container)) { throw "Racer assets not found: $AssetsDir" }
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null
New-Item -ItemType Directory -Force -Path $runtimeOutputDir | Out-Null
if (-not $SkipLink -and (Test-Path -LiteralPath $exe -PathType Leaf)) {
    # Never let a failed compile or link leave a stale executable that can be
    # mistaken for the artifact produced by this invocation.
    Remove-Item -LiteralPath $exe -Force
}

$commonLib = Join-Path $ProjectRoot "CommonLib\commonLib"
$flagsFile = Join-Path $ProjectRoot "builds\core\generated_core_cpc_flags.rsp"
$runtimeLib = Join-Path $env:LOCALAPPDATA "cpc\1.4\lib\libcprime1.a"

if (-not (Test-Path -LiteralPath $flagsFile)) { throw "Missing flags rsp: $flagsFile" }

$flags = Get-Content -LiteralPath $flagsFile | Where-Object { $_.Trim() }
$flags += "-D_MSC_VER=1900"
$flags += "-DCPRIME_RACER_BUILD"
$flags += "-DMA_NO_SSE2"
$flags += "-DMA_NO_AVX2"
$flags += ('-I"' + (Join-Path $ProjectRoot "CommonLib\3rdParty\Imagine\include") + '"')
$flags += ('-I"' + (Join-Path $ProjectRoot "CommonLib\3rdParty\webp\include") + '"')
$flags += ('-I"' + (Join-Path $ProjectRoot "CommonLib\3rdParty\libtiff\Include") + '"')
$flags += ('-I"' + (Join-Path $ProjectRoot "CommonLib\3rdParty\nanoflan") + '"')
$flags += "-D_S_IFDIR=0x4000"
$flags += "-D_S_IREAD=0x0100"
$flags += "-D_S_IWRITE=0x0080"

# The normal project files are the source manifest, just as they are for the
# Clang batch builds.  A caller may pass the text manifest produced by the CMD
# files, while direct driver invocations read the same ClCompile entries here.
if ($SourcesFile) {
    $SourcesFile = [System.IO.Path]::GetFullPath($SourcesFile)
    if (-not (Test-Path -LiteralPath $SourcesFile -PathType Leaf)) {
        throw "Project source manifest not found: $SourcesFile"
    }
    $projectSources = Get-Content -LiteralPath $SourcesFile | Where-Object { $_.Trim() } |
        ForEach-Object { [System.IO.Path]::GetFullPath($_.Trim().Trim('"')) }
} else {
    $projectSources = @()
    $projectSpecs = @(
        @{ File = Join-Path $commonLib "commonLib.vcxproj"; Root = $commonLib }
        @{ File = Join-Path $ProjectRoot "Projects\Model Viewer\Model Viewer.vcxproj"; Root = Join-Path $ProjectRoot "Projects\Model Viewer" }
    )
    foreach ($spec in $projectSpecs) {
        if (-not (Test-Path -LiteralPath $spec.File -PathType Leaf)) {
            throw "Visual Studio project not found: $($spec.File)"
        }
        foreach ($line in Select-String -LiteralPath $spec.File -Pattern '<ClCompile Include="([^"]+)"') {
            foreach ($match in $line.Matches) {
                $projectSources += [System.IO.Path]::GetFullPath((Join-Path $spec.Root $match.Groups[1].Value))
            }
        }
    }
}
$projectSources = @($projectSources | Select-Object -Unique)
if (-not $projectSources.Count) { throw "The Visual Studio projects contain no compile sources." }

$projectBuildSources = @()
foreach ($source in $projectSources) {
    if (-not (Test-Path -LiteralPath $source -PathType Leaf)) {
        throw "Project compile source not found: $source"
    }
    $leaf = [System.IO.Path]::GetFileName($source)
    # Retain the three existing CPC compatibility substitutions.  Source
    # discovery itself now comes entirely from the shared project manifest.
    if ($leaf -eq "clKNN3.cpp" -or $leaf -eq "clPrint.cpp") { continue }
    if ($leaf -eq "clAssert.cpp") {
        $source = Join-Path $commonLib "src\Platform\clRelAssert.cpp"
        $leaf = "clRelAssert.cpp"
    }
    $name = [System.IO.Path]::GetFileNameWithoutExtension($leaf)
    $priority = if ($name -eq "clCamera") { 0 } elseif (
        $source.StartsWith((Join-Path $ProjectRoot "Projects\Model Viewer"), [System.StringComparison]::OrdinalIgnoreCase) -or
        $source -like "*\src\Core\*") { 1 } else { 2 }
    $entry = @{ Name = $name; Source = $source; Priority = $priority }
    if ($name -eq "LZ4") { $entry.Flags = @("-Dui32=uint32_t", "-Dui8=uint8_t") }
    $projectBuildSources += $entry
}

# CPC still needs these implementation units in addition to the canonical
# project graph.  They are compiler/link compatibility inputs, not a second
# hand-maintained copy of the project's source list.
$cpcImplementationSources = @(
    @{ Name = "vnHalf"; Source = (Join-Path $ProjectRoot "CommonLib\3rdParty\Imagine\include\Base\vnhalf.cpp") }
    @{ Name = "vnImage"; Source = (Join-Path $ProjectRoot "CommonLib\3rdParty\Imagine\include\Base\vnimage.cpp") }
    @{ Name = "vnImageAverage"; Source = (Join-Path $ProjectRoot "CommonLib\3rdParty\Imagine\include\Kernels\vnimageaverage.cpp") }
    @{ Name = "vnImageBicubic"; Source = (Join-Path $ProjectRoot "CommonLib\3rdParty\Imagine\include\Kernels\vnimagebicubic.cpp") }
    @{ Name = "vnImageBilinear"; Source = (Join-Path $ProjectRoot "CommonLib\3rdParty\Imagine\include\Kernels\vnimagebilinear.cpp") }
    @{ Name = "vnImageCoverage"; Source = (Join-Path $ProjectRoot "CommonLib\3rdParty\Imagine\include\Kernels\vnimagecoverage.cpp") }
    @{ Name = "vnImageGaussian"; Source = (Join-Path $ProjectRoot "CommonLib\3rdParty\Imagine\include\Kernels\vnimagegaussian.cpp"); PatchGaussianConstants = $true; Flags = @(('-I"' + (Join-Path $ProjectRoot "CommonLib\3rdParty\Imagine\include\Kernels") + '"')) }
    @{ Name = "vnImageLanczos"; Source = (Join-Path $ProjectRoot "CommonLib\3rdParty\Imagine\include\Kernels\vnimagelanczos.cpp") }
    @{ Name = "vnImageNearest"; Source = (Join-Path $ProjectRoot "CommonLib\3rdParty\Imagine\include\Kernels\vnimagenearest.cpp") }
    @{ Name = "vnImageSpline"; Source = (Join-Path $ProjectRoot "CommonLib\3rdParty\Imagine\include\Kernels\vnimagespline.cpp") }
    @{ Name = "vnImageConvert"; Source = (Join-Path $ProjectRoot "CommonLib\3rdParty\Imagine\include\Operators\vnimageconvert.cpp") }
    @{ Name = "vnImageResize"; Source = (Join-Path $ProjectRoot "CommonLib\3rdParty\Imagine\include\Operators\vnimageresize.cpp") }
    @{ Name = "vnImageScale"; Source = (Join-Path $ProjectRoot "CommonLib\3rdParty\Imagine\include\Operators\vnimagescale.cpp") }
    @{ Name = "vnImageBlock"; Source = (Join-Path $ProjectRoot "CommonLib\3rdParty\Imagine\include\Utilities\vnimageblock.cpp") }
    @{ Name = "vnImageBlockPack"; Source = (Join-Path $ProjectRoot "CommonLib\3rdParty\Imagine\include\Utilities\vnimageblockpack.cpp") }
    @{ Name = "vnImageBlockUnpack"; Source = (Join-Path $ProjectRoot "CommonLib\3rdParty\Imagine\include\Utilities\vnimageblockunpack.cpp") }
    @{ Name = "vnImageSampler"; Source = (Join-Path $ProjectRoot "CommonLib\3rdParty\Imagine\include\Utilities\vnimagesampler.cpp") }
)
$cpcSupportSources = @(
    @{ Name = "clKNN3CpcSupport"; Source = (Join-Path $PSScriptRoot "support\racer_cpc_knn3.cpp") }
    @{ Name = "clAbiCpcSupport"; Source = (Join-Path $PSScriptRoot "support\racer_cpc_abi.cpp") }
    @{ Name = "clPrintCpcSupport"; Source = (Join-Path $PSScriptRoot "support\racer_cpc_print.cpp") }
)
$additionalSources = @($projectBuildSources) + @($cpcImplementationSources) + @($cpcSupportSources)

if ($Jobs -gt [Environment]::ProcessorCount) {
    $Jobs = [Environment]::ProcessorCount
}
$script:activeCompiles = [System.Collections.ArrayList]::new()
$script:anyCompileFailed = $false

function Complete-CpcCompile {
    param([pscustomobject]$Compile)

    $proc = $Compile.Process
    $timedOut = -not $proc.HasExited -and
        $Compile.Timer.Elapsed.TotalSeconds -ge $CompileTimeoutSeconds
    if ($timedOut) {
        Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue
        $proc.WaitForExit()
        $exit = -999
        Add-Content -LiteralPath $Compile.Log -Encoding ASCII `
            -Value ("cpc compile timed out after {0}s" -f $CompileTimeoutSeconds)
    } else {
        $proc.WaitForExit()
        $proc.Refresh()
        $exit = $proc.ExitCode
    }
    $Compile.Timer.Stop()
    if (Test-Path -LiteralPath $Compile.ErrLog) {
        Get-Content -LiteralPath $Compile.ErrLog | Add-Content -LiteralPath $Compile.Log
    }
    $text = ""
    if (Test-Path -LiteralPath $Compile.Log) {
        $raw = Get-Content -Raw -LiteralPath $Compile.Log
        if ($raw) { $text = $raw }
    }
    $errors = ([regex]::Matches($text, "error:" )).Count
    $warnings = ([regex]::Matches($text, "warning:" )).Count
    $status = if ($timedOut) { " TIMEOUT" } else { "" }
    Write-Host ("{0,-22} {1,9:n3}s exit={2,-4} warnings={3,-4} errors={4}{5}" -f `
        $Compile.Label, $Compile.Timer.Elapsed.TotalSeconds, $exit, $warnings, $errors, $status)
    if ($exit -ne 0) {
        $script:anyCompileFailed = $true
        $firstError = (Get-Content -LiteralPath $Compile.Log | Select-String -Pattern "error:" | Select-Object -First 3) -join "`n"
        Write-Host $firstError
    }
}

function Wait-CpcCompileSlot {
    param([switch]$Drain)

    do {
        $completed = $false
        foreach ($compile in @($script:activeCompiles)) {
            if ($compile.Process.HasExited -or
                $compile.Timer.Elapsed.TotalSeconds -ge $CompileTimeoutSeconds) {
                Complete-CpcCompile -Compile $compile
                [void]$script:activeCompiles.Remove($compile)
                $completed = $true
                if (-not $Drain) { return }
            }
        }
        if ($script:activeCompiles.Count -and (-not $completed -or $Drain)) {
            Start-Sleep -Milliseconds 10
        }
    } while ($script:activeCompiles.Count -and
             ($Drain -or $script:activeCompiles.Count -ge $Jobs))
}

function Invoke-CpcCompile {
    param(
        [string]$Label,
        [string]$Source,
        [string[]]$ExtraFlags
    )

    $obj = Join-Path $OutDir ($Label + ".obj")
    $log = Join-Path $OutDir ($Label + ".obj.log")
    $args = @($flags)
    if ($ExtraFlags) {
        $args += @($ExtraFlags)
    }
    # Start-Process reconstructs a command line from ArgumentList and otherwise
    # splits source paths containing spaces into multiple input files.
    $quotedSource = if ($Source -match '\s') { '"' + $Source + '"' } else { $Source }
    $args += @("-c", $quotedSource, "-o", $obj)
    $errLog = $log + ".err"
    Remove-Item -LiteralPath $log, $errLog -Force -ErrorAction SilentlyContinue
    while ($script:activeCompiles.Count -ge $Jobs) {
        Wait-CpcCompileSlot
    }
    $compileTimer = [System.Diagnostics.Stopwatch]::StartNew()
    $proc = Start-Process -FilePath $CpcPath -ArgumentList $args `
        -RedirectStandardOutput $log -RedirectStandardError $errLog `
        -PassThru -NoNewWindow
    # Materialize the native handle now so ExitCode remains available after
    # redirected streams drain on Windows PowerShell.
    $processHandle = $proc.Handle
    [void]$script:activeCompiles.Add([pscustomobject]@{
        Label = $Label
        Process = $proc
        Timer = $compileTimer
        Log = $log
        ErrLog = $errLog
    })
    return $true
}

if (-not $LinkOnly) {
    # Remove stale objects from previous runs so every link is built from a
    # single consistent compiler and source state.
    $outFull = [System.IO.Path]::GetFullPath($OutDir).TrimEnd('\') + '\'
    foreach ($pattern in @("generated_*.obj", "additional_*.obj", "unity_*.obj", "core_*.obj", "commonlib_*.obj")) {
        Get-ChildItem -LiteralPath $OutDir -Filter $pattern -ErrorAction SilentlyContinue | ForEach-Object {
            $full = [System.IO.Path]::GetFullPath($_.FullName)
            if ($full.StartsWith($outFull, [System.StringComparison]::OrdinalIgnoreCase)) {
                Remove-Item -LiteralPath $full -Force
            }
        }
    }
    if ($Unity) {
        $unityLines = @()
        # Define camera members immediately after their declarations.  Large
        # template-heavy consumers can otherwise instantiate similarly named
        # member signatures before CPC reaches the out-of-class definitions.
        $unitySupportNames = @("clKNN3CpcSupport", "clAbiCpcSupport", "clPrintCpcSupport")
        $unityEntries = @($additionalSources |
            Where-Object { $unitySupportNames -notcontains $_.Name } |
            Sort-Object @{ Expression = { if ($_.Priority -ne $null) { $_.Priority } else { 2 } } })
        foreach ($entry in $unityEntries) {
            $src = $entry.Source
            if ($entry.PatchGaussianConstants) {
                $patchedKernelDir = Join-Path $OutDir "Imagine\Kernels"
                New-Item -ItemType Directory -Force -Path $patchedKernelDir | Out-Null
                $patchedSource = Join-Path $patchedKernelDir "vnimagegaussian.cpp"
                $patchedHeader = Join-Path $patchedKernelDir "vnImageGaussian.h"
                Copy-Item -LiteralPath $src -Destination $patchedSource -Force
                $headerSource = Join-Path (Split-Path $src -Parent) "vnImageGaussian.h"
                $sourceText = Get-Content -Raw -LiteralPath $headerSource
                $sourceText = $sourceText.Replace(
                    "static CONST FLOAT32 g_stddev   = sqrt( g_variance );",
                    "static CONST FLOAT32 g_stddev   = 0.31622776601683794f;")
                $sourceText = $sourceText.Replace(
                    "static CONST FLOAT32 g_coeff    = 1.0f / ( g_stddev * sqrt( 2.0 * VN_PI ) );",
                    "static CONST FLOAT32 g_coeff    = 1.2615662610100802f;")
                Set-Content -LiteralPath $patchedHeader -Value $sourceText -Encoding ASCII
                $src = $patchedSource
            }
            if ($entry.Name -eq "LZ4") {
                $unityLines += "#define ui32 uint32_t"
                $unityLines += "#define ui8 uint8_t"
            }
            $unityLines += ('#include "' + $src.Replace('\', '/') + '"')
            if ($entry.Name -eq "LZ4") {
                $unityLines += "#undef ui8"
                $unityLines += "#undef ui32"
            }
        }
        $unityKernelFlags = @('-I"' + (Join-Path $ProjectRoot "CommonLib\3rdParty\Imagine\include\Kernels") + '"')
        $unityChunks = @()
        $currentChunk = @()
        $includeCount = 0
        for ($lineIndex = 0; $lineIndex -lt $unityLines.Count; ++$lineIndex) {
            $line = $unityLines[$lineIndex]
            $isIsolatedInclude = $line -like '*src/Raster/clImage.cpp"' -or
                $line -like '*src/Raster/clHistogram.cpp"' -or
                $line -like '*src/Grid/clReachabilityFinder.cpp"'
            if ($isIsolatedInclude -and $currentChunk.Count) {
                $unityChunks += ,@($currentChunk)
                $currentChunk = @()
                $includeCount = 0
            }
            $currentChunk += $line
            if ($line.StartsWith('#include ')) { ++$includeCount }
            $nextIsUndef = $lineIndex + 1 -lt $unityLines.Count -and $unityLines[$lineIndex + 1].StartsWith('#undef ')
            if (($includeCount -ge 4 -or $isIsolatedInclude) -and -not $nextIsUndef) {
                $unityChunks += ,@($currentChunk)
                $currentChunk = @()
                $includeCount = 0
            }
        }
        if ($currentChunk.Count) { $unityChunks += ,@($currentChunk) }

        $unityOk = $true
        $unityChunkPaths = @()
        for ($chunkIndex = 0; $chunkIndex -lt $unityChunks.Count; ++$chunkIndex) {
            $chunkSource = Join-Path $OutDir ("generated_racer_unity_{0:d2}.cpp" -f $chunkIndex)
            Set-Content -LiteralPath $chunkSource -Value $unityChunks[$chunkIndex] -Encoding ASCII
            $unityChunkPaths += $chunkSource
        }
        # Long template-heavy chunks determine parallel wall time.  Launch
        # those first so quick translation units cannot occupy every worker
        # while the critical jobs are still waiting in the submission loop.
        $unityCompileOrder = @(0..($unityChunks.Count - 1) | Sort-Object -Descending {
            $text = $unityChunks[$_] -join "`n"
            if ($text -like '*clRenderObject.cpp*') { 1000 }
            elseif ($text -like '*Racer.cpp*') { 950 }
            elseif ($text -like '*cl2DDraw.cpp*') { 900 }
            elseif ($text -like '*clRenderObjectCore.cpp*') { 850 }
            elseif ($text -like '*src/Raster/clImage.cpp*') { 800 }
            elseif ($text -like '*clDepthImage.cpp*') { 750 }
            elseif ($text -like '*clShaders.cpp*') { 700 }
            elseif ($text -like '*clImageRaw.cpp*') { 650 }
            else { 0 }
        })
        foreach ($chunkIndex in $unityCompileOrder) {
            $chunkSource = $unityChunkPaths[$chunkIndex]
            $chunkOk = Invoke-CpcCompile -Label ("unity_Racer_{0:d2}" -f $chunkIndex) -Source $chunkSource -ExtraFlags $unityKernelFlags
            if (-not $chunkOk) { $unityOk = $false }
        }
        $generatedOk = $unityOk
        $additionalOk = $unityOk
        foreach ($entry in $additionalSources | Where-Object { $unitySupportNames -contains $_.Name }) {
            $supportOk = Invoke-CpcCompile -Label ("additional_" + $entry.Name) -Source $entry.Source
            if (-not $supportOk) { $additionalOk = $false }
        }
    } else {
    $generatedOk = $true
    $additionalOk = $true
    foreach ($entry in $additionalSources) {
        $src = $entry.Source
        if ($entry.PatchGaussianConstants) {
            $patchedKernelDir = Join-Path $OutDir "Imagine\Kernels"
            New-Item -ItemType Directory -Force -Path $patchedKernelDir | Out-Null
            $patchedSource = Join-Path $patchedKernelDir "vnimagegaussian.cpp"
            $patchedHeader = Join-Path $patchedKernelDir "vnImageGaussian.h"
            Copy-Item -LiteralPath $src -Destination $patchedSource -Force
            $headerSource = Join-Path (Split-Path $src -Parent) "vnImageGaussian.h"
            $sourceText = Get-Content -Raw -LiteralPath $headerSource
            $sourceText = $sourceText.Replace(
                "static CONST FLOAT32 g_stddev   = sqrt( g_variance );",
                "static CONST FLOAT32 g_stddev   = 0.31622776601683794f;")
            $sourceText = $sourceText.Replace(
                "static CONST FLOAT32 g_coeff    = 1.0f / ( g_stddev * sqrt( 2.0 * VN_PI ) );",
                "static CONST FLOAT32 g_coeff    = 1.2615662610100802f;")
            Set-Content -LiteralPath $patchedHeader -Value $sourceText -Encoding ASCII
            $src = $patchedSource
        }
        $extraFlags = if ($entry.Flags) { @($entry.Flags) } else { @() }
        $ok = Invoke-CpcCompile -Label ("additional_" + $entry.Name) -Source $src -ExtraFlags $extraFlags
        if (-not $ok) { $additionalOk = $false }
    }
    }
}

if ($script:activeCompiles.Count) {
    Wait-CpcCompileSlot -Drain
}
if ($script:anyCompileFailed) {
    $generatedOk = $false
    $additionalOk = $false
}
if (-not $LinkOnly) {
    if ($Unity) {
        Write-Host ("Unity source compile: " + $(if ($generatedOk -and $additionalOk) { "OK" } else { "FAILURE" }))
    } else {
        Write-Host ("Generated source compile: " + $(if ($generatedOk) { "all OK" } else { "FAILURES" }))
        Write-Host ("Additional source compile: " + $(if ($additionalOk) { "all OK" } else { "FAILURES" }))
    }
}

if ($SkipLink) {
    if ($generatedOk -and $additionalOk) { exit 0 }
    exit 1
}
if (-not $LinkOnly -and (-not $generatedOk -or -not $additionalOk)) {
    Write-Host "Link skipped because one or more source compilations failed."
    Write-Host "Executable: NOT PRODUCED"
    exit 1
}

# Link all objects that exist in the output directory. cpc's @file syntax
# runs each line of the file as one job, so the whole link command must be a
# single line in the batch file.
$objs = @()
$objs += Get-ChildItem -LiteralPath $OutDir -Filter "generated_*.obj" | Sort-Object Name
$objs += Get-ChildItem -LiteralPath $OutDir -Filter "additional_*.obj" | Sort-Object Name
$objs += Get-ChildItem -LiteralPath $OutDir -Filter "unity_*.obj" | Sort-Object Name
$linkBat = Join-Path $OutDir "link_cmd.bat"

$def = Join-Path $OutDir "racer_test.def"
@"
LIBRARY racer_test.exe

EXPORTS
AmdPowerXpressRequestHighPerformance
NvOptimusEnablement
"@ | Set-Content -LiteralPath $def -Encoding ASCII

$linkLog = Join-Path $OutDir "link.log"
$runtimeDllNames = @("SDL2.dll", "libtiff-5.dll", "libwebp-7.dll", "libjpeg-9.dll", "zlib1.dll")
if ($RuntimeDllDir) {
    $runtimeCandidates = @([System.IO.Path]::GetFullPath($RuntimeDllDir))
} else {
    $runtimeCandidates = @(
        (Join-Path $ProjectRoot "CommonLib\3rdParty\Runtime\x64"),
        "C:\Program Files\Python312\Lib\site-packages\pygame"
    )
    $programFiles = [Environment]::GetFolderPath([Environment+SpecialFolder]::ProgramFiles)
    $runtimeCandidates += Get-ChildItem -LiteralPath $programFiles -Directory -Filter "Python*" -ErrorAction SilentlyContinue |
        ForEach-Object { Join-Path $_.FullName "Lib\site-packages\pygame" }
}
$runtimeCandidates = @($runtimeCandidates | Where-Object { $_ } | Select-Object -Unique)
$runtimeDllDir = $runtimeCandidates | Where-Object {
    $candidate = $_
    (Test-Path -LiteralPath $candidate -PathType Container) -and
        -not ($runtimeDllNames | Where-Object { -not (Test-Path -LiteralPath (Join-Path $candidate $_) -PathType Leaf) })
} | Select-Object -First 1
if (-not $runtimeDllDir) {
    throw ("Unable to locate one complete Racer x64 runtime bundle. Required files: " +
        ($runtimeDllNames -join ", ") + ". Checked: " + ($runtimeCandidates -join "; "))
}

$sdl2Dll = Join-Path $runtimeDllDir "SDL2.dll"
$sdl2Def = Join-Path $OutDir "SDL2.def"
& $CpcPath -impdef $sdl2Dll -o $sdl2Def
if ($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath $sdl2Def -PathType Leaf)) {
    throw "Failed to generate SDL2 import definition from $sdl2Dll"
}

$expectedMachine = 0x8664
foreach ($dllName in $runtimeDllNames) {
    $source = Join-Path $runtimeDllDir $dllName
    $image = Get-PeImageInfo -Path $source
    if ($image.Machine -ne $expectedMachine) {
        throw ("Runtime DLL has the wrong architecture: {0} is {1}; expected x64" -f $source, $image.MachineName)
    }
    Copy-Item -LiteralPath $source -Destination (Join-Path $runtimeOutputDir $dllName) -Force
}
Write-Host ("Runtime DLL bundle: " + $runtimeDllDir)

# Racer resolves resources below Assets/ relative to its current working
# directory.  Expose the complete source asset tree beside the executable so
# launching Racer.exe directly behaves the same as launching from CommonLib.
# A junction avoids copying the roughly 500 MB development asset tree on every
# compiler iteration while still making every resource available.
$packagedAssets = Join-Path $runtimeOutputDir "Assets"
if (Test-Path -LiteralPath $packagedAssets) {
    $packagedAssetsItem = Get-Item -LiteralPath $packagedAssets -Force
    if ($packagedAssetsItem.Attributes -band [System.IO.FileAttributes]::ReparsePoint) {
        $packagedAssetsTarget = [System.IO.Path]::GetFullPath([string]$packagedAssetsItem.Target)
        if ($packagedAssetsTarget -ne $AssetsDir) {
            throw "Racer asset junction points to '$packagedAssetsTarget'; expected '$AssetsDir'."
        }
    } elseif (-not $packagedAssetsItem.PSIsContainer) {
        throw "Racer asset path is not a directory: $packagedAssets"
    }
} else {
    New-Item -ItemType Junction -Path $packagedAssets -Target $AssetsDir | Out-Null
}
Write-Host ("Runtime assets: " + $packagedAssets + " -> " + $AssetsDir)

$tiffDll = Join-Path $runtimeOutputDir "libtiff-5.dll"
$webpDll = Join-Path $runtimeOutputDir "libwebp-7.dll"
$tiffDef = Join-Path $OutDir "libtiff.def"
$webpDef = Join-Path $OutDir "libwebp.def"
@"
LIBRARY $(Split-Path $tiffDll -Leaf)

EXPORTS
_TIFFfree
_TIFFmalloc
TIFFClose
TIFFDefaultStripSize
TIFFGetField
TIFFOpen
TIFFReadRGBAImage
TIFFScanlineSize
TIFFSetField
TIFFWriteScanline
"@ | Set-Content -LiteralPath $tiffDef -Encoding ASCII
@"
LIBRARY $(Split-Path $webpDll -Leaf)

EXPORTS
WebPDecodeRGBA
WebPEncodeLosslessRGBA
WebPEncodeRGBA
WebPFree
"@ | Set-Content -LiteralPath $webpDef -Encoding ASCII
$linkTokens = @()
foreach ($obj in $objs) { $linkTokens += $obj.FullName }
$linkTokens += @(
    $def,
    $sdl2Def,
    $tiffDef,
    $webpDef,
    $runtimeLib,
    "-luser32", "-lgdi32", "-lopengl32", "-lws2_32", "-lmsvcrt",
    "-limm32", "-lshcore", "-lshell32", "-lole32"
)
if ($ExportSymbols) { $linkTokens += "-rdynamic" }
$linkTokens += @("-o", $exe)
$linkTokens -join " " | Set-Content -LiteralPath $linkBat -Encoding ASCII

$savedEap = $ErrorActionPreference
$ErrorActionPreference = "Continue"
try {
    & $CpcPath ("@" + $linkBat) *> $linkLog 2>&1
    $exit = $LASTEXITCODE
} finally {
    $ErrorActionPreference = $savedEap
}

$linkText = if (Test-Path -LiteralPath $linkLog) { Get-Content -Raw -LiteralPath $linkLog } else { "" }
if ($null -eq $linkText) { $linkText = "" }
$undefined = [regex]::Matches($linkText, "undefined symbol '([^']+)'") |
    ForEach-Object { $_.Groups[1].Value } |
    Sort-Object -Unique
$undefinedFile = Join-Path $OutDir "link_undefined.txt"
Set-Content -LiteralPath $undefinedFile -Encoding ASCII `
    -Value ($undefined -join [Environment]::NewLine)

Write-Host ""
Write-Host ("Link exit: {0}" -f $exit)
Write-Host ("Undefined symbols: {0}" -f $undefined.Count)
$undefined | ForEach-Object { Write-Host ("  " + $_) }
if ($exit -eq 0 -and (Test-Path -LiteralPath $exe)) {
    Write-Host ("Executable: " + $exe)
    $dependencyResult = Test-PeRuntimeDependencies -ImagePath $exe -ThrowOnError
    Write-Host ("Runtime dependency closure: OK ({0} local DLLs, {1})" -f `
        $dependencyResult.LocalImages.Count, $dependencyResult.Machine)
    if (-not $SkipSmokeTest) {
        if ($SmokeTestSeconds -lt 1) {
            throw "SmokeTestSeconds must be at least 1 unless -SkipSmokeTest is used."
        }
        $smokeStdout = Join-Path $OutDir "smoke.stdout.log"
        $smokeStderr = Join-Path $OutDir "smoke.stderr.log"
        # Relative assets are part of the application package, so launch with
        # the packaged executable directory as the working directory.
        $runtimeWorkingDir = $runtimeOutputDir
        Remove-Item -LiteralPath $smokeStdout, $smokeStderr -Force -ErrorAction SilentlyContinue
        $smokeProcess = Start-Process -FilePath $exe -WorkingDirectory $runtimeWorkingDir `
            -WindowStyle Hidden -RedirectStandardOutput $smokeStdout `
            -RedirectStandardError $smokeStderr -PassThru
        # Retain the native handle so ExitCode remains available after a timed wait.
        $smokeProcessHandle = $smokeProcess.Handle
        if ($smokeProcess.WaitForExit($SmokeTestSeconds * 1000)) {
            $smokeProcess.WaitForExit()
            $smokeProcess.Refresh()
            Write-Host ("Startup smoke test: FAILED (exited after less than {0}s, code {1})" -f `
                $SmokeTestSeconds, $smokeProcess.ExitCode)
            foreach ($smokeLog in @($smokeStdout, $smokeStderr)) {
                if (Test-Path -LiteralPath $smokeLog) {
                    $smokeText = Get-Content -Raw -LiteralPath $smokeLog
                    if ($smokeText) {
                        Write-Host ((Split-Path $smokeLog -Leaf) + ":")
                        Write-Host $smokeText
                    }
                }
            }
            Remove-Item -LiteralPath $exe -Force
            Write-Host "Executable: NOT PRODUCED"
            $exit = 1
        } else {
            # Stop only the process created for this bounded smoke test.
            Stop-Process -Id $smokeProcess.Id -Force -ErrorAction SilentlyContinue
            $smokeProcess.WaitForExit()
            Write-Host ("Startup smoke test: OK (running after {0}s)" -f $SmokeTestSeconds)
        }
    } else {
        Write-Host "Startup smoke test: SKIPPED"
    }
} else {
    if (Test-Path -LiteralPath $exe -PathType Leaf) {
        Remove-Item -LiteralPath $exe -Force
    }
    Write-Host "Executable: NOT PRODUCED"
}
exit $exit
