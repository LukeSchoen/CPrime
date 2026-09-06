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
$sources = Join-Path $PSScriptRoot 'abi\msvc_layout'
$commonArgs = @('-Werror')
if ($RuntimeRoot) { $commonArgs += '-B' + (Resolve-Path -LiteralPath $RuntimeRoot).Path }
$work = Join-Path ([IO.Path]::GetTempPath()) ('cprime-msvc-layout-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $work | Out-Null
try {
    # This gate isolates record layout and ordinary virtual dispatch. RTTI,
    # deleting destructors and exception propagation require separate ABI gates.
    $provider = Join-Path $work 'provider.obj'
    & $native --target=x86_64-pc-windows-msvc -std=c++17 -Werror -fno-rtti -fno-exceptions -fno-autolink `
        -c (Join-Path $sources 'provider.cpp') -o $provider
    if ($LASTEXITCODE -ne 0) { throw 'Native class-layout provider failed to compile.' }
    $executable = Join-Path $work 'layout.exe'
    $savedPreference = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    try {
        $output = & $compiler @commonArgs (Join-Path $sources 'consumer.cpp') $provider -o $executable 2>&1
        $result = $LASTEXITCODE
    } finally { $ErrorActionPreference = $savedPreference }
    if ($result -ne 0) { throw "Mixed class-layout build failed (exit $result):`n$($output -join "`n")" }
    if (-not (Test-Path -LiteralPath $executable -PathType Leaf)) { throw 'Class-layout executable missing.' }
    & $executable
    if ($LASTEXITCODE -ne 0) { throw "Mixed class-layout executable returned $LASTEXITCODE." }
    Write-Output 'PASS native/CPC object layout and virtual dispatch in both directions'
} finally {
    $tempRoot = [IO.Path]::GetFullPath([IO.Path]::GetTempPath()).TrimEnd('\', '/') + [IO.Path]::DirectorySeparatorChar
    $resolvedWork = [IO.Path]::GetFullPath($work)
    if (-not $resolvedWork.StartsWith($tempRoot, [StringComparison]::OrdinalIgnoreCase) -or
        [IO.Path]::GetFileName($resolvedWork) -notlike 'cprime-msvc-layout-*') {
        throw "Refusing to remove an unexpected temporary directory: $resolvedWork"
    }
    if (Test-Path -LiteralPath $resolvedWork) { Remove-Item -LiteralPath $resolvedWork -Recurse -Force }
}
