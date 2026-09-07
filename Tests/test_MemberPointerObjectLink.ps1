param([string]$CompilerPath = '', [string]$RuntimeRoot = '')
$ErrorActionPreference = 'Stop'
if (!$CompilerPath) { $CompilerPath = Join-Path $PSScriptRoot '..\cpc.exe' }
$compiler = (Resolve-Path -LiteralPath $CompilerPath).Path
$work = Join-Path $PSScriptRoot 'out\member-pointer-object'
[IO.Directory]::CreateDirectory($work) | Out-Null
$object = Join-Path $work 'worker.obj'
$executable = Join-Path $work 'worker.exe'
$arguments = @('-Werror')
if ($RuntimeRoot) { $arguments += '-B' + (Resolve-Path -LiteralPath $RuntimeRoot).Path }
$source = Join-Path $PSScriptRoot 'features\StdConcurrency\pass\test_thread_deferred_member_address.cpp'
& $compiler @arguments -c $source -o $object
if ($LASTEXITCODE -ne 0) { throw 'Member-pointer object compilation failed.' }
& $compiler @arguments $object -o $executable
if ($LASTEXITCODE -ne 0) { throw 'Member-pointer object linking failed.' }
& $executable
if ($LASTEXITCODE -ne 0) { throw 'Deferred worker member function did not run correctly.' }
Write-Output 'PASS separately compiled member-pointer relocation and deferred thread callback'
