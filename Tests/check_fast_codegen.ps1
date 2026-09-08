param([string]$CompilerPath = (Join-Path $PSScriptRoot '../cpc.exe'))
$ErrorActionPreference = 'Stop'
$compiler = (Resolve-Path -LiteralPath $CompilerPath).Path
$out = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../build/check-fast-codegen'))
[void][IO.Directory]::CreateDirectory($out)
$source = Join-Path $out 'check.c'
[IO.File]::WriteAllText($source, @'
#include <string.h>
int leaf(int x) { return x * 37 + 5; }
int wrapper(int x) { return leaf(x); }
__attribute__((noinline)) int retained(int x) { return x * 19; }
int calls_retained(int x) { return retained(x); }
void *copy31(void *d, const void *s) { return memcpy(d, s, 31); }
void *clear31(void *d) { return memset(d, 0, 31); }
__attribute__((noinline)) void opaque(char *p) { p[0] = 1; }
int near_probe(int a, int b, int c, int d) {
    char padding[4017];
    padding[0] = a;
    opaque(padding); a += b;
    opaque(padding); b += c;
    opaque(padding); c += d;
    opaque(padding); d += a;
    return a + b + c + d + padding[0];
}
int main(void) {
    char a[32] = "abcdefghijklmnopqrstuvwxyz", b[32];
    if (wrapper(7) != 264 || calls_retained(7) != 133) return 1;
    if (copy31(b, a) != b || memcmp(a, b, 31)) return 2;
    if (clear31(b) != b || b[0] || b[30]) return 3;
    return 0;
}
'@)
function Invoke-Checked([string]$Program, [string[]]$Arguments) {
    & $Program @Arguments
    if ($LASTEXITCODE -ne 0) { throw "$Program failed with exit code $LASTEXITCODE" }
}
function Get-Calls([string]$Path) {
    $data = [IO.File]::ReadAllBytes($Path)
    if ($data.Length -lt 64 -or [BitConverter]::ToString($data, 0, 6) -ne '7F-45-4C-46-02-01') { throw 'Expected ELF64 little-endian object' }
    $sectionOffset = [BitConverter]::ToUInt64($data, 40)
    $sectionSize = [BitConverter]::ToUInt16($data, 58)
    $count = [BitConverter]::ToUInt16($data, 60)
    $sections = @(for ($i = 0; $i -lt $count; ++$i) {
        $at = [int]($sectionOffset + $i * $sectionSize)
        [pscustomobject]@{ Type = [BitConverter]::ToUInt32($data, $at+4); Offset = [BitConverter]::ToUInt64($data, $at+24); Size = [BitConverter]::ToUInt64($data, $at+32); Link = [BitConverter]::ToUInt32($data, $at+40); Info = [BitConverter]::ToUInt32($data, $at+44); Entry = [BitConverter]::ToUInt64($data, $at+56) }
    })
    $symbols = @(foreach ($section in $sections) {
        if ($section.Type -ne 2) { continue }
        $strings = $sections[$section.Link]
        if (-not $section.Entry) { throw 'Invalid ELF symbol entry size' }
        for ($at = [int]$section.Offset; $at -lt $section.Offset + $section.Size; $at += $section.Entry) {
            $name = [int]($strings.Offset + [BitConverter]::ToUInt32($data, $at))
            $end = $name
            while ($data[$end] -ne 0) { ++$end }
            [pscustomobject]@{ Name = [Text.Encoding]::UTF8.GetString($data, $name, $end-$name); Info = $data[$at+4]; Index = [BitConverter]::ToUInt16($data, $at+6); Value = [BitConverter]::ToUInt64($data, $at+8); Size = [BitConverter]::ToUInt64($data, $at+16) }
        }
    })
    $references = @{}; $bodies = @{}
    foreach ($section in $sections) {
        if ($section.Type -ne 4) { continue }
        if (-not $section.Entry) { throw 'Invalid ELF relocation entry size' }
        for ($at = [int]$section.Offset; $at -lt $section.Offset + $section.Size; $at += $section.Entry) {
            $offset = [BitConverter]::ToUInt64($data, $at)
            $info = [BitConverter]::ToUInt64($data, $at+8)
            foreach ($symbol in $symbols) {
                if (($symbol.Info -band 15) -eq 2 -and $symbol.Index -eq $section.Info -and $symbol.Value -le $offset -and $offset -lt $symbol.Value + $symbol.Size) {
                    $references[$symbol.Name] = @($references[$symbol.Name]) + $symbols[[int]($info -shr 32)].Name
                    break
                }
            }
        }
    }
    foreach ($symbol in $symbols) {
        if (($symbol.Info -band 15) -eq 2 -and $symbol.Index -gt 0 -and $symbol.Index -lt $sections.Count) {
            $body = New-Object byte[] ([int]$symbol.Size)
            [Array]::Copy($data, [int]($sections[$symbol.Index].Offset + $symbol.Value), $body, 0, $body.Length)
            $bodies[$symbol.Name] = $body
        }
    }
    @{ References = $references; Bodies = $bodies }
}
$cases = @(
    @{ Name = 'plain'; Flags = @('-O0') }, @{ Name = 'fast'; Flags = @('-O2') },
    @{ Name = 'fast-debug'; Flags = @('-O2', '-g') }, @{ Name = 'no-inline'; Flags = @('-O2', '-fno-inline') },
    @{ Name = 'no-builtin'; Flags = @('-O2', '-fno-builtin') }
)
foreach ($case in $cases) {
    $name = $case.Name; $obj = Join-Path $out "$name.o"; $exe = Join-Path $out "$name.exe"
    foreach ($artifact in @($obj, $exe)) { if (Test-Path -LiteralPath $artifact) { Remove-Item -LiteralPath $artifact } }
    Invoke-Checked $compiler ($case.Flags + @('-Werror', '-c', $source, '-o', $obj))
    $calls = Get-Calls $obj; $refs = $calls.References; $frame = $calls.Bodies['near_probe']
    if ($frame[0] -eq 0xb8 -and $frame[11] -eq 0x4c -and $frame[12] -eq 0x89) { throw 'Probed frame used ordinary saved-register prolog' }
    if ((@($refs['wrapper']) -contains 'leaf') -ne ($name -in @('plain', 'no-inline', 'fast-debug'))) { throw "$name leaf inlining mismatch" }
    if (@($refs['calls_retained']) -notcontains 'retained') { throw "$name lost noinline call" }
    foreach ($pair in @(@('copy31', 'memcpy'), @('clear31', 'memset'))) {
        if ((@($refs[$pair[0]]) -contains $pair[1]) -ne ($name -in @('plain', 'no-builtin'))) { throw "$name $($pair[0]) builtin mismatch" }
    }
    Invoke-Checked $compiler ($case.Flags + @('-Werror', $source, '-o', $exe))
    Invoke-Checked $exe @()
}
$batchSources = [ordered]@{
    a = 'static int leaf(int x) { return x + 3; } int first(void) { return leaf(7); }'
    b = 'static int leaf(int x) { return x + 9; } int second(void) { return leaf(7); }'
    main = 'int first(void); int second(void); int main(void) { return first()!=10 || second()!=16; }'
}
$jobs = @(); $objects = @()
function Quote-Batch([string[]]$Arguments) { (@($Arguments | ForEach-Object { '"' + $_.Replace('\', '\\').Replace('"', '\"') + '"' }) -join ' ') }
foreach ($name in $batchSources.Keys) {
    $path = Join-Path $out "batch-$name.c"; $obj = Join-Path $out "batch-$name.o"
    [IO.File]::WriteAllText($path, $batchSources[$name])
    if (Test-Path -LiteralPath $obj) { Remove-Item -LiteralPath $obj }
    $objects += $obj
    $jobs += Quote-Batch @('-O2', '-Werror', '-c', $path, '-o', $obj)
}
$batchExe = Join-Path $out 'batch.exe'
if (Test-Path -LiteralPath $batchExe) { Remove-Item -LiteralPath $batchExe }
$jobs += Quote-Batch ($objects + @('-o', $batchExe))
$batch = Join-Path $out 'batch.txt'
[IO.File]::WriteAllText($batch, ($jobs -join "`n"))
Invoke-Checked $compiler @("@$batch")
Invoke-Checked $batchExe @()
Write-Output 'PASS: inline code, noinline attributes, opt-outs, small copies/fills, runtime results, and fresh batch state'
