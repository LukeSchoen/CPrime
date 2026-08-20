param(
    [string]$CpcPath = "",
    [string]$ProjectRoot = "C:\Luke\Src\OT\cl",
    [string]$OutDir = "",
    [switch]$LinkOnly,
    [switch]$SkipLink
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
    $OutDir = Join-Path $ProjectRoot "builds\split_new"
}
$OutDir = [System.IO.Path]::GetFullPath($OutDir)

if (-not (Test-Path -LiteralPath $CpcPath)) { throw "Compiler not found: $CpcPath" }
if (-not (Test-Path -LiteralPath $ProjectRoot)) { throw "Project root not found: $ProjectRoot" }
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null

$commonLib = Join-Path $ProjectRoot "CommonLib\commonLib"
$flagsFile = Join-Path $ProjectRoot "builds\core\generated_core_cpc_flags.rsp"
$sourcesFile = Join-Path $ProjectRoot "builds\core\generated_core_sources.rsp"
$sdl2Def = Join-Path $ProjectRoot "builds\split\SDL2.def"
$runtimeLib = Join-Path $env:LOCALAPPDATA "cpc\1.4\lib\libcprime1.a"

if (-not (Test-Path -LiteralPath $flagsFile)) { throw "Missing flags rsp: $flagsFile" }
if (-not (Test-Path -LiteralPath $sourcesFile)) { throw "Missing sources rsp: $sourcesFile" }
if (-not (Test-Path -LiteralPath $sdl2Def)) { throw "Missing SDL2 import def: $sdl2Def" }

$flags = Get-Content -LiteralPath $flagsFile | Where-Object { $_.Trim() }

# Extra CommonLib implementation sources compiled separately because the
# generated core source list does not include them.
$extraCommonLibSources = @(
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
    @{ Name = "clMemoryStream";     Rel = "Streams\clMemoryStream.cpp" }
    @{ Name = "clRawFileStream";    Rel = "Streams\clRawFileStream.cpp" }
    @{ Name = "clString";          Rel = "Strings\clString.cpp" }
    @{ Name = "clTinyGLWrangler";  Rel = "Polygon\clTinyGLWrangler.cpp" }
    @{ Name = "clWindow";          Rel = "Raster\clWindow.cpp" }
    @{ Name = "clColorAccess";     Rel = "Raster\clColorAccess.cpp" }
    @{ Name = "clRelAssert";       Rel = "Platform\clRelAssert.cpp" }
    @{ Name = "clPath";            Rel = "Platform\clPath.cpp" }
    @{ Name = "clFolder";          Rel = "Platform\clFolder.cpp" }
    @{ Name = "clDecryptThis";     Rel = "Encryption\clDecryptThis.cpp" }
    @{ Name = "clCamera";          Rel = "UI\clCamera.cpp" }
    @{ Name = "clImage";           Rel = "Raster\clImage.cpp" }
    @{ Name = "clRenderObjectCore"; Rel = "Polygon\clRenderObjectCore.cpp" }
    @{ Name = "clRenderObject";    Rel = "Polygon\clRenderObject.cpp" }
)

function Invoke-CpcCompile {
    param(
        [string]$Label,
        [string]$Source,
        [string[]]$ExtraFlags
    )

    $obj = Join-Path $OutDir ($Label + ".obj")
    $log = Join-Path $OutDir ($Label + ".obj.log")
    $args = @($flags) + @($ExtraFlags) + @("-c", $Source, "-o", $obj)
    $savedEap = $ErrorActionPreference
    $ErrorActionPreference = "Continue"
    try {
        & $CpcPath @args *> $log 2>&1
        $exit = $LASTEXITCODE
    } finally {
        $ErrorActionPreference = $savedEap
    }
    $text = ""
    if (Test-Path -LiteralPath $log) {
        $raw = Get-Content -Raw -LiteralPath $log
        if ($raw) { $text = $raw }
    }
    $errors = ([regex]::Matches($text, "error:" )).Count
    $warnings = ([regex]::Matches($text, "warning:" )).Count
    Write-Host ("{0,-22} exit={1,-3} warnings={2,-4} errors={3}" -f $Label, $exit, $warnings, $errors)
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
    foreach ($pattern in @("core_*.obj", "commonlib_*.obj")) {
        Get-ChildItem -LiteralPath $OutDir -Filter $pattern -ErrorAction SilentlyContinue | ForEach-Object {
            $full = [System.IO.Path]::GetFullPath($_.FullName)
            if ($full.StartsWith($outFull, [System.StringComparison]::OrdinalIgnoreCase)) {
                Remove-Item -LiteralPath $full -Force
            }
        }
    }
    $coreSources = Get-Content -LiteralPath $sourcesFile | Where-Object { $_.Trim() } |
        ForEach-Object { $_.Trim().Trim('"') }
    $coreOk = $true
    foreach ($src in $coreSources) {
        $name = [System.IO.Path]::GetFileNameWithoutExtension($src)
        $ok = Invoke-CpcCompile -Label ("core_" + $name) -Source $src
        if (-not $ok) { $coreOk = $false }
    }
    Write-Host ("Core compile: " + $(if ($coreOk) { "all OK" } else { "FAILURES" }))

    $extraCommonLibOk = $true
    foreach ($entry in $extraCommonLibSources) {
        $src = Join-Path $commonLib ("src\" + $entry.Rel)
        $ok = Invoke-CpcCompile -Label ("commonlib_" + $entry.Name) -Source $src
        if (-not $ok) { $extraCommonLibOk = $false }
    }
    Write-Host ("Extra CommonLib compile: " + $(if ($extraCommonLibOk) { "all OK" } else { "FAILURES" }))
}

if ($SkipLink) {
    exit 0
}

# Link all objects that exist in the output directory. cpc's @file syntax
# runs each line of the file as one job, so the whole link command must be a
# single line in the batch file.
$objs = @()
$objs += Get-ChildItem -LiteralPath $OutDir -Filter "core_*.obj" | Sort-Object Name
$objs += Get-ChildItem -LiteralPath $OutDir -Filter "commonlib_*.obj" | Sort-Object Name
$linkBat = Join-Path $OutDir "link_cmd.bat"

$def = Join-Path $OutDir "racer_test.def"
@"
LIBRARY racer_test.exe

EXPORTS
AmdPowerXpressRequestHighPerformance
NvOptimusEnablement
"@ | Set-Content -LiteralPath $def -Encoding ASCII

$exe = Join-Path $OutDir "Racer.exe"
$linkLog = Join-Path $OutDir "link.log"
$linkTokens = @()
foreach ($obj in $objs) { $linkTokens += $obj.FullName }
$linkTokens += @(
    $def,
    $sdl2Def,
    $runtimeLib,
    "-luser32", "-lgdi32", "-lopengl32", "-lws2_32", "-lmsvcrt",
    "-limm32", "-lshcore",
    "-o", $exe
)
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
$undefined = [regex]::Matches($linkText, "undefined symbol '([^']+)'") |
    ForEach-Object { $_.Groups[1].Value } |
    Sort-Object -Unique
$undefinedFile = Join-Path $OutDir "link_undefined.txt"
$undefined | Set-Content -LiteralPath $undefinedFile -Encoding ASCII

Write-Host ""
Write-Host ("Link exit: {0}" -f $exit)
Write-Host ("Undefined symbols: {0}" -f $undefined.Count)
$undefined | ForEach-Object { Write-Host ("  " + $_) }
if (Test-Path -LiteralPath $exe) {
    Write-Host ("Executable: " + $exe)
} else {
    Write-Host "Executable: NOT PRODUCED"
}
exit 0
