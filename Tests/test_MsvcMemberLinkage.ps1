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
$sources = Join-Path $PSScriptRoot 'abi\msvc_members'
$commonArgs = @('-Werror')
if ($RuntimeRoot) { $commonArgs += '-B' + (Resolve-Path -LiteralPath $RuntimeRoot).Path }
$work = Join-Path ([IO.Path]::GetTempPath()) ('cprime-msvc-members-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $work | Out-Null
try {
    $provider = Join-Path $work 'provider.obj'
    & $native --target=x86_64-pc-windows-msvc -std=c++17 -Werror -fno-rtti -fno-exceptions -fno-autolink `
        -c (Join-Path $sources 'provider.cpp') -o $provider
    if ($LASTEXITCODE -ne 0) { throw 'Native member provider failed to compile.' }
    $executable = Join-Path $work 'members.exe'
    $savedPreference = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    try {
        $output = & $compiler @commonArgs (Join-Path $sources 'consumer.cpp') $provider -o $executable 2>&1
        $result = $LASTEXITCODE
    } finally { $ErrorActionPreference = $savedPreference }
    if ($result -ne 0) { throw "Mixed member build failed (exit $result):`n$($output -join "`n")" }
    if (-not (Test-Path -LiteralPath $executable -PathType Leaf)) { throw 'Member executable missing.' }
    & $executable
    if ($LASTEXITCODE -ne 0) { throw "Mixed member executable returned $LASTEXITCODE." }
    Write-Output 'PASS native/CPC constructors, destructors, operators, access, nested names, template and scalar signatures, callbacks, and member record returns'
} finally {
    $tempRoot = [IO.Path]::GetFullPath([IO.Path]::GetTempPath()).TrimEnd('\', '/') + [IO.Path]::DirectorySeparatorChar
    $resolvedWork = [IO.Path]::GetFullPath($work)
    if (-not $resolvedWork.StartsWith($tempRoot, [StringComparison]::OrdinalIgnoreCase) -or
        [IO.Path]::GetFileName($resolvedWork) -notlike 'cprime-msvc-members-*') {
        throw "Refusing to remove an unexpected temporary directory: $resolvedWork"
    }
    if (Test-Path -LiteralPath $resolvedWork) { Remove-Item -LiteralPath $resolvedWork -Recurse -Force }
}
