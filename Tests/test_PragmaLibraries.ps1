param(
    [string]$CompilerPath = '',
    [string]$RuntimeRoot = ''
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
if (-not $CompilerPath) { $CompilerPath = Join-Path $root 'cpc.exe' }
$compiler = (Resolve-Path -LiteralPath $CompilerPath).Path
$commonArgs = @()
if ($RuntimeRoot) { $commonArgs += '-B' + (Resolve-Path -LiteralPath $RuntimeRoot).Path }
$work = Join-Path ([IO.Path]::GetTempPath()) ('cprime-pragma-tests-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $work | Out-Null

function Compile([string]$Label, [string[]]$Arguments) {
    $savedPreference = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    try {
        $output = & $compiler @commonArgs @Arguments 2>&1
        $result = $LASTEXITCODE
    } finally { $ErrorActionPreference = $savedPreference }
    if ($result -ne 0) { throw "$Label failed (exit $result):`n$($output -join "`n")" }
}

function Check-Executable([string]$Label, [string]$Path) {
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) { throw "$Label did not produce an executable." }
    & $Path
    if ($LASTEXITCODE -ne 0) { throw "$Label returned $LASTEXITCODE." }
    Write-Output "PASS $Label"
}

try {
    Push-Location $work
    try {
        New-Item -ItemType Directory 'dependencies' | Out-Null
        Set-Content -Encoding ASCII 'provider.c' 'int library_value(void) { return 42; }'
        Set-Content -Encoding ASCII 'consumer.cpp' @'
#define DEPENDENCY_DIRECTORY "dependencies/"
#pragma comment(lib, DEPENDENCY_DIRECTORY "provider library.lib")
#pragma comment(lib, DEPENDENCY_DIRECTORY "provider library.lib")
extern "C" int library_value();
int consume_library() { return library_value(); }
'@
        Set-Content -Encoding ASCII 'main.cpp' 'int consume_library(); int main() { return consume_library() != 42; }'
        Compile 'provider object' @('-c', 'provider.c', '-o', 'provider.obj')
        Compile 'provider archive' @('-ar', 'rcs', 'dependencies/provider library.lib', 'provider.obj')
        Compile 'source pragma dependency' @('main.cpp', 'consumer.cpp', '-o', 'source.exe')
        Check-Executable 'source pragma dependency with concatenated, quoted relative path' (Join-Path $work 'source.exe')

        Compile 'consumer object' @('-c', 'consumer.cpp', '-o', 'consumer.obj')
        Compile 'object pragma dependency' @('main.cpp', 'consumer.obj', '-o', 'object.exe')
        Check-Executable 'separate object preserves pragma dependency' (Join-Path $work 'object.exe')

        Compile 'consumer archive' @('-ar', 'rcs', 'consumer.lib', 'consumer.obj')
        Compile 'archived pragma dependency' @('main.cpp', 'consumer.lib', '-o', 'archive.exe')
        Check-Executable 'archive extraction preserves pragma dependency' (Join-Path $work 'archive.exe')

        Compile 'relocatable object' @('-r', 'consumer.obj', '-o', 'relocated.obj')
        Compile 'relocated pragma dependency' @('main.cpp', 'relocated.obj', '-o', 'relocated.exe')
        Check-Executable 'relocatable link preserves pragma dependency' (Join-Path $work 'relocated.exe')

        Set-Content -Encoding ASCII 'named.cpp' @'
#pragma comment(lib, "provider library.lib")
extern "C" int library_value();
int main() { return library_value() != 42; }
'@
        Compile 'named object' @('-c', 'named.cpp', '-o', 'named.obj')
        Compile 'named library search' @('named.obj', '-Ldependencies', '-o', 'named.exe')
        Check-Executable 'bare library names retain library-path search' (Join-Path $work 'named.exe')
    } finally { Pop-Location }
} finally {
    $tempRoot = [IO.Path]::GetFullPath([IO.Path]::GetTempPath()).TrimEnd('\', '/') + [IO.Path]::DirectorySeparatorChar
    $resolvedWork = [IO.Path]::GetFullPath($work)
    if (-not $resolvedWork.StartsWith($tempRoot, [StringComparison]::OrdinalIgnoreCase) -or
        [IO.Path]::GetFileName($resolvedWork) -notlike 'cprime-pragma-tests-*') {
        throw "Refusing to remove an unexpected temporary directory: $resolvedWork"
    }
    if (Test-Path -LiteralPath $resolvedWork) { Remove-Item -LiteralPath $resolvedWork -Recurse -Force }
}
