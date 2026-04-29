param(
    [string]$RootPath = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
)

$ErrorActionPreference = "Stop"

$sdkInclude = Join-Path $RootPath "third-party\win32-sdk\include\winapi"
$sdkLib = Join-Path $RootPath "third-party\win32-sdk\lib"
$compilerExe = Join-Path $RootPath "cpc.exe"
if (-not (Test-Path $compilerExe)) {
    $compilerExe = Join-Path $RootPath "cpc.exe"
}

if (-not (Test-Path $sdkInclude)) { throw "Missing include path: $sdkInclude" }
if (-not (Test-Path $sdkLib)) { throw "Missing lib path: $sdkLib" }
if (-not (Test-Path $compilerExe)) { throw "Missing cpc.exe in root: $RootPath" }

$base = "https://raw.githubusercontent.com/mingw-w64/mingw-w64/master/mingw-w64-headers/include"
$headers = @(
    "commctrl.h",
    "objbase.h",
    "objidl.h",
    "objidlbase.h",
    "ole2.h",
    "oleauto.h",
    "propidl.h",
    "propkeydef.h",
    "propkey.h",
    "propsys.h",
    "servprov.h",
    "shlobj.h",
    "shobjidl.h",
    "shobjidl_core.h",
    "shlwapi.h",
    "shtypes.h",
    "strsafe.h",
    "unknwn.h",
    "wtypes.h",
    "wtypesbase.h"
)

foreach ($header in $headers) {
    $dest = Join-Path $sdkInclude $header
    if (Test-Path $dest) { continue }
    $url = "$base/$header"
    Write-Host "Fetching $header"
    try {
        Invoke-WebRequest -Uri $url -OutFile $dest
    } catch {
        Write-Warning "Skipping missing header: $header"
    }
}

$libs = @(
    "advapi32", "comctl32", "ole32", "oleaut32", "rpcrt4",
    "shlwapi", "windowscodecs", "shell32", "dwmapi"
)
foreach ($lib in $libs) {
    $def = Join-Path $sdkLib ($lib + ".def")
    if (Test-Path $def) { continue }
    $dll = Join-Path $env:WINDIR ("System32\" + $lib + ".dll")
    if (-not (Test-Path $dll)) { continue }
    Write-Host "Generating $lib.def"
    & $compilerExe -impdef $dll -o $def
    if ($LASTEXITCODE -ne 0) {
        throw "Failed generating $def"
    }
}

Write-Host "win32-sdk extension complete."
