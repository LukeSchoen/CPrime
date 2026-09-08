param(
    [string]$CompilerPath = (Join-Path $PSScriptRoot '../cpc.exe'),
    [string]$RuntimeLibPath = (Join-Path $PSScriptRoot '../build/compiler/lib'),
    [int]$MaxBytes = 0,
    [switch]$FullIncludes
)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$compiler = (Resolve-Path -LiteralPath $CompilerPath).Path
$temporaryRoot = [IO.Path]::GetFullPath((Join-Path $root 'build'))
$work = Join-Path $temporaryRoot ('portable-check-' + [guid]::NewGuid().ToString('N'))
$oldLocalAppData = $env:LOCALAPPDATA
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
    Assert-Minified ('const char *s = "a  +  b";') ('const char*s="a  +  b";' + "`n")
    Assert-Minified ('int f(int a, int b) { return a + +b; }') ('int f(int a,int b){return a+ +b;}' + "`n")
    Assert-Minified ('const char*s="a\' + "`n" + '  b";') ('const char*s="a\' + "`n" + '  b";' + "`n")
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
    $image = & (Join-Path $root 'scripts/windows/measure-compiler-size.ps1') -ExePath $portable
    if (-not $image.PortableBytes) { throw 'Compiler has no embedded portable package' }
    if (@($image.Imports | Where-Object { $_ -match '^(vcruntime|msvcp)[0-9]|libcprime' }).Count) {
        throw 'Portable compiler requires a separate compiler/VC runtime DLL'
    }
    # A preexisting extraction can hide missing files in the new package.
    $env:LOCALAPPDATA = Join-Path $work 'cache'
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

    $extracted = @(Get-ChildItem -LiteralPath (Join-Path $env:LOCALAPPDATA 'cpc') -Directory)
    if ($extracted.Count -ne 1) { throw 'Expected one fresh portable extraction' }
    $expected = Join-Path $work 'expected'
    New-Item -ItemType Directory -Path (Join-Path $expected 'include'), (Join-Path $expected 'lib') -Force | Out-Null
    Copy-Item -Path (Join-Path $root 'third-party/win32-sdk/include/*') -Destination (Join-Path $expected 'include') -Recurse
    Copy-Item -Path (Join-Path $root 'include/runtime/*') -Destination (Join-Path $expected 'include') -Recurse -Force
    & $helper headers (Join-Path $expected 'include')
    if ($LASTEXITCODE) { throw 'Expected SDK normalization failed' }
    Copy-Item -Path (Join-Path $root 'third-party/win32-sdk/lib/*') -Destination (Join-Path $expected 'lib') -Recurse
    Copy-Item -Path (Join-Path $RuntimeLibPath '*') -Destination (Join-Path $expected 'lib') -Recurse -Force
    $checked = 0
    foreach ($file in Get-ChildItem -LiteralPath $expected -Recurse -File) {
        $relative = $file.FullName.Substring($expected.Length + 1)
        $actual = Join-Path $extracted[0].FullName $relative
        if (-not (Test-Path -LiteralPath $actual -PathType Leaf)) { throw "Packaged SDK file missing: $relative" }
        $isOverlay = $relative -like 'include\winapi\*' -and
            (Test-Path -LiteralPath (Join-Path $root ('include/runtime/' + $file.Name)))
        if ($isOverlay -and [IO.File]::ReadAllText($actual) -ceq ('#include "../' + $file.Name + '"' + "`n")) {
            # The canonical shim is independently checked at include/<name>.
        } elseif ((Get-FileHash -LiteralPath $file.FullName).Hash -ne (Get-FileHash -LiteralPath $actual).Hash) {
            throw "Packaged SDK file differs from its normalized source: $relative"
        }
        ++$checked
    }
    Write-Output "PASS complete extracted SDK/runtime inventory ($checked files)"

    foreach ($suite in @('c_compat', 'features/Includes', 'payload')) {
        $selection = @{}
        if ($suite -eq 'features/Includes' -and -not $FullIncludes) {
            $selection.Select = @('test_include_cerrno.cpp', 'test_include_climits.cpp',
                'test_include_cstdarg.cpp', 'test_include_cstddef.cpp', 'test_include_ctime.cpp',
                'test_include_crtdbg.cpp', 'test_include_combaseapi_apartment.cpp',
                'test_shell_com_interfaces.cpp', 'test_opengl_core.cpp',
                'test_regex_ecmascript.cpp', 'test_string_streams.cpp',
                'test_map_balanced_operations.cpp', 'test_unordered_map_operations.cpp',
                'test_vector_insert_erase_ranges.cpp')
        }
        & (Join-Path $PSScriptRoot 'run.ps1') -Suite $suite -CompilerPath $portable @selection
        if ($LASTEXITCODE) { throw "Packaged $suite suite failed" }
    }

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
    $bytes = (Get-Item -LiteralPath $portable).Length
    if ($MaxBytes -gt 0 -and $bytes -gt $MaxBytes) { throw "Portable compiler size $bytes exceeds $MaxBytes bytes" }
    Write-Output "PASS portable compiler size: $bytes bytes"
} finally {
    $env:LOCALAPPDATA = $oldLocalAppData
    $resolved = [IO.Path]::GetFullPath($work)
    if (-not $resolved.StartsWith($temporaryRoot.TrimEnd('\') + '\', [StringComparison]::OrdinalIgnoreCase)) {
        throw "Unexpected fixture cleanup path: $resolved"
    }
    if (Test-Path -LiteralPath $resolved) { Remove-Item -LiteralPath $resolved -Recurse }
}
