param(
    [string]$CompilerPath = '',
    [string]$RuntimeRoot = '',
    [string]$NativeCompilerPath = ''
)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
if (-not $CompilerPath) { $CompilerPath = Join-Path $root 'cpc.exe' }
$compiler = (Resolve-Path -LiteralPath $CompilerPath).Path
if (-not $NativeCompilerPath) {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (-not (Test-Path -LiteralPath $vswhere)) { throw 'MSVC is required; specify -NativeCompilerPath.' }
    $installation = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if (-not $installation) { throw 'No MSVC C++ toolchain found.' }
    $versionFile = Join-Path $installation 'VC\Auxiliary\Build\Microsoft.VCToolsVersion.default.txt'
    $version = (Get-Content -LiteralPath $versionFile -Raw).Trim()
    $NativeCompilerPath = Join-Path $installation "VC\Tools\MSVC\$version\bin\Hostx64\x64\cl.exe"
}
$native = (Resolve-Path -LiteralPath $NativeCompilerPath).Path
$sources = Join-Path $PSScriptRoot 'abi\msvc_record_return'
$commonArgs = @('-Werror')
if ($RuntimeRoot) { $commonArgs += '-B' + (Resolve-Path -LiteralPath $RuntimeRoot).Path }
$work = Join-Path ([IO.Path]::GetTempPath()) ('cprime-msvc-record-return-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $work | Out-Null
try {
    $provider = Join-Path $work 'provider.obj'
    # MSVC's current trivial-record rule differs from Clang 12's older POD rule.
    # Compile the oracle with MSVC itself, without importing its default CRT.
    & $native /nologo /std:c++17 /c /Od /GS- /GR- /EHsc /Zl "/Fo$provider" (Join-Path $sources 'provider.cpp')
    if ($LASTEXITCODE -ne 0) { throw 'MSVC record-return provider failed to compile.' }
    $executable = Join-Path $work 'record-return.exe'
    & $compiler @commonArgs (Join-Path $sources 'consumer.cpp') $provider -o $executable
    if ($LASTEXITCODE -ne 0) { throw 'Mixed record-return build failed.' }
    if (-not (Test-Path -LiteralPath $executable -PathType Leaf)) { throw 'Record-return executable missing.' }
    & $executable
    if ($LASTEXITCODE -ne 0) { throw "Mixed record-return executable returned $LASTEXITCODE." }
    Write-Output 'PASS MSVC/CPC free record returns: public, private, protected, inherited, defaulted, and nested special members'
} finally {
    $tempRoot = [IO.Path]::GetFullPath([IO.Path]::GetTempPath()).TrimEnd('\', '/') + [IO.Path]::DirectorySeparatorChar
    $resolvedWork = [IO.Path]::GetFullPath($work)
    if (-not $resolvedWork.StartsWith($tempRoot, [StringComparison]::OrdinalIgnoreCase) -or
        [IO.Path]::GetFileName($resolvedWork) -notlike 'cprime-msvc-record-return-*') {
        throw "Refusing to remove an unexpected temporary directory: $resolvedWork"
    }
    if (Test-Path -LiteralPath $resolvedWork) { Remove-Item -LiteralPath $resolvedWork -Recurse -Force }
}
