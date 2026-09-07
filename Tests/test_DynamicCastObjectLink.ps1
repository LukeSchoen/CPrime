param([string]$CompilerPath = '', [string]$RuntimeRoot = '')
$ErrorActionPreference = 'Stop'
if (!$CompilerPath) { $CompilerPath = Join-Path $PSScriptRoot '..\cpc.exe' }
$compiler = (Resolve-Path -LiteralPath $CompilerPath).Path
$work = Join-Path $PSScriptRoot '..\build\tests\dynamic-cast-object'
[IO.Directory]::CreateDirectory($work) | Out-Null
$arguments = @('-Werror')
if ($RuntimeRoot) { $arguments += '-B' + (Resolve-Path -LiteralPath $RuntimeRoot).Path }
$objects = @()
foreach ($name in @('dynamic_cast_link_main', 'dynamic_cast_link_other')) {
    $source = Join-Path $PSScriptRoot ('features\Classes\pass\' + $name + '.cpp')
    $object = Join-Path $work ($name + '.obj')
    & $compiler @arguments -c $source -o $object
    if ($LASTEXITCODE -ne 0) { throw 'Dynamic-cast object compilation failed.' }
    $objects += $object
}
$executable = Join-Path $work 'dynamic_cast.exe'
& $compiler @arguments @objects -o $executable
if ($LASTEXITCODE -ne 0) { throw 'Dynamic-cast object linking failed.' }
& $executable
if ($LASTEXITCODE -ne 0) { throw 'Cross-translation-unit runtime type check failed.' }
Write-Output 'PASS cross-translation-unit dynamic_cast'
