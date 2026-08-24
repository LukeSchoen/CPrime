param(
    [string]$CpcPath = "",
    [string]$ProjectRoot = "C:\Luke\Src\OT\cl",
    [string]$OutDir = "",
    [string]$RuntimeDllDir = "",
    [string]$AssetsDir = "",
    [int]$CompileTimeoutSeconds = 30,
    [int]$SmokeTestSeconds = 10,
    [switch]$LinkOnly,
    [switch]$SkipLink,
    [switch]$SkipSmokeTest,
    [switch]$ExportSymbols
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
    $OutDir = Join-Path $ProjectRoot "builds\racer"
}
$OutDir = [System.IO.Path]::GetFullPath($OutDir)
. (Join-Path $PSScriptRoot "support\pe_runtime_dependencies.ps1")

if (-not $AssetsDir) {
    $AssetsDir = Join-Path $ProjectRoot "CommonLib\Assets"
}
$AssetsDir = [System.IO.Path]::GetFullPath($AssetsDir)

if (-not (Test-Path -LiteralPath $CpcPath)) { throw "Compiler not found: $CpcPath" }
if (-not (Test-Path -LiteralPath $ProjectRoot)) { throw "Project root not found: $ProjectRoot" }
if (-not (Test-Path -LiteralPath $AssetsDir -PathType Container)) { throw "Racer assets not found: $AssetsDir" }
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null
$exe = Join-Path $OutDir "Racer.exe"
if (-not $SkipLink -and (Test-Path -LiteralPath $exe -PathType Leaf)) {
    # Never let a failed compile or link leave a stale executable that can be
    # mistaken for the artifact produced by this invocation.
    Remove-Item -LiteralPath $exe -Force
}

$commonLib = Join-Path $ProjectRoot "CommonLib\commonLib"
$flagsFile = Join-Path $ProjectRoot "builds\core\generated_core_cpc_flags.rsp"
$sourcesFile = Join-Path $ProjectRoot "builds\core\generated_core_sources.rsp"
$sdl2Def = Join-Path $ProjectRoot "builds\racer\SDL2.def"
$runtimeLib = Join-Path $env:LOCALAPPDATA "cpc\1.4\lib\libcprime1.a"

if (-not (Test-Path -LiteralPath $flagsFile)) { throw "Missing flags rsp: $flagsFile" }
if (-not (Test-Path -LiteralPath $sourcesFile)) { throw "Missing sources rsp: $sourcesFile" }
if (-not (Test-Path -LiteralPath $sdl2Def)) { throw "Missing SDL2 import def: $sdl2Def" }

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

