param(
    [string]$ExporterPath = (Join-Path $PSScriptRoot '../scripts/windows/export-build-manifest.ps1'),
    [ValidateSet('Prime', 'Clang')][string]$Toolchain = 'Prime',
    [string]$CompilerPath = (Join-Path $PSScriptRoot '..\cpc.exe'),
    [string]$NativeCompilerPath = ''
)
$ErrorActionPreference = 'Stop'
$driver = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\build_project.ps1'))
$CompilerPath = [IO.Path]::GetFullPath($CompilerPath)
$fixtureJobs = if ($Toolchain -eq 'Prime') { 1 } else { 2 }
if (-not $NativeCompilerPath) {
    $NativeCompilerPath = if ($Toolchain -eq 'Clang') { $CompilerPath }
        else { Join-Path $PSScriptRoot '../third-party/clang/bin/clang.exe' }
}
if (-not (Test-Path -LiteralPath $NativeCompilerPath -PathType Leaf)) { throw 'Pass -NativeCompilerPath for the Clang compiler used to create native COFF inputs.' }
$temporaryRoot = [IO.Path]::GetFullPath([IO.Path]::GetTempPath()).TrimEnd('\') + '\'
$fixture = Join-Path $temporaryRoot ('CPrime manifest ' + [guid]::NewGuid().ToString('N'))
if (-not $fixture.StartsWith($temporaryRoot, [StringComparison]::OrdinalIgnoreCase)) { throw 'Fixture must be under the temporary directory' }
try {
    New-Item -ItemType Directory -Path $fixture | Out-Null
    if ($Toolchain -eq 'Prime') {
        $parallelLog = Join-Path $fixture 'parallel-rejected.log'
        $savedErrorAction = $ErrorActionPreference
        try {
            $ErrorActionPreference = 'Continue'
            & powershell -NoProfile -ExecutionPolicy Bypass -File $driver -Jobs 2 *> $parallelLog
            $parallelExit = $LASTEXITCODE
        } finally { $ErrorActionPreference = $savedErrorAction }
        if ($parallelExit -eq 0 -or
            (Get-Content -LiteralPath $parallelLog -Raw) -notmatch 'Cprime builds require -Jobs 1') {
            throw 'Cprime accepted parallel compiler execution'
        }
    }
    @'
Project("{fixture}") = "App", "App.vcxproj", "{app}"
EndProject
Project("{fixture}") = "Library", "library\Library.vcxproj", "{library}"
EndProject
Project("{fixture}") = "Unreferenced", "library\Unreferenced.vcxproj", "{unreferenced}"
EndProject
Project("{fixture}") = "Leaf", "library\Leaf.vcxproj", "{leaf}"
EndProject
Project("{fixture}") = "Right", "library\Right.vcxproj", "{right}"
EndProject
Project("{fixture}") = "Resources", "library\Resources.vcxproj", "{resources}"
EndProject
'@ | Set-Content -LiteralPath (Join-Path $fixture 'fixture.sln')
    @'
<Project xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
  <ItemGroup><ProjectConfiguration Include="Release|x64" /></ItemGroup>
  <PropertyGroup><OutputLabel>Selected</OutputLabel><BuildTag>$(OutputLabel) App</BuildTag></PropertyGroup>
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Release|x64'">
    <ConfigurationType>Application</ConfigurationType><TargetName>$(BuildTag)</TargetName><TargetExt>.exe</TargetExt>
    <WindowsTargetPlatformVersion>10.0</WindowsTargetPlatformVersion>
    <OutDir>$(ProjectDir)custom output\</OutDir>
  </PropertyGroup>
  <ItemDefinitionGroup Condition="'$(Configuration)|$(Platform)'=='Release|x64'">
    <ClCompile><PreprocessorDefinitions>BASE=40;PROJECT_ONLY;%(PreprocessorDefinitions)</PreprocessorDefinitions>
      <AdditionalIncludeDirectories>base includes;%(AdditionalIncludeDirectories)</AdditionalIncludeDirectories>
      <TreatWarningAsError>true</TreatWarningAsError></ClCompile>
    <ClCompile Condition="'$(Configuration)|$(Platform)'=='Debug|x64'"><PreprocessorDefinitions>WRONG_CONFIGURATION</PreprocessorDefinitions></ClCompile>
    <Link><AdditionalDependencies>lib folder\extra.obj;lib folder\other.obj;dxguid.lib;%(AdditionalDependencies)</AdditionalDependencies></Link>
    <ResourceCompile><PreprocessorDefinitions>RESOURCE_ID=101;RESOURCE_VALUE=1234</PreprocessorDefinitions></ResourceCompile>
  </ItemDefinitionGroup>
  <ItemGroup>
    <ClCompile Include="main file.cpp" />
    <ClCompile Include="helper.cpp" />
    <ClCompile Include="per_file.c"><PreprocessorDefinitions>EXTRA=2;%(PreprocessorDefinitions)</PreprocessorDefinitions></ClCompile>
    <ClCompile Include="override.c"><PreprocessorDefinitions>BASE=7</PreprocessorDefinitions>
      <AdditionalIncludeDirectories>override includes</AdditionalIncludeDirectories></ClCompile>
    <ClCompile Include="ordered.c"><AdditionalIncludeDirectories>override includes;%(AdditionalIncludeDirectories)</AdditionalIncludeDirectories></ClCompile>
    <ClCompile Include="empty.c"><PreprocessorDefinitions></PreprocessorDefinitions><AdditionalIncludeDirectories></AdditionalIncludeDirectories></ClCompile>
    <ClCompile Include="excluded.cpp"><ExcludedFromBuild Condition="'$(Configuration)|$(Platform)'=='Release|x64'">true</ExcludedFromBuild></ClCompile>
  </ItemGroup>
  <ItemGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x64'"><ClCompile Include="excluded.cpp" /></ItemGroup>
  <ItemGroup>
    <ProjectReference Include="library\Library.vcxproj" />
    <ProjectReference Include="library\Right.vcxproj" />
    <ProjectReference Include="library\Unreferenced.vcxproj"><LinkLibraryDependencies>false</LinkLibraryDependencies></ProjectReference>
    <ProjectReference Include="library\Resources.vcxproj"><UseLibraryDependencyInputs>true</UseLibraryDependencyInputs></ProjectReference>
    <ResourceCompile Include="first.rc" />
    <ResourceCompile Include="second.rc"><PreprocessorDefinitions>RESOURCE_ID=202;RESOURCE_VALUE=4321</PreprocessorDefinitions></ResourceCompile>
    <ResourceCompile Include="excluded.rc"><ExcludedFromBuild>true</ExcludedFromBuild></ResourceCompile>
  </ItemGroup>
</Project>
'@ | Set-Content -LiteralPath (Join-Path $fixture 'App.vcxproj')
    New-Item -ItemType Directory -Path (Join-Path $fixture 'base includes'), (Join-Path $fixture 'override includes'), (Join-Path $fixture 'lib folder'), (Join-Path $fixture 'library') | Out-Null
    @'
<Project xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
  <ItemGroup><ProjectConfiguration Include="Release|x64" /></ItemGroup>
  <PropertyGroup><ConfigurationType>StaticLibrary</ConfigurationType><TargetName>Selected Library</TargetName></PropertyGroup>
  <ItemGroup><ClCompile Include="used.c" /><ClCompile Include="unused.c" /></ItemGroup>
  <ItemGroup><ProjectReference Include="Leaf.vcxproj" /></ItemGroup>
</Project>
'@ | Set-Content -LiteralPath (Join-Path $fixture 'library\Library.vcxproj')
    @'
<Project xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
  <ItemGroup><ProjectConfiguration Include="Release|x64" /></ItemGroup>
  <PropertyGroup><ConfigurationType>StaticLibrary</ConfigurationType><TargetName>Unreferenced Library</TargetName></PropertyGroup>
  <ItemGroup><ClCompile Include="unreferenced.c" /></ItemGroup>
</Project>
'@ | Set-Content -LiteralPath (Join-Path $fixture 'library\Unreferenced.vcxproj')
    @'
<Project xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
  <ItemGroup><ProjectConfiguration Include="Release|x64" /></ItemGroup>
  <PropertyGroup><ConfigurationType>StaticLibrary</ConfigurationType><TargetName>Leaf Library</TargetName></PropertyGroup>
  <ItemGroup><ClCompile Include="leaf_one.c" /><ClCompile Include="leaf_two.c" /></ItemGroup>
</Project>
'@ | Set-Content -LiteralPath (Join-Path $fixture 'library\Leaf.vcxproj')
    @'
<Project xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
  <ItemGroup><ProjectConfiguration Include="Release|x64" /></ItemGroup>
  <PropertyGroup><ConfigurationType>StaticLibrary</ConfigurationType><TargetName>Right Library</TargetName></PropertyGroup>
  <ItemGroup><ClCompile Include="right.c" /><ProjectReference Include="Leaf.vcxproj" /></ItemGroup>
</Project>
'@ | Set-Content -LiteralPath (Join-Path $fixture 'library\Right.vcxproj')
    @'
<Project xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
  <ItemGroup><ProjectConfiguration Include="Release|x64" /></ItemGroup>
  <PropertyGroup><ConfigurationType>StaticLibrary</ConfigurationType><TargetName>Resource Library</TargetName></PropertyGroup>
  <ItemDefinitionGroup><ResourceCompile><PreprocessorDefinitions>RESOURCE_ID=303;RESOURCE_VALUE=5053</PreprocessorDefinitions></ResourceCompile></ItemDefinitionGroup>
  <ItemGroup><ResourceCompile Include="..\first.rc" /><ClCompile Include="resource_value.c" /></ItemGroup>
</Project>
'@ | Set-Content -LiteralPath (Join-Path $fixture 'library\Resources.vcxproj')
    'extern int leaf_one(void), right_value(void); int library_value(void) { return leaf_one() + right_value(); }' | Set-Content -LiteralPath (Join-Path $fixture 'library\used.c')
    'extern int leaf_two(void); int right_value(void) { return leaf_two(); }' | Set-Content -LiteralPath (Join-Path $fixture 'library\right.c')
    'int leaf_one(void) { return 30; }' | Set-Content -LiteralPath (Join-Path $fixture 'library\leaf_one.c')
    'int leaf_two(void) { return 47; }' | Set-Content -LiteralPath (Join-Path $fixture 'library\leaf_two.c')
    'int resource_linkage_value(void) { return 99; }' | Set-Content -LiteralPath (Join-Path $fixture 'library\resource_value.c')
    'extern int missing(void); int unused(void) { return missing(); }' | Set-Content -LiteralPath (Join-Path $fixture 'library\unused.c')
    'int library_value(void) { return -99; }' | Set-Content -LiteralPath (Join-Path $fixture 'library\unreferenced.c')
    '#define ORDER 1' | Set-Content -LiteralPath (Join-Path $fixture 'base includes\selected.h')
    '#define ORDER 3' | Set-Content -LiteralPath (Join-Path $fixture 'override includes\selected.h')
    @'
#include <windows.h>
extern "C" const GUID IID_IDirectInput8A;
extern "C" const int *native_address(void), *native_other_address(void);
int helper();
extern "C" int override_value(void), ordered_value(void), empty_value(void), extra_value(void), library_value(void), resource_linkage_value(void);
static int resource_value(int id) {
    HRSRC resource = FindResourceA(0, MAKEINTRESOURCEA(id), MAKEINTRESOURCEA(10));
    if (!resource || SizeofResource(0, resource) != 2) return -1;
    return *(const unsigned short *)LockResource(LoadResource(0, resource));
}
int main() {
    return helper() != 42 || override_value() != 10 || ordered_value() != 43
        || empty_value() != 1 || extra_value() != 9 || library_value() != 77
        || resource_value(101) != 1234 || resource_value(202) != 4321
        || resource_value(303) != 5053 || resource_linkage_value() != 99
        || IID_IDirectInput8A.Data1 != 0xbf798030
        || native_address() != native_other_address() || *native_address() != 23;
}
'@ | Set-Content -LiteralPath (Join-Path $fixture 'main file.cpp')
    @'
LANGUAGE 9, 1
RESOURCE_ID RCDATA BEGIN RESOURCE_VALUE END
'@ | Set-Content -LiteralPath (Join-Path $fixture 'first.rc'), (Join-Path $fixture 'second.rc')
    'resource payload' | Set-Content -LiteralPath (Join-Path $fixture 'asset.payload')
    Add-Content -LiteralPath (Join-Path $fixture 'second.rc') -Value '987 RCDATA "asset.payload"'
    '#error Excluded resource was compiled' | Set-Content -LiteralPath (Join-Path $fixture 'excluded.rc')
    'extern "C" int value(void); int helper() { return value(); }' | Set-Content -LiteralPath (Join-Path $fixture 'helper.cpp')
    'int value(void) { return BASE + EXTRA; }' | Set-Content -LiteralPath (Join-Path $fixture 'per_file.c')
    @'
#include "selected.h"
#ifdef PROJECT_ONLY
#error A replacement per-file definition inherited project definitions
#endif
int override_value(void) { return BASE + ORDER; }
'@ | Set-Content -LiteralPath (Join-Path $fixture 'override.c')
    "#include `"selected.h`"`nint ordered_value(void) { return BASE + ORDER; }" | Set-Content -LiteralPath (Join-Path $fixture 'ordered.c')
    @'
#ifdef BASE
#error Empty per-file definitions must clear the project defaults
#endif
int empty_value(void) { return 1; }
'@ | Set-Content -LiteralPath (Join-Path $fixture 'empty.c')
    @'
__declspec(selectany) int native_shared_value = 23;
#pragma comment(lib, "LIBCMT")
#pragma comment(lib, "OLDNAMES")
static int native_value = 8;
static int *native_pointer = &native_value;
static int native_bss[8];
extern int empty_value(void);
int extra_value(void) { native_bss[3] = *native_pointer + empty_value(); return native_bss[3]; }
const int *native_address(void) { return &native_shared_value; }
'@ | Set-Content -LiteralPath (Join-Path $fixture 'lib folder\extra.c')
    @'
__declspec(selectany) int native_shared_value = 23;
const int *native_other_address(void) { return &native_shared_value; }
'@ | Set-Content -LiteralPath (Join-Path $fixture 'lib folder\other.c')
    & $NativeCompilerPath -c (Join-Path $fixture 'lib folder\extra.c') -o (Join-Path $fixture 'lib folder\extra.obj')
    if ($LASTEXITCODE -ne 0) { throw 'Failed to compile the fixture link dependency' }
    & $NativeCompilerPath -c (Join-Path $fixture 'lib folder\other.c') -o (Join-Path $fixture 'lib folder\other.obj')
    if ($LASTEXITCODE -ne 0) { throw 'Failed to compile the second native COFF dependency' }
    '#error Excluded source was compiled' | Set-Content -LiteralPath (Join-Path $fixture 'excluded.cpp')
    & $ExporterPath -ProjectRoot $fixture -SolutionPath (Join-Path $fixture 'fixture.sln')
    $manifestPath = Join-Path $fixture 'build\manifest\Release-x64.json'
    $manifest = Get-Content -Raw -LiteralPath $manifestPath | ConvertFrom-Json
    if ($manifest.projects[0].sources.Count -ne 6) { throw 'Source selection did not preserve item-group conditions and ExcludedFromBuild' }
    if ($manifest.projects[0].sources[2].defines -notcontains 'EXTRA=2') { throw 'Per-file definitions were lost' }
    if ($manifest.projects[0].resources.Count -ne 2) { throw 'Resource selection was not preserved' }
    if ($manifest.projects[0].windowsSdkVersion -ne '10.0') { throw 'Windows SDK selection was not preserved' }
    # An explicit tool version is authoritative; an unavailable selection
    # must not silently use whichever Visual Studio happens to be installed.
    $appProjectPath = Join-Path $fixture 'App.vcxproj'
    $appProjectText = Get-Content -LiteralPath $appProjectPath -Raw
    $selectedProject = $appProjectText.Replace('<WindowsTargetPlatformVersion>10.0</WindowsTargetPlatformVersion>',
        '<WindowsTargetPlatformVersion>10.0</WindowsTargetPlatformVersion><VCToolsVersion>0.0.0</VCToolsVersion>')
    $selectedProject | Set-Content -LiteralPath $appProjectPath
    & $ExporterPath -ProjectRoot $fixture -SolutionPath (Join-Path $fixture 'fixture.sln')
    $selectedManifest = Get-Content -Raw -LiteralPath $manifestPath | ConvertFrom-Json
    if ($selectedManifest.projects[0].msvcToolsVersion -ne '0.0.0') { throw 'MSVC tools selection was not preserved' }
    $selectionLog = Join-Path $fixture 'missing-toolset.log'
    $savedErrorAction = $ErrorActionPreference
    try {
        $ErrorActionPreference = 'Continue'
        & powershell -NoProfile -ExecutionPolicy Bypass -File $driver -CompilerPath $CompilerPath -Toolchain $Toolchain `
            -ProjectRoot $fixture -OutDir (Join-Path $fixture 'missing-toolset') -Jobs $fixtureJobs *> $selectionLog
        $selectionExit = $LASTEXITCODE
    } finally { $ErrorActionPreference = $savedErrorAction }
    if ($selectionExit -eq 0 -or (Get-Content -LiteralPath $selectionLog -Raw) -notmatch 'Visual C\+\+ tools 0\.0\.0') {
        throw 'An unavailable explicit MSVC toolset selection was ignored'
    }
    $appProjectText | Set-Content -LiteralPath $appProjectPath
    & $ExporterPath -ProjectRoot $fixture -SolutionPath (Join-Path $fixture 'fixture.sln')
    foreach ($unity in @($false, $true)) {
        $output = Join-Path $fixture $(if ($unity) { 'unity' } else { 'separate' })
        $arguments = @('-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', $driver, '-CompilerPath', $CompilerPath,
            '-Toolchain', $Toolchain, '-OutDir', $output)
        if ($Toolchain -ne 'Prime') { $arguments += '-Jobs', [string]$fixtureJobs }
        if ($unity) { $arguments += '-Unity' }
        Push-Location $fixture
        try {
            & powershell @arguments
            if ($LASTEXITCODE -ne 0) { throw "Manifest build failed (Unity=$unity)" }
        } finally { Pop-Location }
        $executable = Join-Path $fixture 'custom output\Selected App.exe'
        & $executable
        if ($LASTEXITCODE -ne 0) { throw "Manifest executable failed (Unity=$unity)" }
        $inputs = Get-Content -Raw -LiteralPath (Join-Path $output 'compile_inputs.json') | ConvertFrom-Json
        if (@($inputs | ForEach-Object { $_.Inputs }).Count -ne 13) { throw 'Driver changed the selected source list' }
        $combined = @($inputs | Where-Object { $_.Inputs.Count -gt 1 })
        if ($unity -and $combined.Count -eq 0) { throw 'Unity build did not combine any sources' }
        if (-not $unity -and $combined.Count) { throw 'Separate build combined sources' }
        $archives = @(Get-ChildItem -LiteralPath $output -Filter '*.lib')
        if ($archives.Count -ne 5) { throw 'Static-library projects did not produce separate archives' }
        foreach ($archive in $archives) {
            if ([Text.Encoding]::ASCII.GetString([IO.File]::ReadAllBytes($archive.FullName), 0, 8) -ne "!<arch>`n") {
                throw 'Static-library output is not an archive'
            }
        }
        if ($Toolchain -eq 'Prime') {
            $checker = Join-Path (Split-Path $driver -Parent) 'build/check_project_build.exe'
            $snapshot = Join-Path $output $(if ($unity) { 'check-unity.bin' } else { 'check-separate.bin' })
            & $checker $snapshot
            if ($LASTEXITCODE) { throw 'Fresh manifest snapshot was not accepted by the native checker' }
            & powershell @arguments -ProjectRoot $fixture
            if ($LASTEXITCODE) { throw 'Incremental manifest rebuild failed' }
            $metrics = Get-Content -Raw (Join-Path $output 'build_metrics.json') | ConvertFrom-Json
            if ($metrics.CompiledUnits -or $metrics.Processes.Count) { throw 'Unchanged manifest build ran tools' }
            Add-Content -LiteralPath (Join-Path $fixture 'helper.cpp') -Value '// incremental source edit'
            & powershell @arguments -ProjectRoot $fixture
            if ($LASTEXITCODE) { throw 'Selective source rebuild failed' }
            $metrics = Get-Content -Raw (Join-Path $output 'build_metrics.json') | ConvertFrom-Json
            if ($metrics.CompiledUnits -ne 1) { throw 'Source edit rebuilt more than its translation unit/unity group' }
            Add-Content -LiteralPath (Join-Path $fixture 'first.rc') -Value '// incremental resource edit'
            & powershell @arguments -ProjectRoot $fixture
            if ($LASTEXITCODE) { throw 'Resource incremental rebuild failed' }
            $metrics = Get-Content -Raw (Join-Path $output 'build_metrics.json') | ConvertFrom-Json
            if ($metrics.CompiledUnits -or -not @($metrics.Processes | Where-Object Kind -eq 'resource').Count) { throw 'Resource edit did not rebuild resources independently' }
            Add-Content -LiteralPath (Join-Path $fixture 'asset.payload') -Value 'changed binary resource'
            & $checker $snapshot | Out-Null
            if ($LASTEXITCODE -eq 0) { throw 'Native checker missed a resource asset change' }
            & powershell @arguments -ProjectRoot $fixture
            if ($LASTEXITCODE) { throw 'Resource asset incremental rebuild failed' }
            $metrics = Get-Content -Raw (Join-Path $output 'build_metrics.json') | ConvertFrom-Json
            if ($metrics.CompiledUnits -or -not @($metrics.Processes | Where-Object Kind -eq 'resource').Count) { throw 'External resource payload dependency was missed' }
            & $executable
            if ($LASTEXITCODE) { throw 'Incrementally linked resources are incorrect' }
        }
    }
    # The selected warning policy is enforced, and a diagnostic build can
    # explicitly relax it without changing the project settings.
    Add-Content -LiteralPath (Join-Path $fixture 'empty.c') -Value '#warning manifest_fixture_warning'
    $warningOutput = Join-Path $fixture 'warnings'
    $warningArgs = @('-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', $driver, '-CompilerPath', $CompilerPath,
        '-Toolchain', $Toolchain, '-ProjectRoot', $fixture, '-OutDir', $warningOutput, '-Jobs', [string]$fixtureJobs, '-SkipLink')
    & powershell @warningArgs
    if ($LASTEXITCODE -eq 0) { throw 'The manifest warning-as-error policy was ignored' }
    & powershell @warningArgs -AllowWarnings
    if ($LASTEXITCODE -ne 0) { throw 'The explicit warning-policy override was ignored' }
    # A manifest generated before effective per-source overrides were added
    # must still combine its project flags and per-source additions.
    'int main(void) { return BASE + EXTRA != 42; }' | Set-Content -LiteralPath (Join-Path $fixture 'legacy.c')
    $legacyManifest = @{
        schemaVersion = 1; platform = 'x64'; projects = @(@{
            kind = 'Application'; targetName = 'Legacy'; directory = $fixture
            defines = @('BASE=40'); includeDirectories = @(); linkLibraries = @(); libraryDirectories = @()
            sources = @(@{path = (Join-Path $fixture 'legacy.c'); defines = @('EXTRA=2'); includeDirectories = @()})
        })
    }
    $legacyPath = Join-Path $fixture 'legacy.json'
    $legacyManifest | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $legacyPath
    & powershell -NoProfile -ExecutionPolicy Bypass -File $driver -CompilerPath $CompilerPath -Toolchain $Toolchain -ProjectRoot $fixture -ManifestPath $legacyPath -OutDir (Join-Path $fixture 'legacy')
    if ($LASTEXITCODE -ne 0) { throw 'Legacy manifest build failed' }
    & (Join-Path $fixture 'build\Legacy.exe')
    if ($LASTEXITCODE -ne 0) { throw 'Legacy manifest executable failed' }
    Write-Host 'Build manifest: conditional selection, effective settings/warning policy, expanded output, archive dependencies, resource data, native COFF/COMDAT/default libraries, selected MSVC/SDK libraries, spaced paths, unity, and legacy compatibility passed.'
} finally {
    $resolvedFixture = [IO.Path]::GetFullPath($fixture)
    if ($resolvedFixture.StartsWith($temporaryRoot, [StringComparison]::OrdinalIgnoreCase) -and
        $resolvedFixture -ne $temporaryRoot.TrimEnd('\') -and (Test-Path -LiteralPath $resolvedFixture)) {
        Remove-Item -LiteralPath $resolvedFixture -Recurse -Force
    }
}
