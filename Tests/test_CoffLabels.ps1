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
.globl coff_imagebase_address
coff_imagebase_address:
.quad __ImageBase
.section .xdata,"dr"
.p2align 2
coff_unwind:
.byte 1, 0, 0, 0
.section .pdata,"dr"
.rva coff_local_start, coff_local_end, coff_unwind
'@
        Set-Content -Encoding ASCII 'main.c' @'
#include <windows.h>
extern int coff_label_value(void);
extern int (*coff_label_address)(void);
extern void *coff_imagebase_address;
extern char __ImageBase;
struct RuntimeFunction { unsigned begin, end, unwind; };
typedef struct RuntimeFunction* (*LookupFunction)(unsigned long long, unsigned long long*, void*);
int main(void) {
    unsigned long long image = 0, code = (unsigned long long)(void*)coff_label_value;
    struct RuntimeFunction* entry;
    LookupFunction lookup = (LookupFunction)GetProcAddress(GetModuleHandleA("ntdll.dll"), "RtlLookupFunctionEntry");
    if (coff_label_value() != 42 || coff_label_address() != 42) return 1;
    if (coff_label_address != coff_label_value) return 2;
    if (coff_imagebase_address != GetModuleHandleA(0) || (void *)&__ImageBase != GetModuleHandleA(0)) return 5;
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

        # Code and unwind contribution names may have different lexical order.
        # Each input subsection is valid on its own; the final exception
        # directory must contain contiguous rows ordered by final code RVA.
        Set-Content -Encoding ASCII 'subsections.s' @'
.section .text$z,"xr"
.globl coff_last
coff_last:
movl $42, %eax
retq
coff_last_end:
.section .text$a,"xr"
.globl coff_first
coff_first:
movl $7, %eax
retq
coff_first_end:
.section .xdata,"dr"
.p2align 2
subsection_unwind:
.byte 1, 0, 0, 0
.section .pdata$a,"dr"
.p2align 3
.rva coff_last, coff_last_end, subsection_unwind
.section .pdata$z,"dr"
.p2align 3
.rva coff_first, coff_first_end, subsection_unwind
'@
        Set-Content -Encoding ASCII 'subsections.c' @'
#include <windows.h>
extern int coff_first(void), coff_last(void);
struct RuntimeFunction { unsigned begin, end, unwind; };
typedef struct RuntimeFunction* (WINAPI *LookupFunction)(unsigned long long, unsigned long long*, void*);
int main(void) {
    unsigned long long image, code[2] = { (unsigned long long)(void*)coff_first, (unsigned long long)(void*)coff_last };
    LookupFunction lookup = (LookupFunction)GetProcAddress(GetModuleHandleA("ntdll.dll"), "RtlLookupFunctionEntry");
    int i;
    if (!lookup || coff_first()!=7 || coff_last()!=42) return 1;
    for (i=0; i<2; ++i) {
        struct RuntimeFunction *entry = lookup(code[i], &image, 0);
        if (!entry || image+entry->begin!=code[i] || image+entry->end<=code[i] || !entry->unwind) return 2+i;
    }
    return 0;
}
'@
        & $native -c 'subsections.s' -o 'subsections.obj'
        if ($LASTEXITCODE -ne 0) { throw 'Native unwind subsections failed.' }
        Compile 'unwind subsection ordering' @('subsections.c', 'subsections.obj', '-o', 'subsections.exe')
        Check-Executable 'native exception directory packs and sorts input subsections' (Join-Path $work 'subsections.exe')
        Compile 'relocatable unwind subsections' @('-r', 'subsections.obj', '-o', 'subsections-r.o')
        Compile 'relocated unwind subsection ordering' @('subsections.c', 'subsections-r.o', '-o', 'subsections-r.exe')
        Check-Executable 'packed unwind directory survives relocatable inputs' (Join-Path $work 'subsections-r.exe')

        # Archive members may contain unrelated entry points, as in libraries
        # built alongside their examples. A known DLL provider satisfies an
        # undefined symbol without extracting a competing archive definition.
        Set-Content -Encoding ASCII 'shadow.c' @'
int printf(const char *format, ...) { return -1; }
int main(void) { return 99; }
'@
        Set-Content -Encoding ASCII 'required.c' 'int archive_required(void) { return 42; }'
        Set-Content -Encoding ASCII 'imports.c' @'
extern int archive_required(void);
extern int printf(const char *, ...);
int main(void) { return archive_required() != 42 || printf("") != 0; }
'@
        Set-Content -Encoding ASCII 'deferred.c' @'
#pragma comment(lib, "./shadow.lib")
extern int archive_required(void);
extern int printf(const char *, ...);
int main(void) { return archive_required() != 42 || printf("") != 0; }
'@
        & $native -c -ffunction-sections 'shadow.c' -o 'shadow.obj'
        if ($LASTEXITCODE -ne 0) { throw 'Native competing provider compilation failed.' }
        & $native -c 'required.c' -o 'required.obj'
        if ($LASTEXITCODE -ne 0) { throw 'Native required provider compilation failed.' }
        Compile 'provider archive' @('-ar', 'rcs', 'shadow.lib', 'shadow.obj', 'required.obj')
        Compile 'known import provider' @('imports.c', '-lmsvcrt', 'shadow.lib', '-o', 'imports.exe')
        Check-Executable 'known DLL import prevents unrelated archive extraction' (Join-Path $work 'imports.exe')
        Compile 'deferred archive provider' @('deferred.c', '-o', 'deferred.exe')
        Check-Executable 'implicit CRT resolves before deferred DEFAULTLIB archives' (Join-Path $work 'deferred.exe')
        $savedPreference = $ErrorActionPreference
        $ErrorActionPreference = 'Continue'
        try {
            $conflict = & $compiler @commonArgs 'imports.c' 'shadow.lib' '-o' 'conflict.exe' 2>&1
            $conflictResult = $LASTEXITCODE
        } finally { $ErrorActionPreference = $savedPreference }
        if ($conflictResult -eq 0 -or ($conflict -join "`n") -notmatch "(COMDAT|defined twice|multiple definition)") {
            throw "Explicit archive definition must retain its duplicate-main diagnostic: $($conflict -join "`n")"
        }
        Write-Output 'PASS explicit archive order retains duplicate-definition diagnostics'

        Set-Content -Encoding ASCII 'early.c' @'
extern int late_helper(void);
int early_value(void) { return late_helper() + 2; }
'@
        Set-Content -Encoding ASCII 'late.c' @'
extern int early_value(void);
int late_helper(void) { return 40; }
int late_value(void) { return early_value(); }
'@
        Set-Content -Encoding ASCII 'cycle.c' @'
extern int late_value(void);
int main(void) { return late_value() != 42; }
'@
        & $native -c 'early.c' -o 'early.obj'
        if ($LASTEXITCODE -ne 0) { throw 'Native early provider compilation failed.' }
        & $native -c 'late.c' -o 'late.obj'
        if ($LASTEXITCODE -ne 0) { throw 'Native late provider compilation failed.' }
        Compile 'early archive' @('-ar', 'rcs', 'early.lib', 'early.obj')
        Compile 'late archive' @('-ar', 'rcs', 'late.lib', 'late.obj')
        Compile 'mutually dependent archives' @('cycle.c', 'early.lib', 'late.lib', '-o', 'cycle.exe')
        Check-Executable 'later native archives can reference earlier archive providers' (Join-Path $work 'cycle.exe')
        Set-Content -Encoding ASCII 'first-choice.c' 'int choice(void) { return 42; }'
        Set-Content -Encoding ASCII 'last-choice.c' 'int choice(void) { return 7; }'
        Set-Content -Encoding ASCII 'late-demand.c' 'extern int choice(void); int demand(void) { return choice(); }'
        Set-Content -Encoding ASCII 'ordered.c' 'extern int demand(void); int main(void) { return demand() != 42; }'
        Compile 'first choice object' @('-c', 'first-choice.c', '-o', 'first-choice.o')
        Compile 'last choice object' @('-c', 'last-choice.c', '-o', 'last-choice.o')
        Compile 'late demand object' @('-c', 'late-demand.c', '-o', 'late-demand.o')
        Compile 'first provider archive' @('-ar', 'rcs', 'first-choice.lib', 'first-choice.o')
        Compile 'later competing archive' @('-ar', 'rcs', 'last-choice.lib', 'late-demand.o', 'last-choice.o')
        Compile 'ordered delayed provider' @('ordered.c', 'first-choice.lib', 'last-choice.lib', '-o', 'ordered.exe')
        Check-Executable 'late references retain first archive provider priority' (Join-Path $work 'ordered.exe')
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
