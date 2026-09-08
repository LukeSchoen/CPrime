param(
    [string]$CompilerPath = '',
    [string]$RuntimeRoot = '',
    [string]$NativeCompilerPath = (Join-Path $PSScriptRoot '../third-party/clang/bin/clang.exe')
)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
if (-not $CompilerPath) { $CompilerPath = Join-Path $root 'cpc.exe' }
$compiler = (Resolve-Path -LiteralPath $CompilerPath).Path
$native = (Resolve-Path -LiteralPath $NativeCompilerPath).Path
$sources = Join-Path $PSScriptRoot 'abi\native_tls'
$commonArgs = @('-Werror')
if ($RuntimeRoot) { $commonArgs += '-B' + (Resolve-Path -LiteralPath $RuntimeRoot).Path }
$work = Join-Path ([IO.Path]::GetTempPath()) ('cprime-native-tls-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $work | Out-Null
try {
    $objects = @()
    # Reverse declaration order: the linker must order subsection contributions.
    foreach ($name in @('late', 'early', 'directory')) {
        $object = Join-Path $work ($name + '.obj')
        & $native --target=x86_64-pc-windows-msvc -std=c++17 -Werror -fno-rtti -fno-exceptions -fno-autolink `
            -c (Join-Path $sources ($name + '.cpp')) -o $object
        if ($LASTEXITCODE -ne 0) { throw "Native TLS source $name failed to compile." }
        $objects += $object
    }
    foreach ($partial in @($false, $true)) {
        $inputs = $objects
        if ($partial) {
            $combined = Join-Path $work 'combined.o'
            & $compiler @commonArgs -r @objects -o $combined
            if ($LASTEXITCODE -ne 0) { throw 'Native TLS partial link failed.' }
            $inputs = @($combined)
        }
        $executable = Join-Path $work ('tls-' + $partial + '.exe')
        & $compiler @commonArgs (Join-Path $sources 'consumer.cpp') @inputs -o $executable
        if ($LASTEXITCODE -ne 0) { throw "Native TLS link failed (partial=$partial)." }
        $output = @(& $executable)
        if ($LASTEXITCODE -ne 0) { throw "Native TLS executable returned $LASTEXITCODE (partial=$partial)." }
        $expected = @('tls-dtor', 'tls-dtor', 'atexit', 'preterminate', 'terminate')
        if (($output -join '|') -ne ($expected -join '|')) {
            throw "Incorrect native shutdown order: $($output -join ', ')"
        }
        Write-Output "PASS native TLS alignment, ordered callbacks, thread isolation, dynamic initialization, constructors and termination (partial=$partial)"
    }
} finally {
    $tempRoot = [IO.Path]::GetFullPath([IO.Path]::GetTempPath()).TrimEnd('\', '/') + [IO.Path]::DirectorySeparatorChar
    $resolvedWork = [IO.Path]::GetFullPath($work)
    if (-not $resolvedWork.StartsWith($tempRoot, [StringComparison]::OrdinalIgnoreCase) -or
        [IO.Path]::GetFileName($resolvedWork) -notlike 'cprime-native-tls-*') {
        throw "Refusing to remove an unexpected temporary directory: $resolvedWork"
    }
    if (Test-Path -LiteralPath $resolvedWork) { Remove-Item -LiteralPath $resolvedWork -Recurse -Force }
}
