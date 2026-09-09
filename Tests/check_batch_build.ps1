param([string]$CompilerPath = (Join-Path $PSScriptRoot '..\cpc.exe'))
$ErrorActionPreference = 'Stop'
$CompilerPath = [IO.Path]::GetFullPath($CompilerPath)
$driver = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\build\build_project.exe'))
$temporaryRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../build')).TrimEnd('\') + '\'
$fixture = Join-Path $temporaryRoot ('CPrime batch ' + [guid]::NewGuid().ToString('N'))
try {
    [void][IO.Directory]::CreateDirectory($fixture)
    $fakeSource = Join-Path $fixture 'fake.c'
    $fakeCompiler = Join-Path $fixture 'fake.exe'
    @'
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
int main(void) {
    const char *mode = getenv("CPRIME_BATCH_TEST_MODE");
    fprintf(stderr, "# cprime batch start 1\n");
    fflush(stderr);
    if (mode[0] == 't') Sleep(10000);
    if (mode[0] == 'c') ExitProcess(0xc0000005u);
    if (mode[0] == 'p') return 0;
    if (mode[0] == 'e') {
        fprintf(stderr, "error: deliberate failure\n# cprime batch end 1 1 0\n");
        return 1;
    }
    fprintf(stderr, "# cprime batch end 1 0 0\n");
    return 0; /* Deliberately omit the object. */
}
'@ | Set-Content -LiteralPath $fakeSource
    & $CompilerPath $fakeSource -o $fakeCompiler
    if ($LASTEXITCODE -ne 0) { throw 'Could not compile failure fixture' }
    $source = Join-Path $fixture 'main.c'
    'int main(void) { return 0; }' | Set-Content -LiteralPath $source
    $manifestPath = Join-Path $fixture 'manifest.json'
    @{
        schemaVersion = 1; platform = 'x64'; configuration = 'Debug'
        projects = @(@{ kind = 'Application'; directory = $fixture; targetName = 'app'
            sources = @(@{ path = $source }); includeDirectories = @(); defines = @() })
    } | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $manifestPath
    foreach ($mode in @('timeout', 'crash', 'protocol', 'error', 'missing')) {
        $outDir = Join-Path $fixture $mode
        [void][IO.Directory]::CreateDirectory($outDir)
        $exe = Join-Path $outDir 'app.exe'
        & $driver -CompilerPath $CompilerPath -ProjectRoot $fixture -ManifestPath $manifestPath -OutDir $outDir -ExePath $exe *> (Join-Path $fixture 'warm.log')
        if ($LASTEXITCODE) { throw 'Could not prepare stale build outputs' }
        $savedMode = $env:CPRIME_BATCH_TEST_MODE
        $savedPreference = $ErrorActionPreference
        try {
            $ErrorActionPreference = 'Continue'
            $env:CPRIME_BATCH_TEST_MODE = $mode
            $log = & $driver -CompilerPath $fakeCompiler `
                -ProjectRoot $fixture -ManifestPath $manifestPath -OutDir $outDir -ExePath $exe -CompileTimeoutSeconds 1 2>&1
            $code = $LASTEXITCODE
        } finally { $env:CPRIME_BATCH_TEST_MODE = $savedMode; $ErrorActionPreference = $savedPreference }
        if ($code -eq 0 -or [IO.File]::Exists($exe)) { throw "$mode was incorrectly accepted" }
        if (($log -join "`n") -notmatch 'Executable: NOT PRODUCED') { throw "$mode did not report build failure" }
        if ($mode -eq 'timeout' -and ($log -join "`n") -notmatch 'timed out') { throw 'Timeout was not enforced' }
        Write-Host "PASS batch $mode"
    }

    # Exercise fresh symbol, template, and identifier state across jobs in one
    # process, including a large identifier table followed by smaller units.
    $batch = Join-Path $fixture 'jobs.txt'
    $commands = @()
    $executables = @()
    foreach ($test in @('Includes/pass/test_identifier_table_growth.c',
                        'MemberFunctions/pass/test_static_integral_member_definition.cpp',
                        'MemberFunctions/pass/test_member_lookup_class_index.cpp')) {
        $inputPath = Join-Path $PSScriptRoot ('features/' + $test)
        $outputPath = Join-Path $fixture ([IO.Path]::GetFileNameWithoutExtension($test) + '.exe')
        $arguments = @($inputPath, '-o', $outputPath)
        $commands += (@($arguments | ForEach-Object { '"' + $_.Replace('\', '\\').Replace('"', '\"') + '"' }) -join ' ')
        $executables += $outputPath
    }
    foreach ($value in @(7, 91)) {
        $inputPath = Join-Path $fixture ("state-$value.cpp")
        $outputPath = Join-Path $fixture ("state-$value.exe")
        $prefix = if ($value -eq 7) { '#define LEAK_FROM_PREVIOUS_JOB 1' }
            else { "#ifdef LEAK_FROM_PREVIOUS_JOB`n#error macro survived compiler state destruction`n#endif" }
        @"
$prefix
struct Base { int x; };
struct Derived : Base { int y; };
template<class T> struct Box { static int evaluate(T x) { return x + $value; } };
int main() { Derived d; d.x = $value; return Box<int>::evaluate(d.x) != 2 * $value; }
"@ | Set-Content -LiteralPath $inputPath
        $arguments = @($inputPath, '-o', $outputPath)
        $commands += (@($arguments | ForEach-Object { '"' + $_.Replace('\', '\\').Replace('"', '\"') + '"' }) -join ' ')
        $executables += $outputPath
    }
    [IO.File]::WriteAllLines($batch, $commands, [Text.UTF8Encoding]::new($false))
    & $CompilerPath ('@' + $batch)
    if ($LASTEXITCODE -ne 0) { throw 'Fresh-state batch compilation failed' }
    foreach ($exe in $executables) {
        & $exe
        if ($LASTEXITCODE -ne 0) { throw "Batch executable failed: $exe" }
    }
    Write-Host 'PASS batch fresh-state runtime checks'
} finally {
    $resolved = [IO.Path]::GetFullPath($fixture)
    if (-not $resolved.StartsWith($temporaryRoot, [StringComparison]::OrdinalIgnoreCase)) { throw 'Unsafe fixture cleanup path' }
    if (Test-Path -LiteralPath $resolved) { Remove-Item -LiteralPath $resolved -Recurse -Force }
}
