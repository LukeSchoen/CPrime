param(
    [Parameter(Mandatory = $true)][string]$ExePath,
    [Parameter(Mandatory = $true)][string]$RootPath,
    [Parameter(Mandatory = $true)][string]$RuntimeLibPath,
    [ValidateSet("none", "full")][string]$Profile = "none",
    [string]$CacheDir = '',
    [switch]$PrepareOnly
)

$ErrorActionPreference = "Stop"

function New-TempDir {
    $base = [System.IO.Path]::Combine([System.IO.Path]::GetTempPath(), "cprime-pack-" + [System.Guid]::NewGuid().ToString("N"))
    [System.IO.Directory]::CreateDirectory($base) | Out-Null
    return $base
}

function Copy-Tree {
    param([string]$From, [string]$To)
    if (-not (Test-Path $From)) {
        throw "Source path not found: $From"
    }
    [System.IO.Directory]::CreateDirectory($To) | Out-Null
    Copy-Item -Path (Join-Path $From "*") -Destination $To -Recurse -Force
}

function Ensure-ImpDef {
    param([string]$TccExe, [string]$LibName, [string]$OutDir)
    $def = Join-Path $OutDir ($LibName + ".def")
    if (Test-Path $def) {
        return
    }
    $dll = Join-Path $env:WINDIR ("System32\" + $LibName + ".dll")
    if (-not (Test-Path $dll)) {
        throw "Missing system DLL for import-def generation: $dll"
    }
    & $TccExe -impdef $dll -o $def
    if ($LASTEXITCODE -ne 0 -or -not (Test-Path $def)) {
        throw "Failed to generate import definition for $LibName"
    }
}

$ExePath = [System.IO.Path]::GetFullPath($ExePath)
$RootPath = [System.IO.Path]::GetFullPath($RootPath)
$RuntimeLibPath = [System.IO.Path]::GetFullPath($RuntimeLibPath)

if (-not (Test-Path $ExePath)) { throw "ExePath not found: $ExePath" }
if (-not (Test-Path $RootPath)) { throw "RootPath not found: $RootPath" }
if (-not (Test-Path $RuntimeLibPath)) { throw "RuntimeLibPath not found: $RuntimeLibPath" }

if ($Profile -eq "none") {
    return
}

if (-not $CacheDir) { $CacheDir = Join-Path $RootPath 'build/portable-cache' }
$CacheDir = [IO.Path]::GetFullPath($CacheDir)
$cachedHelper = Join-Path $CacheDir 'portable-payload.exe'
$cachedPayload = Join-Path $CacheDir 'payload.bin'
if (Test-Path -LiteralPath $cachedHelper) {
    $hit = $false
    try {
        if ($PrepareOnly) { & $cachedHelper check $CacheDir $RootPath $RuntimeLibPath 2>$null }
        else { & $cachedHelper cached $ExePath $CacheDir $RootPath $RuntimeLibPath 2>$null }
        $hit = $LASTEXITCODE -eq 0
    } catch { $hit = $false }
    if ($hit) { return }
}
New-Item -ItemType Directory -Force -Path $CacheDir | Out-Null
# Publish the input digest last. A failed preparation cannot authorize old data.
$stamp = Join-Path $CacheDir 'inputs.sha256'
if (Test-Path -LiteralPath $stamp) { Remove-Item -LiteralPath $stamp }
Write-Host 'Portable payload: preparing SDK and runtime'

$stage = New-TempDir
try {
    $toolDir = Join-Path $stage "tools"
    [System.IO.Directory]::CreateDirectory($toolDir) | Out-Null
    $helper = Join-Path $toolDir "portable-payload.exe"
    $helperSource = Join-Path $PSScriptRoot "../../src/tools/portable_payload.c"
    & $ExePath "-B$RootPath" "-I$RootPath/include/runtime" "-I$RootPath/third-party/win32-sdk/include" "-I$RootPath/third-party/win32-sdk/include/winapi" -O2 $helperSource -o $helper
    if ($LASTEXITCODE -ne 0) { throw "Failed to build native packaging helper" }

    $stageInclude = Join-Path $stage "include"
    $stageLib = Join-Path $stage "lib"
    [System.IO.Directory]::CreateDirectory($stageInclude) | Out-Null
    [System.IO.Directory]::CreateDirectory($stageLib) | Out-Null

    # Keep the complete vendored SDK, including transitive Windows headers.
    # A curated filename list silently breaks otherwise valid include chains.
    Copy-Tree -From (Join-Path $RootPath "third-party\win32-sdk\include") -To $stageInclude
    Copy-Tree -From (Join-Path $RootPath "include\runtime") -To $stageInclude

    # Runtime overlays are the public CPC headers. Use those same declarations
    # for qualified winapi includes, instead of shipping a second, conflicting
    # COM interface implementation behind the normal include search path.
    foreach ($header in Get-ChildItem -LiteralPath (Join-Path $RootPath 'include/runtime') -Filter '*.h' -File) {
        $duplicate = Join-Path $stageInclude ('winapi/' + $header.Name)
        if (Test-Path -LiteralPath $duplicate -PathType Leaf) {
            [IO.File]::WriteAllText($duplicate, ('#include "../' + $header.Name + '"' + "`n"), [Text.Encoding]::ASCII)
        }
    }

    & $helper headers $stageInclude
    if ($LASTEXITCODE -ne 0) { throw "Failed to normalize portable headers" }

    Copy-Tree -From (Join-Path $RootPath "third-party\win32-sdk\lib") -To $stageLib
    Copy-Item -Path (Join-Path $RuntimeLibPath "*") -Destination $stageLib -Force

    # Check the selected archive, not the runtime used to build this helper.
    # Root lib/ may intentionally contain a bootstrap-only __chkstk which is
    # incompatible with native COFF objects despite linking successfully.
    $runtimeCheck = Join-Path $toolDir 'check-runtime-abi.exe'
    & $ExePath "-B$stage" `
        (Join-Path $PSScriptRoot '../../src/tools/check_runtime_abi.c') `
        (Join-Path $PSScriptRoot '../../src/tools/check_runtime_abi.S') `
        (Join-Path $stageLib 'libcprime1.a') -o $runtimeCheck
    if ($LASTEXITCODE -ne 0) { throw 'Failed to build the runtime ABI check' }
    & $runtimeCheck
    if ($LASTEXITCODE -ne 0) { throw "Refusing to package an incompatible runtime: $RuntimeLibPath" }

    $requiredLibs = @(
        "kernel32", "user32", "gdi32", "ws2_32", "msvcrt", "ucrtbase",
        "comdlg32", "shell32", "uxtheme", "dwmapi", "msimg32",
        "advapi32", "comctl32", "ole32", "oleaut32", "rpcrt4",
        "shlwapi", "windowscodecs", "imm32", "shcore"
    )
    foreach ($lib in $requiredLibs) {
        Ensure-ImpDef -TccExe $ExePath -LibName $lib -OutDir $stageLib
    }

    # Keep the established ordering/UTF-8 paths; serialize and compress in C.
    $entries = Get-ChildItem -Path $stageInclude, $stageLib -File -Recurse | Sort-Object FullName
    $paths = @($entries | ForEach-Object { $_.FullName.Substring($stage.Length + 1).Replace('\', '/') })
    $manifest = Join-Path $toolDir "files.txt"
    [System.IO.File]::WriteAllLines($manifest, [string[]]$paths, [System.Text.UTF8Encoding]::new($false))
    $prepared = Join-Path $toolDir 'payload.bin'
    & $helper prepare $prepared $stage $manifest
    if ($LASTEXITCODE -ne 0) { throw 'Failed to prepare portable payload' }
    Copy-Item -LiteralPath $prepared -Destination $cachedPayload -Force
    Copy-Item -LiteralPath $helper -Destination $cachedHelper -Force
    & $cachedHelper seal $CacheDir $RootPath $RuntimeLibPath
    if ($LASTEXITCODE -ne 0) { throw 'Failed to record portable payload dependencies' }
    if (-not $PrepareOnly) {
        & $cachedHelper attach $ExePath $cachedPayload
        if ($LASTEXITCODE -ne 0) { throw 'Failed to append portable payload' }
    }

} finally {
    if (Test-Path $stage) {
        $temporaryRoot = [IO.Path]::GetFullPath([IO.Path]::GetTempPath()).TrimEnd('\', '/') + '\'
        $resolvedStage = [IO.Path]::GetFullPath($stage)
        if (-not $resolvedStage.StartsWith($temporaryRoot, [StringComparison]::OrdinalIgnoreCase) -or
            [IO.Path]::GetFileName($resolvedStage) -notlike 'cprime-pack-*') {
            throw "Refusing to remove an unexpected packaging directory: $resolvedStage"
        }
        Remove-Item -LiteralPath $resolvedStage -Recurse -Force
    }
}
