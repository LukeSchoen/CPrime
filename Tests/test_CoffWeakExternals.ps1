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
$common = @()
$common += '-I' + (Join-Path $root 'include/runtime')
$common += '-I' + (Join-Path $root 'third-party/win32-sdk/include')
$common += '-I' + (Join-Path $root 'third-party/win32-sdk/include/winapi')
if ($RuntimeRoot) { $common += '-B' + (Resolve-Path -LiteralPath $RuntimeRoot).Path }
$work = Join-Path ([IO.Path]::GetTempPath()) ('cprime-coff-weak-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $work | Out-Null
function Invoke-Checked([string]$program, [string[]]$parameters) {
    $saved = $ErrorActionPreference; $ErrorActionPreference = 'Continue'
    try { $output = & $program @parameters 2>&1; $code = $LASTEXITCODE }
    finally { $ErrorActionPreference = $saved }
    if ($code) { throw "Command failed ($code): $program`n$($output -join "`n")" }
}
function Compile([string[]]$parameters) {
    if ($parameters[0] -eq '-ar') { Invoke-Checked $compiler $parameters }
    else { Invoke-Checked $compiler ($common + $parameters) }
}
function Check([string]$name, [string[]]$inputs, [int]$expected = 42) {
    Compile ($inputs + @(('-DEXPECTED=' + $expected), '-o', 'test.exe'))
    Invoke-Checked (Join-Path $work 'test.exe') @()
    Write-Output "PASS $name"
}
function Weak-Record([string]$path, [uint32]$search) {
    # Architecture-neutral alias member, as emitted by Microsoft's librarian:
    # one empty .drectve section, external fallback, weak optional + auxiliary.
    $memory = New-Object IO.MemoryStream
    $writer = New-Object IO.BinaryWriter($memory)
    $writer.Write([uint16]0); $writer.Write([uint16]1); $writer.Write([uint32]0)
    $writer.Write([uint32]60); $writer.Write([uint32]3)
    $writer.Write([uint16]0); $writer.Write([uint16]0)
    $writer.Write([Text.Encoding]::ASCII.GetBytes('.drectve'))
    1..6 | ForEach-Object { $writer.Write([uint32]0) }
    $writer.Write([uint16]0); $writer.Write([uint16]0); $writer.Write([uint32]0xA00)
    $writer.Write([Text.Encoding]::ASCII.GetBytes('fallback'))
    $writer.Write([uint32]0); $writer.Write([int16]0); $writer.Write([uint16]0)
    $writer.Write([byte]2); $writer.Write([byte]0)
    $writer.Write([Text.Encoding]::ASCII.GetBytes('optional'))
    $writer.Write([uint32]0); $writer.Write([int16]0); $writer.Write([uint16]0)
    $writer.Write([byte]105); $writer.Write([byte]1)
    $writer.Write([uint32]0); $writer.Write($search); $writer.Write((New-Object byte[] 10))
    $writer.Write([uint32]4)
    [IO.File]::WriteAllBytes((Join-Path $work $path), $memory.ToArray())
    $writer.Dispose(); $memory.Dispose()
}
try {
    Push-Location $work
    try {
        Set-Content -Encoding ASCII 'main.c' 'extern int optional(void); int main(void) { return optional() != EXPECTED; }'
        Set-Content -Encoding ASCII 'fallback.c' 'int fallback(void) { return 42; }'
        Set-Content -Encoding ASCII 'strong.c' 'int optional(void) { return 7; }'
        Weak-Record 'alias.obj' 3
        Check 'neutral COFF weak alias uses external fallback' @('main.c', 'alias.obj', 'fallback.c')
        Check 'strong direct definition overrides weak alias' @('main.c', 'alias.obj', 'fallback.c', 'strong.c') 7
        Compile @('-r', 'alias.obj', '-o', 'alias-r.obj')
        Check 'weak fallback survives relocatable link' @('main.c', 'alias-r.obj', 'fallback.c')
        Check 'strong definition overrides after relocatable link' @('main.c', 'alias-r.obj', 'fallback.c', 'strong.c') 7
        Compile @('-ar', 'rcs', 'alias.lib', 'alias.obj')
        Check 'librarian indexes neutral weak alias members' @('main.c', 'alias.lib', 'fallback.c')
        Compile @('-c', 'strong.c', '-o', 'strong.o')
        Compile @('-ar', 'rcs', 'strong.lib', 'strong.o')
        Weak-Record 'nolib.obj' 1
        Check 'NOLIBRARY weak policy preserves fallback' @('main.c', 'nolib.obj', 'fallback.c', 'strong.lib')
        Weak-Record 'search.obj' 2
        Check 'SEARCH_LIBRARY weak policy finds strong archive provider' @('main.c', 'search.obj', 'fallback.c', 'strong.lib') 7
        Set-Content -Encoding ASCII 'native.s' @'
.text
.globl fallback
fallback:
movl $42, %eax
retq
.weak optional
.set optional, fallback
.data
.globl weak_address
weak_address:
.quad optional
'@
        Invoke-Checked $native @('-c', 'native.s', '-o', 'native.obj')
        Set-Content -Encoding ASCII 'native-main.c' 'extern int optional(void); extern int (*weak_address)(void); int main(void) { return optional()!=EXPECTED || weak_address()!=EXPECTED || weak_address!=optional; }'
        Check 'native weak function relocation and address identity' @('native-main.c', 'native.obj')
        Check 'native weak address follows strong definition' @('native-main.c', 'native.obj', 'strong.c') 7
        Set-Content -Encoding ASCII 'import.s' @'
.weak optional
.set optional, GetCurrentProcessId
.data
.globl weak_address
weak_address:
.quad optional
'@
        Invoke-Checked $native @('-c', 'import.s', '-o', 'import.obj')
        Set-Content -Encoding ASCII 'import-main.c' @'
#include <windows.h>
extern DWORD optional(void);
extern DWORD (*weak_address)(void);
int main(void) { return optional()!=GetCurrentProcessId() || weak_address()!=GetCurrentProcessId() || weak_address!=optional; }
'@
        Check 'weak alias resolves to a DLL import thunk' @('import-main.c', 'import.obj')
        Set-Content -Encoding ASCII 'alternate.s' @'
.section .drectve,"yn"
.ascii " /alternatename:optional=fallback /alternatename:unused=absent"
'@
        Invoke-Checked $native @('-c', 'alternate.s', '-o', 'alternate.obj')
        Set-Content -Encoding ASCII 'empty.c' 'int main(void) { return 0; }'
        Check 'unused alternate directives introduce no references' @('empty.c', 'alternate.obj')
        Check 'alternate name binds the live fallback' @('main.c', 'alternate.obj', 'fallback.c')
        Check 'direct definition overrides alternate name' @('main.c', 'alternate.obj', 'fallback.c', 'strong.c') 7
        Check 'resolved alternate primary requires no fallback definition' @('main.c', 'alternate.obj', 'strong.c') 7
        Check 'archive definition is searched before alternate fallback' @('main.c', 'alternate.obj', 'fallback.c', 'strong.lib') 7
        Compile @('-c', 'fallback.c', '-o', 'fallback.o')
        Compile @('-ar', 'rcs', 'fallback.lib', 'fallback.o')
        Check 'alternate fallback triggers archive rescan' @('main.c', 'alternate.obj', 'fallback.lib')
        Compile @('-r', 'alternate.obj', '-o', 'alternate-r.o')
        Check 'deferred alternate directives survive relocatable output' @('main.c', 'alternate-r.o', 'fallback.lib')
        Set-Content -Encoding ASCII 'chain.s' @'
.section .drectve,"yn"
.ascii " /alternatename:middle=fallback /alternatename:optional=middle"
'@
        Invoke-Checked $native @('-c', 'chain.s', '-o', 'chain.obj')
        Check 'alternate-name chains activate independent of directive order' @('main.c', 'chain.obj', 'fallback.lib')
        Set-Content -Encoding ASCII 'alternate-import.s' @'
.section .drectve,"yn"
.ascii " /alternatename:optional=GetCurrentProcessId"
'@
        Invoke-Checked $native @('-c', 'alternate-import.s', '-o', 'alternate-import.obj')
        Set-Content -Encoding ASCII 'alternate-import.c' @'
#include <windows.h>
extern DWORD optional(void);
int main(void) { return optional() != GetCurrentProcessId(); }
'@
        Check 'alternate name resolves through a DLL import fallback' @('alternate-import.c', 'alternate-import.obj')
        Set-Content -Encoding ASCII 'section.s' @'
.section .rdata$a,"dr"
.quad 0
.section .rdata$z,"dr"
.globl native_value
native_value:
.long 42
.data
.globl native_section
native_section:
.secidx native_value
.globl native_offset
native_offset:
.secrel32 native_value
'@
        Invoke-Checked $native @('-c', 'section.s', '-o', 'section.obj')
        Set-Content -Encoding ASCII 'section-main.c' @'
#include <windows.h>
extern int native_value;
extern unsigned short native_section;
extern unsigned native_offset;
int main(void) {
    BYTE *image = (BYTE *)GetModuleHandleA(0);
    IMAGE_NT_HEADERS *header = (IMAGE_NT_HEADERS *)(image + ((IMAGE_DOS_HEADER *)image)->e_lfanew);
    IMAGE_SECTION_HEADER *sections = IMAGE_FIRST_SECTION(header);
    if (!native_section || native_section > header->FileHeader.NumberOfSections) return 1;
    return native_value != 42 || image + sections[native_section-1].VirtualAddress + native_offset != (BYTE *)&native_value;
}
'@
        Check 'native SECTION and SECREL address the containing PE section' @('section-main.c', 'section.obj')
        Compile @('-r', 'section.obj', '-o', 'section-r.obj')
        Check 'native section relocations survive relocatable link' @('section-main.c', 'section-r.obj')
    }
    finally { Pop-Location }
}
finally {
    $resolved = [IO.Path]::GetFullPath($work)
    $temp = [IO.Path]::GetFullPath([IO.Path]::GetTempPath())
    if (-not $resolved.StartsWith($temp, [StringComparison]::OrdinalIgnoreCase)) { throw 'Unsafe fixture cleanup path.' }
    Remove-Item -LiteralPath $resolved -Recurse -Force
}
