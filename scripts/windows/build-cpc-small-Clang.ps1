param(
    [string]$OutDir = '',
    [string]$RuntimeLibPath = '',
    [switch]$Test
)
$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
if (-not $OutDir) { $OutDir = Join-Path $root 'build/compiler-small' }
if (-not $RuntimeLibPath) { $RuntimeLibPath = Join-Path $root 'build/compiler/lib' }
$OutDir = [IO.Path]::GetFullPath($OutDir)
$RuntimeLibPath = (Resolve-Path -LiteralPath $RuntimeLibPath).Path
# Use a built runtime, not root lib/'s bootstrap stack-probe ABI. The packer
# validates it before including every runtime object and SDK header.
& (Join-Path $root 'BuildProfile/build-cpc-clang.ps1') -OutDir $OutDir -Optimization z -SystemCRT -Map
if ($LASTEXITCODE) { throw 'Small compiler host build failed' }
$compiler = Join-Path $OutDir 'cpc.exe'
Move-Item -LiteralPath (Join-Path $OutDir 'cpc-clang.exe') -Destination $compiler -Force
& (Join-Path $PSScriptRoot 'pack-portable.ps1') -ExePath $compiler -RootPath $root -RuntimeLibPath $RuntimeLibPath -Profile full
& (Join-Path $PSScriptRoot 'measure-compiler-size.ps1') -ExePath $compiler -JsonPath (Join-Path $OutDir 'size.json') |
    Select-Object Path, TotalBytes, PEBytes, PortableBytes | Format-List
if ($Test) {
    & (Join-Path $root 'Tests/test_PortablePackaging.ps1') -CompilerPath $compiler -RuntimeLibPath $RuntimeLibPath
}
