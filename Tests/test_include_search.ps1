param([string]$CompilerPath = (Join-Path $PSScriptRoot '../cpc.exe'))
$ErrorActionPreference = 'Stop'
$compiler = (Resolve-Path -LiteralPath $CompilerPath).Path
$temporary = [IO.Path]::GetFullPath([IO.Path]::GetTempPath())
$work = Join-Path $temporary ('cprime include search ' + [guid]::NewGuid().ToString('N'))
function Write-Fixture([string]$Name, [string]$Text) { [IO.File]::WriteAllText((Join-Path $work $Name), $Text) }
function Invoke-Checked([string]$Program, [string[]]$Arguments) {
    & $Program @Arguments
    if ($LASTEXITCODE -ne 0) { throw "$Program failed with exit code $LASTEXITCODE" }
}
try {
    foreach ($name in @('first', 'second', 'left', 'right')) { [void][IO.Directory]::CreateDirectory((Join-Path $work $name)) }
    Write-Fixture 'first/chain.h' "#pragma once`n#define FIRST 7`n#include_next <chain.h>`n"
    Write-Fixture 'second/chain.h' "#pragma once`n#define SECOND 11`n"
    Write-Fixture 'first/repeat.h' "VALUE += 1;`n"
    Write-Fixture 'left/local.h' "#define LEFT 13`n"
    Write-Fixture 'right/local.h' "#define RIGHT 17`n"
    Write-Fixture 'left/entry.h' '#include "local.h"'
    Write-Fixture 'right/entry.h' '#include "local.h"'
    Write-Fixture 'main.cpp' @'
#include <chain.h>
#include <chain.h>
#include "left/entry.h"
#include "right/entry.h"
#if __has_include(<absent-header.h>)
#error missing header reported present
#endif
#if !__has_include(<repeat.h>)
#error existing header reported absent
#endif
int main() {
  int result = 0;
#define VALUE result
#include <repeat.h>
#include <repeat.h>
  return FIRST != 7 || SECOND != 11 || LEFT != 13 || RIGHT != 17 || result != 2;
}
'@
    $arguments = @('-Werror', "-I$work/missing", "-I$work/first", "-I$work/second")
    $source = Join-Path $work 'main.cpp'; $exe = Join-Path $work 'test.exe'
    Invoke-Checked $compiler ($arguments + @($source, '-o', $exe))
    Invoke-Checked $exe @()
    [void][IO.Directory]::CreateDirectory((Join-Path $work 'missing'))
    Write-Fixture 'missing/new.h' "#define NEW_VALUE 23`n"
    Write-Fixture 'main.cpp' "#include <new.h>`nint main() { return NEW_VALUE != 23; }`n"
    Invoke-Checked $compiler ($arguments + @($source, '-o', $exe))
    Invoke-Checked $exe @()
    Write-Fixture 'shared.h' "#pragma once`n#define SHARED_VALUE 29`n"
    Write-Fixture 'one.cpp' "#include `"shared.h`"`nint other() { return SHARED_VALUE; }`n"
    Write-Fixture 'two.cpp' "#include `"shared.h`"`nint other(); int main() { return other() != SHARED_VALUE; }`n"
    Invoke-Checked $compiler ($arguments + @((Join-Path $work 'one.cpp'), (Join-Path $work 'two.cpp'), '-o', $exe))
    Invoke-Checked $exe @()
    Write-Output 'PASS include order, include_next, local origins, guards, repeated unguarded headers, has_include, fresh compilation state'
} finally {
    $resolved = [IO.Path]::GetFullPath($work)
    if (-not $resolved.StartsWith($temporary.TrimEnd('\') + '\', [StringComparison]::OrdinalIgnoreCase)) { throw 'Unexpected fixture cleanup path' }
    if (Test-Path -LiteralPath $resolved) { Remove-Item -LiteralPath $resolved -Recurse }
}