# Additional implementation sources compiled separately because the generated
# source list does not include them.
$additionalSources = @(
    @{ Name = "cl2DDraw";          Rel = "Polygon\cl2DDraw.cpp" }
    @{ Name = "cl2DDrawSample";    Rel = "Polygon\cl2DDrawSample.cpp" }
    @{ Name = "cl2DDrawTransform"; Rel = "Polygon\cl2DDrawTransform.c" }
    @{ Name = "clAlloc";           Rel = "Platform\clAlloc.cpp" }
    @{ Name = "clAssets";          Rel = "Platform\clAssets.cpp" }
    @{ Name = "clControls";        Rel = "UI\clControls.cpp" }
    @{ Name = "clImageAccess";     Rel = "Raster\clImageAccess.cpp" }
    @{ Name = "clHardwareTexture"; Rel = "Raster\clHardwareTexture.cpp" }
    @{ Name = "clMemory";          Rel = "Platform\clMemory.cpp" }
    @{ Name = "clScan";            Rel = "Strings\clScan.cpp" }
    @{ Name = "clSeekRaw";         Rel = "Strings\clSeekRaw.cpp" }
    @{ Name = "clStringEncoding";  Rel = "Strings\clStringEncoding.cpp" }
    @{ Name = "clStream";           Rel = "Streams\clStream.cpp" }
    @{ Name = "clSeek";             Rel = "Streams\clSeek.cpp" }
    @{ Name = "clMemoryStream";     Rel = "Streams\clMemoryStream.cpp" }
    @{ Name = "clRawFileStream";    Rel = "Streams\clRawFileStream.cpp" }
    @{ Name = "clString";          Rel = "Strings\clString.cpp" }
    @{ Name = "clTinyGLWrangler";  Rel = "Polygon\clTinyGLWrangler.cpp" }
    @{ Name = "clWindow";          Rel = "Raster\clWindow.cpp" }
    @{ Name = "clRelAssert";       Rel = "Platform\clRelAssert.cpp" }
    @{ Name = "clPath";            Rel = "Platform\clPath.cpp" }
    @{ Name = "clFolder";          Rel = "Platform\clFolder.cpp" }
    @{ Name = "clDecryptThis";     Rel = "Encryption\clDecryptThis.cpp" }
    @{ Name = "clCamera";          Rel = "UI\clCamera.cpp" }
    @{ Name = "clImage";           Rel = "Raster\clImage.cpp" }
    @{ Name = "clRenderObjectCore"; Rel = "Polygon\clRenderObjectCore.cpp" }
    @{ Name = "clRenderObject";    Rel = "Polygon\clRenderObject.cpp" }
    @{ Name = "clBitArray2";       Rel = "Buffer\clBitArray2.cpp" }
    @{ Name = "clBitList";         Rel = "Buffer\clBitList.cpp" }
    @{ Name = "clLZ4";             Rel = "Compression\clLZ4.cpp" }
    @{ Name = "LZ4";               Rel = "Compression\LZ4.c"; Flags = @("-Dui32=uint32_t", "-Dui8=uint8_t") }
    @{ Name = "clReachabilityFinder"; Rel = "Grid\clReachabilityFinder.cpp" }
    @{ Name = "clHash";            Rel = "Math\clHash.cpp" }
    @{ Name = "clKHashP";          Rel = "Math\Hash\khashp.c" }
    @{ Name = "clDDA2";            Rel = "Math\Geometry\clDDA2.cpp" }
    @{ Name = "clKNN3CpcSupport"; Source = (Join-Path $PSScriptRoot "support\racer_cpc_knn3.cpp") }
    @{ Name = "clPlatformCpcSupport"; Source = (Join-Path $PSScriptRoot "support\racer_cpc_platform.cpp") }
    @{ Name = "clAbiCpcSupport"; Source = (Join-Path $PSScriptRoot "support\racer_cpc_abi.cpp") }
    @{ Name = "clFile";            Rel = "Platform\clFile.cpp" }
    @{ Name = "clDrive";           Rel = "Platform\clDrive.cpp" }
    @{ Name = "clFileDump";        Rel = "Platform\clFileDump.cpp" }
    @{ Name = "clFileHelper";      Rel = "Platform\clFileHelper.cpp" }
    @{ Name = "clReport";          Rel = "Platform\clReport.cpp" }
    @{ Name = "clCannyFilter";     Rel = "Raster\clCannyFilter.cpp" }
    @{ Name = "clColor";           Rel = "Raster\clColor.cpp" }
    @{ Name = "clDepthImage";      Rel = "Raster\clDepthImage.cpp" }
    @{ Name = "clHistogram";       Rel = "Raster\clHistogram.cpp" }
    @{ Name = "clImageRaw";        Rel = "Raster\clImageRaw.cpp" }
    @{ Name = "clFileStream";      Rel = "Streams\clFileStream.cpp" }
    @{ Name = "clWideString";      Rel = "Strings\clWideString.cpp" }
    @{ Name = "clPrintCpcSupport"; Source = (Join-Path $PSScriptRoot "support\racer_cpc_print.cpp") }
    @{ Name = "clThreading";       Rel = "Threading\clThreading.cpp" }
    @{ Name = "clShaderPool";      Rel = "Polygon\clShaderPool.cpp" }
    @{ Name = "clShaderReader";    Rel = "Polygon\clShaderReader.cpp" }
    @{ Name = "clShaders";         Rel = "Polygon\clShaders.cpp" }
    @{ Name = "clIndex";           Rel = "Math\clIndex.cpp" }
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
    $proc = Start-Process -FilePath $CpcPath -ArgumentList $args `
        -RedirectStandardOutput $log -RedirectStandardError $errLog `
        -PassThru -NoNewWindow
    # Force Process to retain its native handle.  Without this, Windows
    # PowerShell can report a null ExitCode after the child has exited.
    $processHandle = $proc.Handle
    $timedOut = $false
    if (-not $proc.WaitForExit($CompileTimeoutSeconds * 1000)) {
        $timedOut = $true
        Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue
        $exit = -999
        Add-Content -LiteralPath $log -Encoding ASCII `
            -Value ("cpc compile timed out after {0}s" -f $CompileTimeoutSeconds)
    } else {
        # Windows PowerShell can leave ExitCode unset after the timed wait until
        # the redirected output streams have drained and the process object is
        # refreshed.  Complete that bookkeeping before recording the result.
        $proc.WaitForExit()
        $proc.Refresh()
        $exit = $proc.ExitCode
    }
    if (Test-Path -LiteralPath $errLog) {
        Get-Content -LiteralPath $errLog | Add-Content -LiteralPath $log
    }
    $text = ""
    if (Test-Path -LiteralPath $log) {
        $raw = Get-Content -Raw -LiteralPath $log
        if ($raw) { $text = $raw }
    }
    $errors = ([regex]::Matches($text, "error:" )).Count
    $warnings = ([regex]::Matches($text, "warning:" )).Count
    $status = if ($timedOut) { " TIMEOUT" } else { "" }
    Write-Host ("{0,-22} exit={1,-4} warnings={2,-4} errors={3}{4}" -f $Label, $exit, $warnings, $errors, $status)
    if ($exit -ne 0) {
        $firstError = (Get-Content -LiteralPath $log | Select-String -Pattern "error:" | Select-Object -First 3) -join "`n"
        Write-Host $firstError
    }
    return $exit -eq 0
}

