param(
    [string]$ExporterPath = 'C:\Luke\Src\OT\cl\export_build_manifest.ps1',
    [string]$CompilerPath = (Join-Path $PSScriptRoot '..\cpc.exe')
)
$ErrorActionPreference = 'Stop'
$driver = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\build_project.ps1'))
$CompilerPath = [IO.Path]::GetFullPath($CompilerPath)
$temporaryRoot = [IO.Path]::GetFullPath([IO.Path]::GetTempPath()).TrimEnd('\') + '\'
$fixture = Join-Path $temporaryRoot ('CPrime manifest ' + [guid]::NewGuid().ToString('N'))
if (-not $fixture.StartsWith($temporaryRoot, [StringComparison]::OrdinalIgnoreCase)) { throw 'Fixture must be under the temporary directory' }
try {
    New-Item -ItemType Directory -Path $fixture | Out-Null
    @'
Project("{fixture}") = "App", "App.vcxproj", "{app}"
EndProject
'@ | Set-Content -LiteralPath (Join-Path $fixture 'commonTool.sln')
    @'
<Project xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
  <ItemGroup><ProjectConfiguration Include="Release|x64" /></ItemGroup>
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Release|x64'">
    <ConfigurationType>Application</ConfigurationType><TargetName>Selected App</TargetName><TargetExt>.exe</TargetExt>
  </PropertyGroup>
  <ItemDefinitionGroup Condition="'$(Configuration)|$(Platform)'=='Release|x64'">
    <ClCompile><PreprocessorDefinitions>BASE=40;%(PreprocessorDefinitions)</PreprocessorDefinitions>
      <AdditionalIncludeDirectories>$(ProjectDir);%(AdditionalIncludeDirectories)</AdditionalIncludeDirectories></ClCompile>
  </ItemDefinitionGroup>
  <ItemGroup>
    <ClCompile Include="main file.cpp" />
    <ClCompile Include="helper.cpp" />
    <ClCompile Include="per_file.c"><PreprocessorDefinitions>EXTRA=2;%(PreprocessorDefinitions)</PreprocessorDefinitions></ClCompile>
    <ClCompile Include="excluded.cpp"><ExcludedFromBuild Condition="'$(Configuration)|$(Platform)'=='Release|x64'">true</ExcludedFromBuild></ClCompile>
  </ItemGroup>
</Project>
'@ | Set-Content -LiteralPath (Join-Path $fixture 'App.vcxproj')
    'int helper(); int main() { return helper() != 42; }' | Set-Content -LiteralPath (Join-Path $fixture 'main file.cpp')
    'extern "C" int value(void); int helper() { return value(); }' | Set-Content -LiteralPath (Join-Path $fixture 'helper.cpp')
    'int value(void) { return BASE + EXTRA; }' | Set-Content -LiteralPath (Join-Path $fixture 'per_file.c')
    '#error Excluded source was compiled' | Set-Content -LiteralPath (Join-Path $fixture 'excluded.cpp')
    & $ExporterPath -ProjectRoot $fixture
    $manifestPath = Join-Path $fixture 'builds\manifest\Release-x64.json'
    $manifest = Get-Content -Raw -LiteralPath $manifestPath | ConvertFrom-Json
    if ($manifest.projects[0].sources.Count -ne 3) { throw 'Source selection did not preserve ExcludedFromBuild' }
    if ($manifest.projects[0].sources[2].defines -notcontains 'EXTRA=2') { throw 'Per-file definitions were lost' }
    foreach ($unity in @($false, $true)) {
        $output = Join-Path $fixture $(if ($unity) { 'unity' } else { 'separate' })
        $arguments = @('-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', $driver, '-CompilerPath', $CompilerPath,
            '-ProjectRoot', $fixture, '-OutDir', $output, '-Jobs', '2')
        if ($unity) { $arguments += '-Unity' }
        & powershell @arguments
        if ($LASTEXITCODE -ne 0) { throw "Manifest build failed (Unity=$unity)" }
        $executable = Join-Path $fixture 'builds\Selected App.exe'
        & $executable
        if ($LASTEXITCODE -ne 0) { throw "Manifest executable failed (Unity=$unity)" }
        $inputs = Get-Content -Raw -LiteralPath (Join-Path $output 'compile_inputs.json') | ConvertFrom-Json
        if (@($inputs | ForEach-Object { $_.Inputs }).Count -ne 3) { throw 'Driver changed the selected source list' }
    }
    Write-Host 'Build manifest: source selection, per-file flags, spaced paths, compile/link, and unity passed.'
} finally {
    $resolvedFixture = [IO.Path]::GetFullPath($fixture)
    if ($resolvedFixture.StartsWith($temporaryRoot, [StringComparison]::OrdinalIgnoreCase) -and
        $resolvedFixture -ne $temporaryRoot.TrimEnd('\') -and (Test-Path -LiteralPath $resolvedFixture)) {
        Remove-Item -LiteralPath $resolvedFixture -Recurse -Force
    }
}
