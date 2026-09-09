param([string]$CompilerPath = (Join-Path $PSScriptRoot '../cpc.exe'))
$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$fixture = Join-Path $root ('build/cache-test-' + [guid]::NewGuid().ToString('N'))
[void][IO.Directory]::CreateDirectory($fixture)
$exe = Join-Path $fixture 'test.exe'
& $CompilerPath (Join-Path $PSScriptRoot 'tools/test_incremental_cache.c') -ladvapi32 -o $exe
if ($LASTEXITCODE) { throw 'Native cache test compilation failed' }
& $exe $fixture
if ($LASTEXITCODE) { throw 'Native cache regression failed' }