if (-not $LinkOnly) {
    # Remove stale objects from previous runs so every link is built from a
    # single consistent compiler and source state.
    $outFull = [System.IO.Path]::GetFullPath($OutDir).TrimEnd('\') + '\'
    foreach ($pattern in @("generated_*.obj", "additional_*.obj", "core_*.obj", "commonlib_*.obj")) {
        Get-ChildItem -LiteralPath $OutDir -Filter $pattern -ErrorAction SilentlyContinue | ForEach-Object {
            $full = [System.IO.Path]::GetFullPath($_.FullName)
            if ($full.StartsWith($outFull, [System.StringComparison]::OrdinalIgnoreCase)) {
                Remove-Item -LiteralPath $full -Force
            }
        }
    }
    $generatedSources = Get-Content -LiteralPath $sourcesFile | Where-Object { $_.Trim() } |
        ForEach-Object { $_.Trim().Trim('"') }
    $generatedOk = $true
    foreach ($src in $generatedSources) {
        $name = [System.IO.Path]::GetFileNameWithoutExtension($src)
        $ok = Invoke-CpcCompile -Label ("generated_" + $name) -Source $src
        if (-not $ok) { $generatedOk = $false }
    }
    Write-Host ("Generated source compile: " + $(if ($generatedOk) { "all OK" } else { "FAILURES" }))

    $additionalOk = $true
    foreach ($entry in $additionalSources) {
        $src = if ($entry.Source) { $entry.Source } else { Join-Path $commonLib ("src\" + $entry.Rel) }
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
    Write-Host ("Additional source compile: " + $(if ($additionalOk) { "all OK" } else { "FAILURES" }))
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

$expectedMachine = 0x8664
foreach ($dllName in $runtimeDllNames) {
    $source = Join-Path $runtimeDllDir $dllName
    $image = Get-PeImageInfo -Path $source
    if ($image.Machine -ne $expectedMachine) {
        throw ("Runtime DLL has the wrong architecture: {0} is {1}; expected x64" -f $source, $image.MachineName)
    }
    Copy-Item -LiteralPath $source -Destination (Join-Path $OutDir $dllName) -Force
}
Write-Host ("Runtime DLL bundle: " + $runtimeDllDir)

# Racer resolves resources below Assets/ relative to its current working
# directory.  Expose the complete source asset tree beside the executable so
# launching Racer.exe directly behaves the same as launching from CommonLib.
# A junction avoids copying the roughly 500 MB development asset tree on every
# compiler iteration while still making every resource available.
$packagedAssets = Join-Path $OutDir "Assets"
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

$tiffDll = Join-Path $OutDir "libtiff-5.dll"
$webpDll = Join-Path $OutDir "libwebp-7.dll"
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
        # Launch from an unrelated directory.  Racer startup must normalize its
        # working directory to the executable before resolving Assets/, while
        # Windows must still resolve the packaged DLLs beside Racer.exe.
        $runtimeWorkingDir = [System.IO.Path]::GetTempPath()
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
