param([string]$CompilerPath = (Join-Path $PSScriptRoot '../cpc.exe'))
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$compiler = (Resolve-Path -LiteralPath $CompilerPath).Path
$temporaryRoot = [IO.Path]::GetFullPath([IO.Path]::GetTempPath())
$work = Join-Path $temporaryRoot ('cprime-portable-' + [guid]::NewGuid().ToString('N'))
try {
    [void][IO.Directory]::CreateDirectory((Join-Path $work 'headers/nested'))
    $helper = Join-Path $work 'portable-payload.exe'
    & $compiler -O2 (Join-Path $root 'src/tools/portable_payload.c') -o $helper
    if ($LASTEXITCODE -ne 0) { throw 'Native packaging helper compilation failed' }

    function Assert-Minified([string]$Source, [string]$Expected) {
        $fixture = Join-Path $work 'fixture.h'
        [IO.File]::WriteAllText($fixture, $Source)
        & $helper minify $fixture
        if ($LASTEXITCODE -ne 0) { throw 'Native header normalization failed' }
        $actual = [IO.File]::ReadAllText($fixture)
        if ($actual -cne $Expected) { throw "Unexpected header normalization: <$actual> expected <$Expected>" }
    }

    Assert-Minified "/* empty */`r`n// comment`r`n" "`n"
    Assert-Minified "int /* gap */ value = 3; // trailing`r`n" "int value=3;`n"
    Assert-Minified "#define SUM(x, y) ((x) + (y))`r`n" "#define SUM(x, y) ((x) + (y))`n"
    Assert-Minified ("#define PAIR(x) \`r`n ((x) + 1)`r`n") ("#define PAIR(x) \`n((x)+1)`n")
    Assert-Minified ('const char *s = "/*literal*/"; // actual comment') ('const char*s="/*literal*/";' + "`n")
    Assert-Minified ('const char *s = "escaped\"quote"; /* comment */') ('const char*s="escaped\"quote";' + "`n")
    Assert-Minified ("int a; /* block`r`nsecond line */ int b;`r`n") "int a;`nint b;`n"
    Write-Output 'PASS portable header normalization fixtures'

    foreach ($encoding in @([Text.Encoding]::UTF8, [Text.Encoding]::Unicode,
                            [Text.Encoding]::BigEndianUnicode, [Text.Encoding]::UTF32)) {
        $fixture = Join-Path $work 'encoded.h'
        [IO.File]::WriteAllText($fixture, "int$([char]0xa0)value = 3;`r`n", $encoding)
        & $helper minify $fixture
        if ($LASTEXITCODE -ne 0 -or [IO.File]::ReadAllText($fixture) -cne "int value=3;`n") {
            throw "Header encoding failed: $($encoding.EncodingName)"
        }
    }
    Write-Output 'PASS BOM decoding and Unicode whitespace'

    $header = Join-Path $work 'headers/nested/example.h'
    $extensionless = Join-Path $work 'headers/vector'
    [IO.File]::WriteAllText($header, "/* comment */ int answer = 42;`r`n")
    [IO.File]::WriteAllText($extensionless, "// preserve extensionless header`r`n")
    & $helper headers (Join-Path $work 'headers')
    if ($LASTEXITCODE -ne 0) { throw 'Native recursive normalization failed' }
    if ([IO.File]::ReadAllText($header) -cne "int answer=42;`n") { throw 'Nested header was not normalized' }
    if ([IO.File]::ReadAllText($extensionless) -cne "// preserve extensionless header`r`n") {
        throw 'Extensionless header was changed'
    }
    Write-Output 'PASS recursive .h processing and extensionless header preservation'

    # No adjacent runtime or SDK: compilation must use the executable's payload.
    $portable = Join-Path $work 'cpc.exe'
    Copy-Item -LiteralPath $compiler -Destination $portable
    $source = Join-Path $work 'probe.cpp'
    [IO.File]::WriteAllText($source, @'
#include <windows.h>
#include <stdio.h>
#include <vector>
int main() {
    std::vector<int> values;
    values.push_back(42);
    if (values[0] != 42 || GetCurrentProcessId() == 0) return 1;
    puts("portable payload ok");
    return 0;
}
'@)
    Push-Location $work
    try {
        & $portable $source -o probe.exe
        if ($LASTEXITCODE -ne 0) { throw 'Standalone payload compilation failed' }
        $output = & (Join-Path $work 'probe.exe')
        if ($LASTEXITCODE -ne 0 -or $output -cne 'portable payload ok') { throw 'Standalone payload runtime check failed' }
    } finally { Pop-Location }
    Write-Output 'PASS isolated portable Windows, C runtime and C++ headers'

    # Native libraries call __chkstk using the MSVC x64 ABI. A bootstrap
    # archive can link and run ordinary CPC code yet corrupt native frames.
    $abiSource = Join-Path $root 'src/tools/check_runtime_abi.c'
    $abiAssembly = Join-Path $root 'src/tools/check_runtime_abi.S'
    $abiExe = Join-Path $work 'runtime-abi.exe'
    & $portable $abiSource $abiAssembly -o $abiExe
    if ($LASTEXITCODE) { throw 'Portable stack-probe ABI fixture did not build' }
    & $abiExe
    if ($LASTEXITCODE) { throw 'Portable payload contains an incompatible native stack probe' }
    Write-Output 'PASS isolated portable native stack-probe ABI'

    $bootstrapObject = Join-Path $work 'bootstrap-probe.o'
    & $portable -DCPRIME_BOOTSTRAP_CHKSTK -c (Join-Path $root 'src/runtime/windows/chkstk.S') -o $bootstrapObject
    if ($LASTEXITCODE) { throw 'Bootstrap stack-probe fixture did not build' }
    & $portable $abiSource $abiAssembly $bootstrapObject -o $abiExe
    if ($LASTEXITCODE) { throw 'Bootstrap ABI rejection fixture did not link' }
    $savedPreference = $ErrorActionPreference
    try {
        $ErrorActionPreference = 'Continue'
        & $abiExe 2> (Join-Path $work 'bootstrap-rejected.log')
        $bootstrapExit = $LASTEXITCODE
    } finally { $ErrorActionPreference = $savedPreference }
    if ($bootstrapExit -ne 1) { throw 'ABI check did not cleanly reject the bootstrap stack probe' }
    Write-Output 'PASS bootstrap stack-probe rejection'
} finally {
    $resolved = [IO.Path]::GetFullPath($work)
    if (-not $resolved.StartsWith($temporaryRoot.TrimEnd('\') + '\', [StringComparison]::OrdinalIgnoreCase)) {
        throw "Unexpected fixture cleanup path: $resolved"
    }
    if (Test-Path -LiteralPath $resolved) { Remove-Item -LiteralPath $resolved -Recurse }
}
