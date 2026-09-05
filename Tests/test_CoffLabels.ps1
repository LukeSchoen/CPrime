param(
    [string]$CompilerPath = '',
    [string]$RuntimeRoot = '',
    [string]$NativeCompilerPath = 'C:\Luke\Src\OT\cl\CommonLib\Assets\Programs\Clang\clang.exe'
)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
if (-not $CompilerPath) { $CompilerPath = Join-Path $root 'cpc.exe' }
$compiler = (Resolve-Path -LiteralPath $CompilerPath).Path
$native = (Resolve-Path -LiteralPath $NativeCompilerPath).Path
$commonArgs = @()
if ($RuntimeRoot) { $commonArgs += '-B' + (Resolve-Path -LiteralPath $RuntimeRoot).Path }
$work = Join-Path ([IO.Path]::GetTempPath()) ('cprime-coff-labels-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $work | Out-Null
function Compile([string]$Label, [string[]]$Arguments) {
    $savedPreference = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    try { $output = & $compiler @commonArgs @Arguments 2>&1; $result = $LASTEXITCODE }
    finally { $ErrorActionPreference = $savedPreference }
    if ($result -ne 0) { throw "$Label failed (exit $result):`n$($output -join "`n")" }
}
function Check-Executable([string]$Label, [string]$Path) {
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) { throw "$Label did not produce an executable." }
    & $Path
    if ($LASTEXITCODE -ne 0) { throw "$Label returned $LASTEXITCODE." }
    Write-Output "PASS $Label"
}
try {
    Push-Location $work
    try {
        Set-Content -Encoding ASCII 'provider.s' @'
.text
.globl coff_label_value
.def coff_label_value; .scl 2; .type 32; .endef
coff_label_value:
.def coff_local_start; .scl 6; .type 0; .endef
coff_local_start:
movl $42, %eax
retq
.def coff_local_end; .scl 6; .type 0; .endef
coff_local_end:
.data
.globl coff_label_address
coff_label_address:
.quad coff_local_start
.section .xdata,"dr"
coff_unwind:
.byte 1, 0, 0, 0
.section .pdata,"dr"
.rva coff_local_start, coff_local_end, coff_unwind
'@
        Set-Content -Encoding ASCII 'main.c' @'
#include <windows.h>
extern int coff_label_value(void);
extern int (*coff_label_address)(void);
struct RuntimeFunction { unsigned begin, end, unwind; };
typedef struct RuntimeFunction* (*LookupFunction)(unsigned long long, unsigned long long*, void*);
int main(void) {
    unsigned long long image = 0, code = (unsigned long long)(void*)coff_label_value;
    struct RuntimeFunction* entry;
    LookupFunction lookup = (LookupFunction)GetProcAddress(GetModuleHandleA("ntdll.dll"), "RtlLookupFunctionEntry");
    if (coff_label_value() != 42 || coff_label_address() != 42) return 1;
    if (coff_label_address != coff_label_value) return 2;
    if (!lookup) return 3;
    entry = lookup(code, &image, 0);
    if (!entry || image + entry->begin != code || image + entry->end <= code || !entry->unwind) return 4;
    return 0;
}
'@
        & $native -c 'provider.s' -o 'provider.obj'
        if ($LASTEXITCODE -ne 0) { throw 'Native COFF assembly failed.' }
        Compile 'direct object labels' @('main.c', 'provider.obj', '-o', 'direct.exe')
        Check-Executable 'direct native object code/data/unwind label relocations' (Join-Path $work 'direct.exe')
        Compile 'native archive' @('-ar', 'rcs', 'provider.lib', 'provider.obj')
        Compile 'archived labels' @('main.c', 'provider.lib', '-o', 'archive.exe')
        Check-Executable 'archive extraction retains local code labels' (Join-Path $work 'archive.exe')
        Compile 'relocatable labels' @('-r', 'provider.obj', '-o', 'relocated.obj')
        Compile 'relocated labels' @('main.c', 'relocated.obj', '-o', 'relocated.exe')
        Check-Executable 'relocatable link retains native unwind label relocations' (Join-Path $work 'relocated.exe')
    } finally { Pop-Location }
} finally {
    $tempRoot = [IO.Path]::GetFullPath([IO.Path]::GetTempPath()).TrimEnd('\', '/') + [IO.Path]::DirectorySeparatorChar
    $resolvedWork = [IO.Path]::GetFullPath($work)
    if (-not $resolvedWork.StartsWith($tempRoot, [StringComparison]::OrdinalIgnoreCase) -or
        [IO.Path]::GetFileName($resolvedWork) -notlike 'cprime-coff-labels-*') {
        throw "Refusing to remove an unexpected temporary directory: $resolvedWork"
    }
    if (Test-Path -LiteralPath $resolvedWork) { Remove-Item -LiteralPath $resolvedWork -Recurse -Force }
}
