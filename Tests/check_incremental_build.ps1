param([string]$CompilerPath = (Join-Path $PSScriptRoot '../cpc.exe'))
$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$fixture = Join-Path $root ('build/incremental test ' + [guid]::NewGuid().ToString('N'))
[void][IO.Directory]::CreateDirectory($fixture)
$out = Join-Path $fixture 'objects'
$exe = Join-Path $fixture 'bin/app.exe'
$compiler = [IO.Path]::GetFullPath($CompilerPath)
'#define VALUE 3' | Set-Content (Join-Path $fixture 'shared #$$ header.h')
'#define PRIVATE 4' | Set-Content (Join-Path $fixture 'private.h')
'#include "shared #$$ header.h"', 'int first(void) { return VALUE; }' | Set-Content (Join-Path $fixture 'first.c')
'#include "private.h"', 'int second(void) { return PRIVATE; }' | Set-Content (Join-Path $fixture 'second.c')
'#include "shared #$$ header.h"', 'int first(void); int second(void); int extra(void); int main(void) { return first()!=VALUE || second()!=4 || extra()!=5; }' | Set-Content (Join-Path $fixture 'main.c')
'int extra(void) { return 5; }' | Set-Content (Join-Path $fixture 'extra.c')
& $compiler -c (Join-Path $fixture 'extra.c') -o (Join-Path $fixture 'extra.obj')
if ($LASTEXITCODE) { throw 'External object compilation failed' }
$manifest = @{
    schemaVersion = 1; platform = 'x64'; projects = @(@{
        kind = 'Application'; targetName = 'app'; directory = $fixture
        defines = @(); includeDirectories = @($fixture, $out); linkLibraries = @((Join-Path $fixture 'extra.obj')); libraryDirectories = @()
        sources = @('main.c', 'first.c', 'second.c' | ForEach-Object { @{ path = Join-Path $fixture $_ } })
    })
}
$manifestPath = Join-Path $fixture 'manifest.json'
$buildInput = Join-Path $fixture 'project settings.props'
'initial settings' | Set-Content -LiteralPath $buildInput
$manifest | ConvertTo-Json -Depth 8 | Set-Content $manifestPath
function Check-Fast([bool]$Expected) {
    & (Join-Path $root 'build/check_project_build.exe') (Join-Path $out 'check-separate.bin') --check-only | Out-Null
    if (($LASTEXITCODE -eq 0) -ne $Expected) { throw "Native no-change check should return $Expected" }
}
function Build([int]$Expected, [switch]$Fail, [switch]$Force) {
    $args = @(
        '-CompilerPath', $compiler, '-ProjectRoot', $fixture, '-ManifestPath', $manifestPath, '-OutDir', $out, '-ExePath', $exe,
        '-BuildInputs', $buildInput)
    if ($Force) { $args += '-Rebuild' }
    $savedPreference = $ErrorActionPreference
    try {
        $ErrorActionPreference = 'Continue'
        & (Join-Path $root 'build/build_project.exe') @args > (Join-Path $fixture 'build.log') 2>&1
        $buildExit = $LASTEXITCODE
    } finally { $ErrorActionPreference = $savedPreference }
    if ($Fail) { if ($buildExit -eq 0) { throw 'Broken source reused a cached object' }; return }
    if ($buildExit) { throw ([IO.File]::ReadAllText((Join-Path $fixture 'build.log'))) }
    $metrics = Get-Content (Join-Path $out 'build_metrics.json') -Raw | ConvertFrom-Json
    if ($metrics.CompiledUnits -ne $Expected) { throw "Expected $Expected compiled units, got $($metrics.CompiledUnits)" }
    & $exe
    if ($LASTEXITCODE) { throw 'Incremental executable returned the wrong result' }
    Check-Fast $true
    return $metrics
}
Build 3 | Out-Null
# Include-driven source discovery can insert/remove a unit before unchanged units.
'int optional(void) { return 7; }' | Set-Content (Join-Path $fixture 'optional.cpp')
$originalSources = $manifest.projects[0].sources
$manifest.projects[0].sources = @($originalSources[0], @{path = Join-Path $fixture 'optional.cpp'}, $originalSources[1], $originalSources[2])
$manifest | ConvertTo-Json -Depth 8 | Set-Content $manifestPath
Build 1 | Out-Null
1..2 | ForEach-Object {
    'int first(void) { return 3; }' | Set-Content (Join-Path $fixture 'first.c')
    $manifest.projects[0].sources = $originalSources
    $manifest | ConvertTo-Json -Depth 8 | Set-Content $manifestPath
    Build 1 | Out-Null
    '#include "shared #$$ header.h"', 'int first(void) { return VALUE; }' | Set-Content (Join-Path $fixture 'first.c')
    $manifest.projects[0].sources = @($originalSources[2], $originalSources[0], @{path = Join-Path $fixture 'optional.cpp'}, $originalSources[1])
    $manifest | ConvertTo-Json -Depth 8 | Set-Content $manifestPath
    Build 1 | Out-Null
}
$manifest.projects[0].sources = $originalSources
$manifest | ConvertTo-Json -Depth 8 | Set-Content $manifestPath
Build 0 | Out-Null
$stamp = (Get-Item $exe).LastWriteTimeUtc.Ticks
$metrics = Build 0
if ($metrics.Processes.Count -or (Get-Item $exe).LastWriteTimeUtc.Ticks -ne $stamp) { throw 'No-op build invoked tools or changed executable' }
Add-Content -LiteralPath $buildInput 'changed export input'
Check-Fast $false
Build 0 | Out-Null
Remove-Item -LiteralPath $buildInput
Check-Fast $false
Build 0 | Out-Null
'restored settings' | Set-Content -LiteralPath $buildInput
Check-Fast $false
Build 0 | Out-Null
# Header-name changes invalidate include lookup even when no existing input
# changed. Non-header files in the same directory must not recompile sources.
'/* added header */' | Set-Content (Join-Path $fixture 'new-header.h')
Check-Fast $false
Build 3 | Out-Null
Remove-Item -LiteralPath (Join-Path $fixture 'new-header.h')
Check-Fast $false
Build 3 | Out-Null
'unrelated output' | Set-Content (Join-Path $fixture 'new-output.log')
Build 0 | Out-Null
Add-Content (Join-Path $fixture 'extra.c') '/* external link input edit */'
& $compiler -c (Join-Path $fixture 'extra.c') -o (Join-Path $fixture 'extra.obj')
if ($LASTEXITCODE) { throw 'External object recompilation failed' }
Check-Fast $false
$metrics = Build 0
if (@($metrics.Processes | Where-Object Label -eq 'link').Count -ne 1) { throw 'Changed external object was not relinked' }
Add-Content (Join-Path $fixture 'first.c') '/* source edit */'
Check-Fast $false
Build 1 | Out-Null
'#define VALUE 9' | Set-Content (Join-Path $fixture 'shared #$$ header.h')
Check-Fast $false
Build 2 | Out-Null
$secondDep = Get-ChildItem $out -Filter '*.d' | Where-Object { (Get-Content $_.FullName -Raw) -match 'second\.c' } | Select-Object -First 1
Remove-Item ([IO.Path]::ChangeExtension($secondDep.FullName, '.obj'))
Check-Fast $false
Build 1 | Out-Null
Remove-Item $exe
Check-Fast $false
$metrics = Build 0
if (@($metrics.Processes | Where-Object Label -eq 'link').Count -ne 1) { throw 'Missing executable was not relinked' }
# Repeated executable deletion must relink directly without entering the driver.
1..3 | ForEach-Object {
    Remove-Item -LiteralPath $exe
    Check-Fast $false
    & (Join-Path $root 'build/check_project_build.exe') (Join-Path $out 'check-separate.bin')
    if ($LASTEXITCODE -or -not [IO.File]::Exists($exe)) { throw 'Native relink failed' }
    & $exe
    if ($LASTEXITCODE) { throw 'Native relink produced an incorrect executable' }
    Check-Fast $true
}
Remove-Item -LiteralPath $exe
Add-Content (Join-Path $fixture 'first.c') '/* changed source with missing executable */'
& (Join-Path $root 'build/check_project_build.exe') (Join-Path $out 'check-separate.bin')
if ($LASTEXITCODE -eq 0 -or [IO.File]::Exists($exe)) { throw 'Native relink ignored a changed input' }
Build 1 | Out-Null
# A failed direct link must invalidate its snapshot and leave no executable.
$snapshotPath = Join-Path $out 'check-separate.bin'
$stream = [IO.File]::Open($snapshotPath, [IO.FileMode]::Open, [IO.FileAccess]::ReadWrite)
$reader = [IO.BinaryReader]::new($stream)
try {
    $stream.Position = 8
    1..2 | ForEach-Object { $length = $reader.ReadUInt32(); $stream.Position += $length }
    $length = $reader.ReadUInt32()
    $position = $stream.Position
    $command = '"' + $compiler + '" -invalid-relink-test-option'
    if ($command.Length -gt $length) { throw 'Invalid command does not fit the test snapshot' }
    $bytes = [Text.Encoding]::UTF8.GetBytes($command.PadRight($length))
    $stream.Position = $position
    $stream.Write($bytes, 0, $bytes.Length)
} finally { $reader.Dispose() }
Remove-Item -LiteralPath $exe
$savedPreference = $ErrorActionPreference
try {
    $ErrorActionPreference = 'Continue'
    & (Join-Path $root 'build/check_project_build.exe') $snapshotPath *> (Join-Path $fixture 'failed-relink.log')
    $relinkExit = $LASTEXITCODE
} finally { $ErrorActionPreference = $savedPreference }
if ($relinkExit -eq 0 -or [IO.File]::Exists($exe)) { throw 'Failed native link left an executable' }
Check-Fast $false
Build 0 | Out-Null
Add-Content (Join-Path $fixture 'first.c') '#error broken source'
Build 1 -Fail
(Get-Content (Join-Path $fixture 'first.c') | Where-Object { $_ -ne '#error broken source' }) | Set-Content (Join-Path $fixture 'first.c')
Build 1 | Out-Null
$manifest.projects[0].defines = @('CHANGED_SETTING=1')
$manifest | ConvertTo-Json -Depth 8 | Set-Content $manifestPath
Check-Fast $false
Build 3 | Out-Null
Build 3 -Force | Out-Null
Build 0 | Out-Null
$snapshot = Join-Path $out 'check-separate.bin'
[IO.File]::WriteAllBytes($snapshot, [Text.Encoding]::ASCII.GetBytes('CPCCHK01'))
Check-Fast $false
Build 0 | Out-Null
$oldInclude = $env:CPATH
try {
    $env:CPATH = Join-Path $fixture 'new-include-directory'
    Check-Fast $false
    Build 3 | Out-Null
} finally { $env:CPATH = $oldInclude }
Check-Fast $false
Build 3 | Out-Null
# An object without its requested dependency file must not count as a
# successful compiler invocation, otherwise a build cache can become unsound.
$savedPreference = $ErrorActionPreference
try {
    $ErrorActionPreference = 'Continue'
    & $compiler -MD -MF (Join-Path $fixture 'missing/dependencies.d') -c (Join-Path $fixture 'extra.c') -o (Join-Path $fixture 'dep-error.obj') 2> (Join-Path $fixture 'dep-error.log')
    $dependencyExit = $LASTEXITCODE
} finally { $ErrorActionPreference = $savedPreference }
if ($dependencyExit -eq 0) { throw 'Failure to write dependencies was reported as success' }
Write-Host 'PASS: no-op, repeated include toggles, source insertion/removal/reordering, source/header edits, external link inputs, missing outputs, failed-build recovery, settings changes, and forced rebuild.'
