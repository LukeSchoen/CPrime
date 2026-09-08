param([string]$CompilerPath = (Join-Path $PSScriptRoot '../cpc.exe'))
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$compiler = (Resolve-Path -LiteralPath $CompilerPath).Path
$work = Join-Path $root ('build/pack cache ' + [guid]::NewGuid().ToString('N'))
$fixture = Join-Path $work 'root'
$cache = Join-Path $fixture 'build/portable-cache'
$runtime = Join-Path $fixture 'build/runtime'
$target = Join-Path $fixture 'compiler.exe'
function Copy-Fixture([string]$Relative) {
    $destination = Join-Path $fixture $Relative
    New-Item -ItemType Directory -Path (Split-Path $destination -Parent) -Force | Out-Null
    Copy-Item -LiteralPath (Join-Path $root $Relative) -Destination $destination -Recurse
}
function Prepare-Package {
    & (Join-Path $fixture 'scripts/windows/pack-portable.ps1') -ExePath $target -RootPath $fixture -RuntimeLibPath $runtime -Profile full -PrepareOnly
}
function Assert-Cache([bool]$Expected, [string]$Description) {
    & (Join-Path $cache 'portable-payload.exe') check $cache $fixture $runtime
    if (($LASTEXITCODE -eq 0) -ne $Expected) { throw "Cache result wrong: $Description (exit $LASTEXITCODE)" }
    Write-Host "PASS $Description"
}
try {
    foreach ($relative in @('include/runtime', 'third-party/win32-sdk/include',
        'third-party/win32-sdk/lib', 'lib', 'src/tools/portable_payload.c',
        'src/tools/portable_cache.inc', 'src/tools/check_runtime_abi.c',
        'src/tools/check_runtime_abi.S', 'scripts/windows/pack-portable.ps1')) { Copy-Fixture $relative }
    New-Item -ItemType Directory -Path (Split-Path $runtime -Parent) -Force | Out-Null
    Copy-Item -LiteralPath (Join-Path $root 'build/compiler/lib') -Destination $runtime -Recurse
    Copy-Item -LiteralPath $compiler -Destination $target
    $marker = Join-Path $fixture 'include/runtime/cache_marker.h'
    [IO.File]::WriteAllText($marker, "#define CACHE_VALUE 17`n")
    $unpackedHash = (Get-FileHash $target).Hash
    Prepare-Package
    if ((Get-FileHash $target).Hash -ne $unpackedHash) { throw 'PrepareOnly changed the compiler' }
    Assert-Cache $true 'explicit preparation leaves compiler untouched and seals cache'
    $helper = Join-Path $cache 'portable-payload.exe'
    & $helper cached $target $cache $fixture $runtime
    if ($LASTEXITCODE) { throw 'Cached attachment failed' }
    $packedHash = (Get-FileHash $target).Hash
    & $helper cached $target $cache $fixture $runtime
    if ($LASTEXITCODE -or (Get-FileHash $target).Hash -ne $packedHash) { throw 'Repeated packaging grew or changed the executable' }
    Write-Host 'PASS repeated attachment is byte-identical'

    foreach ($file in Get-ChildItem -LiteralPath $runtime -File) { $file.LastWriteTimeUtc = $file.LastWriteTimeUtc.AddSeconds(2) }
    Assert-Cache $true 'identical runtime contents with new timestamps remain cached'
    $time = [IO.File]::GetLastWriteTimeUtc($marker)
    [IO.File]::WriteAllText($marker, "#define CACHE_VALUE 18`n")
    [IO.File]::SetLastWriteTimeUtc($marker, $time)
    Assert-Cache $false 'same-size header edit with preserved timestamp invalidates'
    & $helper cached $target $cache $fixture $runtime
    if ($LASTEXITCODE -ne 2 -or (Get-FileHash $target).Hash -ne $packedHash) { throw 'Cache miss modified the compiler' }
    Prepare-Package
    Assert-Cache $true 'changed header is prepared again'
    & $helper cached $target $cache $fixture $runtime
    if ($LASTEXITCODE -or (Get-FileHash $target).Hash -eq $packedHash) { throw 'Header edit was not packaged' }

    $added = Join-Path $fixture 'include/runtime/cache_added.h'
    [IO.File]::WriteAllText($added, "#define ADDED 1`n")
    Assert-Cache $false 'new SDK header invalidates'
    Prepare-Package
    Remove-Item -LiteralPath $added
    Assert-Cache $false 'deleted SDK header invalidates'
    Prepare-Package

    $definition = Join-Path $runtime 'user32.def'
    $original = [IO.File]::ReadAllBytes($definition)
    $time = [IO.File]::GetLastWriteTimeUtc($definition)
    $changed = $original.Clone(); $changed[0] = $changed[0] -bxor 1
    [IO.File]::WriteAllBytes($definition, $changed)
    [IO.File]::SetLastWriteTimeUtc($definition, $time)
    Assert-Cache $false 'same-size runtime edit with preserved timestamp invalidates'
    [IO.File]::WriteAllBytes($definition, $original)
    Assert-Cache $true 'restored runtime content reuses payload'

    $payload = Join-Path $cache 'payload.bin'
    $payloadHash = (Get-FileHash $payload).Hash
    $original = [IO.File]::ReadAllBytes($payload)
    $changed = $original.Clone(); $changed[20] = $changed[20] -bxor 1
    [IO.File]::WriteAllBytes($payload, $changed)
    Assert-Cache $false 'corrupted prepared payload invalidates'
    Prepare-Package
    if ((Get-FileHash $payload).Hash -ne $payloadHash) {
        throw 'Repaired payload differs from original preparation'
    }
    Assert-Cache $true 'corrupted cache is rebuilt'

    $normalizer = Join-Path $fixture 'src/tools/portable_payload.c'
    Add-Content -LiteralPath $normalizer -Value '/* cache invalidation test */'
    Assert-Cache $false 'normalizer source change invalidates'
    Prepare-Package
    Assert-Cache $true 'updated native helper is prepared'
    $script = Join-Path $fixture 'scripts/windows/pack-portable.ps1'
    Add-Content -LiteralPath $script -Value '# cache invalidation test'
    Assert-Cache $false 'packaging script change invalidates'
    Prepare-Package

    $before = (Get-FileHash $target).Hash
    & $script -ExePath $target -RootPath $fixture -RuntimeLibPath $runtime -Profile none
    if ((Get-FileHash $target).Hash -ne $before) { throw 'Profile none modified compiler' }
    Write-Host 'PASS profile none leaves compiler untouched'

    # Exercise the real batch dispatch with a compiler-build stub. The stub
    # produces an existing host, keeping this a packaging test, not a rebuild.
    $batchRuntime = Join-Path $fixture 'build/compiler/lib'
    New-Item -ItemType Directory -Path (Split-Path $batchRuntime -Parent) -Force | Out-Null
    Copy-Item -LiteralPath $runtime -Destination $batchRuntime -Recurse
    Copy-Item -LiteralPath (Join-Path $root 'BuildClang.cmd') -Destination $fixture
    Copy-Item -LiteralPath $compiler -Destination (Join-Path $fixture 'cpc.exe')
    @'
@echo off
copy /y "%~dp0..\..\cpc.exe" "%~dp0..\..\build\compiler\cpc.exe" >nul
exit /b %ERRORLEVEL%
'@ | Set-Content -LiteralPath (Join-Path $fixture 'scripts/windows/build-cprime.bat') -Encoding ASCII
    & $script -ExePath $target -RootPath $fixture -RuntimeLibPath $batchRuntime -Profile full -PrepareOnly
    $batchOutput = & (Join-Path $fixture 'BuildClang.cmd') --self
    if ($LASTEXITCODE -or -not ($batchOutput -contains 'Portable payload: cached')) {
        throw "Batch build did not use the native package path: $batchOutput"
    }
    Write-Host 'PASS real build dispatch uses native cached packaging'

    # A bootstrap-only archive must never be admitted by a failed preparation.
    Copy-Item -LiteralPath (Join-Path $fixture 'lib/libcprime1.a') -Destination (Join-Path $runtime 'libcprime1.a') -Force
    $rejected = $false
    try { Prepare-Package 2> (Join-Path $work 'rejected-runtime.log') } catch { $rejected = $true }
    if (-not $rejected -or (Test-Path (Join-Path $cache 'inputs.sha256'))) { throw 'Invalid runtime was cached' }
    if ((Get-FileHash $target).Hash -ne $before) { throw 'Failed preparation modified compiler' }
    Write-Host 'PASS invalid runtime is rejected without publishing cache or compiler'
} finally {
    $resolved = [IO.Path]::GetFullPath($work)
    $build = [IO.Path]::GetFullPath((Join-Path $root 'build')) + '\'
    if (-not $resolved.StartsWith($build, [StringComparison]::OrdinalIgnoreCase)) { throw "Unexpected fixture path: $resolved" }
    if (Test-Path -LiteralPath $resolved) { Remove-Item -LiteralPath $resolved -Recurse -Force }
}
