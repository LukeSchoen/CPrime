param([string]$CompilerPath = (Join-Path $PSScriptRoot '../cpc.exe'))
$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$fixture = Join-Path $root ('build/native project ' + [guid]::NewGuid().ToString('N'))
[void][IO.Directory]::CreateDirectory($fixture)
$driver = Join-Path $root 'build/build_project.exe'
$out = Join-Path $fixture 'objects'
$exe = Join-Path $fixture 'app.exe'
$manifestPath = Join-Path $fixture 'manifest.json'
function Write-Fixture($Name, $Text) { [IO.File]::WriteAllText((Join-Path $fixture $Name), $Text + "`n") }
Write-Fixture main.c @'
#include <windows.h>
int left(void); int right(void); int override(void);
int main(void) {
    HMODULE h = GetModuleHandleA(0); HRSRC r = FindResourceA(h,(char*)101,(char*)10);
    if (!r || *(unsigned short*)LockResource(LoadResource(h,r)) != 1234) return 1;
    r = FindResourceA(h,(char*)202,(char*)10);
    if (!r || *(unsigned short*)LockResource(LoadResource(h,r)) != 4321) return 2;
    r = FindResourceA(h,(char*)303,(char*)10);
    return !r || left()+right()!=11 || override()!=7;
}
'@
Write-Fixture override.c @'
#if VALUE != 7 || defined(PROJECT_ONLY)
#error per-source replacement was lost
#endif
int override(void) { return VALUE; }
'@
Write-Fixture left.c 'int leaf(void); int left(void) { return leaf()+1; }'
Write-Fixture right.c 'int leaf(void); int right(void) { return leaf()+2; }'
Write-Fixture leaf.c 'int leaf(void) { return 4; }'
Write-Fixture unused.c 'int main(void) { return 99; }'
Write-Fixture resource.h '#define RESOURCE_VALUE 1234'
Write-Fixture app.rc "#include `"resource.h`"`n101 RCDATA { RESOURCE_VALUE }"
Write-Fixture second.rc '202 RCDATA { 4321 }'
[IO.File]::WriteAllText((Join-Path $fixture 'second.rc'), "202 RCDATA { 4321 }`n", [Text.Encoding]::Unicode)
Write-Fixture library.rc '303 RCDATA { 55 }'
function Ref($Name, $Link = $true, $Objects = $false) {
    @{ projectFile = Join-Path $fixture "$Name.vcxproj"; linkLibraryDependencies = $Link; useLibraryDependencyInputs = $Objects }
}
function Project($Name, $Sources, $Refs = @(), $Resources = @()) {
    Write-Fixture "$Name.vcxproj" '<Project />'
    @{
        kind = $(if ($Name -eq 'app') { 'Application' } else { 'StaticLibrary' })
        projectFile = Join-Path $fixture "$Name.vcxproj"; directory = $fixture
        targetName = $Name; targetPath = $exe; sources = @($Sources | ForEach-Object { @{path = Join-Path $fixture $_} })
        includeDirectories = @($fixture); defines = @('VALUE=2', 'PROJECT_ONLY')
        projectReferences = @($Refs); resources = @($Resources | ForEach-Object { @{path = Join-Path $fixture $_} })
        resourceIncludeDirectories = @($fixture); windowsSdkVersion = '10.0'
        linkLibraries = @(); libraryDirectories = @()
    }
}
$projects = @(
    (Project app @('main.c','override.c') @((Ref left),(Ref right),(Ref unused $false),(Ref resource $true $true)) @('app.rc','second.rc')),
    (Project left @('left.c') @((Ref leaf))),
    (Project leaf @('leaf.c')),
    (Project right @('right.c') @((Ref leaf))),
    (Project unused @('unused.c')),
    (Project resource @() @() @('library.rc'))
)
$projects[0].sources[1].defines = @('VALUE=7')
@{schemaVersion=2; platform='x64'; configuration='Release'; projects=$projects} |
    ConvertTo-Json -Depth 12 | Set-Content $manifestPath -Encoding UTF8
function Build($Expected) {
    & $driver -CompilerPath $CompilerPath -ProjectRoot $fixture -ManifestPath $manifestPath -OutDir $out -ExePath $exe > (Join-Path $fixture 'build.log')
    if ($LASTEXITCODE) { throw (Get-Content (Join-Path $fixture 'build.log') -Raw) }
    & $exe
    if ($LASTEXITCODE) { throw 'Native graph/resource executable failed' }
    $m = Get-Content (Join-Path $out 'build_metrics.json') -Raw | ConvertFrom-Json
    if ($m.CompiledUnits -ne $Expected) { throw "Expected $Expected compiles, got $($m.CompiledUnits)" }
    $m
}
Build 6 | Out-Null
$m = Build 0
if (@($m.Processes | Where-Object Kind -ne driver).Count) { throw 'Unchanged build invoked compiler/resource tools' }
Add-Content (Join-Path $fixture 'leaf.c') '/* leaf edit */'
$m = Build 1
if (@($m.Processes | Where-Object Label -like '*_archive').Count -ne 1) { throw 'Leaf edit rebuilt unrelated archives' }
Add-Content (Join-Path $fixture 'resource.h') '/* resource edit */'
$m = Build 0
if (-not @($m.Processes | Where-Object Label -like '*_resource_0').Count) { throw 'Resource header edit was ignored' }
Write-Fixture unity-main.cpp 'int helper(); int main() { return helper()!=42; }'
Write-Fixture unity-helper.cpp 'int helper() { return 42; }'
$unityManifest = Join-Path $fixture 'unity.json'
@{schemaVersion=2; platform='x64'; configuration='Release'; projects=@((Project app @('unity-main.cpp','unity-helper.cpp')))} |
    ConvertTo-Json -Depth 12 | Set-Content $unityManifest -Encoding UTF8
$unityOut = Join-Path $fixture 'unity'
& $driver -CompilerPath $CompilerPath -ManifestPath $unityManifest -OutDir $unityOut -ExePath $exe -Unity > (Join-Path $fixture 'unity.log')
if ($LASTEXITCODE) { throw 'Native unity build failed' }
& $exe
if ($LASTEXITCODE) { throw 'Unity executable failed' }
$m = Get-Content (Join-Path $unityOut 'build_metrics.json') -Raw | ConvertFrom-Json
if ($m.Sources -ne 2 -or $m.TranslationUnits -ne 1) { throw 'Identical C++ settings were not grouped' }
& $driver -CompilerPath $CompilerPath -ManifestPath $unityManifest -OutDir $unityOut -ExePath $exe -Unity -UnityBatchSize 1 > (Join-Path $fixture 'unity-single.log')
if ($LASTEXITCODE) { throw 'Native unity size change failed' }
& $exe
if ($LASTEXITCODE) { throw 'Resized unity executable failed' }
$m = Get-Content (Join-Path $unityOut 'build_metrics.json') -Raw | ConvertFrom-Json
if ($m.Sources -ne 2 -or $m.TranslationUnits -ne 2 -or $m.CompiledUnits -ne 2) { throw 'Unity batch size did not invalidate the old group' }
Write-Host 'PASS: diamond references, excluded link dependency, per-source replacement, merged application/library resources, selective archive/resource invalidation'
