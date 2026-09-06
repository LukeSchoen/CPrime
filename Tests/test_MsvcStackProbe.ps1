param(
    [string]$CompilerPath = '',
    [string]$RuntimeRoot = '',
    [string]$NativeCompilerPath = 'C:\Luke\Src\OT\cl\CommonLib\Assets\Programs\Clang\clang.exe'
)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
if (-not $CompilerPath) { $CompilerPath = Join-Path $root 'cpc.exe' }
$compiler = (Resolve-Path -LiteralPath $CompilerPath).Path
$native = (Resolve-Path -LiteralPath $NativeCompilerPath).Path
$sources = Join-Path $PSScriptRoot 'abi\msvc_stack_probe'
$commonArgs = @('-Werror')
if ($RuntimeRoot) { $commonArgs += '-B' + (Resolve-Path -LiteralPath $RuntimeRoot).Path }
$work = Join-Path ([IO.Path]::GetTempPath()) ('cprime-msvc-native-runtime-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $work | Out-Null
try {
    foreach ($optimization in @('-O0', '-O2')) {
        foreach ($nativeDefinition in @($true, $false)) {
            $nativeSource = if ($nativeDefinition) { 'provider.cpp' } else { 'consumer.cpp' }
            $primeSource = if ($nativeDefinition) { 'consumer.cpp' } else { 'provider.cpp' }
            $nativeObject = Join-Path $work 'native.obj'
            & $native --target=x86_64-pc-windows-msvc -std=c++17 -Werror -fno-rtti -fno-exceptions -fno-autolink `
                $optimization -c (Join-Path $sources $nativeSource) -o $nativeObject
            if ($LASTEXITCODE -ne 0) { throw "Native native runtime fixture failed: $nativeSource $optimization." }
            $executable = Join-Path $work 'native-runtime.exe'
            & $compiler @commonArgs (Join-Path $sources $primeSource) $nativeObject -o $executable
            if ($LASTEXITCODE -ne 0) { throw "Mixed native runtime build failed: native $nativeSource $optimization." }
            & $executable
            if ($LASTEXITCODE -ne 0) { throw "Mixed native runtime executable returned ${LASTEXITCODE}: native $nativeSource $optimization." }
            Write-Output "PASS native stack probing and constructor vtable ownership: native $nativeSource $optimization"
        }
    }
    $executable = Join-Path $work 'native-runtime-prime.exe'
    & $compiler @commonArgs (Join-Path $sources 'provider.cpp') (Join-Path $sources 'consumer.cpp') -o $executable
    if ($LASTEXITCODE -ne 0) { throw 'CPC native runtime multiple-input fixture failed to build.' }
    & $executable
    if ($LASTEXITCODE -ne 0) { throw "CPC native runtime multiple-input executable returned $LASTEXITCODE." }
    Write-Output 'PASS native runtime multiple-input CPC build'
} finally {
    $tempRoot = [IO.Path]::GetFullPath([IO.Path]::GetTempPath()).TrimEnd('\', '/') + [IO.Path]::DirectorySeparatorChar
    $resolvedWork = [IO.Path]::GetFullPath($work)
    if (-not $resolvedWork.StartsWith($tempRoot, [StringComparison]::OrdinalIgnoreCase) -or
        [IO.Path]::GetFileName($resolvedWork) -notlike 'cprime-msvc-native-runtime-*') {
        throw "Refusing to remove an unexpected temporary directory: $resolvedWork"
    }
    if (Test-Path -LiteralPath $resolvedWork) { Remove-Item -LiteralPath $resolvedWork -Recurse -Force }
}
