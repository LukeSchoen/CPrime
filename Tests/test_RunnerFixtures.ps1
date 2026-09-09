param([string]$CompilerPath = '', [string]$RuntimeRoot = '')
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$buildRoot = [IO.Path]::GetFullPath((Join-Path $projectRoot 'build'))
$fixtureRoot = Join-Path $buildRoot ('runner-validation-' + [guid]::NewGuid().ToString('N'))
$suite = '../build/' + (Split-Path -Leaf $fixtureRoot)
$pass = Join-Path $fixtureRoot 'pass'
$includes = Join-Path $fixtureRoot 'include with spaces'
$savedManifest = $env:CPRIME_TEST_BUILD_MANIFEST
try {
    New-Item -ItemType Directory -Path $pass, $includes | Out-Null
    Set-Content -Encoding ASCII -LiteralPath (Join-Path $fixtureRoot 'context.cpp') -Value '// selected translation unit'
    Set-Content -Encoding ASCII -LiteralPath (Join-Path $includes 'context.hpp') -Value '#define HEADER_VALUE 1'
    Set-Content -Encoding ASCII -LiteralPath (Join-Path $fixtureRoot 'forced.hpp') -Value '#define FORCED_VALUE 1'
    Set-Content -Encoding ASCII -LiteralPath (Join-Path $pass 'test_probe.cpp') -Value @'
// EXPECT_COMPILE_ONLY: 1
// EXPECT_MANIFEST_SOURCE: context.cpp
#include "context.hpp"
#if defined(PROJECT_ONLY) || defined(REMOVE_THIS)
#error source overrides were not applied
#endif
static_assert(SOURCE_VALUE + HEADER_VALUE + FORCED_VALUE == 42, "manifest source preprocessing");
static_assert(sizeof(PROBE_TEXT) == 15, "quoted macro with spaces and backslash");
extern void external_definition(int);
void compile_probe() { external_definition(SOURCE_VALUE); }
'@
    Set-Content -Encoding ASCII -LiteralPath (Join-Path $pass 'test_inputs.cpp') -Value @'
// EXPECT_SOURCES: ["helper with spaces.cpp"]
int helper();
int main() { return helper() != 42; }
'@
    Add-Content -Encoding ASCII -LiteralPath (Join-Path $pass 'test_inputs.cpp') -Value @(
        ('// EXPECT_COMPILE_ARGS: -I"' + $includes + '"'),
        '#include "context.hpp"',
        'static_assert(HEADER_VALUE == 1, "quoted metadata include path");'
    )
    Set-Content -Encoding ASCII -LiteralPath (Join-Path $pass 'helper with spaces.cpp') -Value 'int helper() { return 42; }'
    $manifestPath = Join-Path $fixtureRoot 'manifest with spaces.json'
    $manifest = @{
        generator = 'Fixture'; schemaVersion = 2; platform = 'x64'; projectRoot = $fixtureRoot
        projects = @(@{
            name = 'fixture'; directory = $fixtureRoot
            includeDirectories = @('missing-project-includes'); defines = @('PROJECT_ONLY=1')
            compileSettings = @{ ForcedIncludeFiles = 'missing-project-forced.hpp' }
            sources = @(@{
                path = Join-Path $fixtureRoot 'context.cpp'
                includeDirectories = @($includes + '\')
                defines = @('SOURCE_VALUE=40', 'REMOVE_THIS=1', 'PROBE_TEXT="two words\\tail"') +
                    @(1..700 | ForEach-Object { 'IGNORED_SETTING_' + $_ + '=' + ('1' * 50) })
                settings = @{ ForcedIncludeFiles = 'forced.hpp'; UndefinePreprocessorDefinitions = 'REMOVE_THIS' }
            })
        })
    }
    $manifest | ConvertTo-Json -Depth 8 | Set-Content -Encoding UTF8 -LiteralPath $manifestPath
    $env:CPRIME_TEST_BUILD_MANIFEST = ''
    $runner = Join-Path $projectRoot 'Tests/run.ps1'
    $compilerArguments = @()
    if ($CompilerPath) { $compilerArguments = @('-CompilerPath', $CompilerPath) }
    if ($RuntimeRoot) { $compilerArguments += @('-RuntimeRoot', $RuntimeRoot) }
    $output = & powershell -NoProfile -ExecutionPolicy Bypass -File $runner -Suite $suite @compilerArguments -BuildManifestPath $manifestPath 2>&1
    if ($LASTEXITCODE -ne 0 -or ($output -join "`n") -notmatch 'Summary: 2 passed, 0 failed') { throw ($output -join "`n") }
    Write-Output 'PASS compile-only probes, source overrides, quoted paths and macros, >32KiB arguments, multiple inputs'
    $output = & powershell -NoProfile -ExecutionPolicy Bypass -File $runner -Suite $suite @compilerArguments -BuildManifestPath $manifestPath -Select test_probe.cpp 2>&1
    if ($LASTEXITCODE -ne 0 -or ($output -join "`n") -notmatch 'Summary: 1 passed, 0 failed') { throw ($output -join "`n") }
    try {
        $ErrorActionPreference = 'Continue' # Capture the expected child-script error in Windows PowerShell.
        $output = & powershell -NoProfile -ExecutionPolicy Bypass -File $runner -Suite $suite @compilerArguments -Select missing.cpp 2>&1
    } finally { $ErrorActionPreference = 'Stop' }
    if ($LASTEXITCODE -eq 0 -or ($output -join "`n") -notmatch 'Unknown test selection') { throw 'Unknown selection was not rejected' }
    Write-Output 'PASS exact selection and unknown selection rejection'
    $output = & powershell -NoProfile -ExecutionPolicy Bypass -File $runner -Suite $suite @compilerArguments 2>&1
    if ($LASTEXITCODE -eq 0 -or ($output -join "`n") -notmatch 'test setup failed: This test requires a build manifest') { throw ($output -join "`n") }
    Write-Output 'PASS missing manifest is a setup failure'
    $manifest.projects[0].sources[0].path = Join-Path $fixtureRoot 'different.cpp'
    $manifest | ConvertTo-Json -Depth 8 | Set-Content -Encoding UTF8 -LiteralPath $manifestPath
    $output = & powershell -NoProfile -ExecutionPolicy Bypass -File $runner -Suite $suite @compilerArguments -BuildManifestPath $manifestPath 2>&1
    if ($LASTEXITCODE -eq 0 -or ($output -join "`n") -notmatch 'must select exactly one source') { throw ($output -join "`n") }
    Write-Output 'PASS missing manifest source is a setup failure'
    $crashSuite = Join-Path $fixtureRoot 'crash'
    New-Item -ItemType Directory -Path (Join-Path $crashSuite 'pass') | Out-Null
    Set-Content -Encoding ASCII -LiteralPath (Join-Path $crashSuite 'pass/test_reject.cpp') -Value @(
        '// EXPECT_COMPILE_FAIL: 1', 'invalid source'
    )
    $fakeCompiler = Join-Path $fixtureRoot 'crash-compiler.cmd'
    Set-Content -Encoding ASCII -LiteralPath $fakeCompiler -Value '@exit /b -1073741819'
    $output = & powershell -NoProfile -ExecutionPolicy Bypass -File $runner -Suite ($suite + '/crash') -CompilerPath $fakeCompiler 2>&1
    if ($LASTEXITCODE -eq 0 -or ($output -join "`n") -notmatch 'compiler crashed') { throw ($output -join "`n") }
    Write-Output 'PASS compiler crash cannot satisfy an expected compile failure'
} finally {
    $env:CPRIME_TEST_BUILD_MANIFEST = $savedManifest
    $target = [IO.Path]::GetFullPath($fixtureRoot)
    if ([IO.Path]::GetDirectoryName($target) -ne $buildRoot -or
        [IO.Path]::GetFileName($target) -notlike 'runner-validation-*') {
        throw "Refusing to remove unexpected fixture path: $target"
    }
    Remove-Item -LiteralPath $target -Recurse -Force -ErrorAction SilentlyContinue
}
