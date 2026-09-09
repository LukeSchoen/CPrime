param([Parameter(Mandatory = $true)][string]$CompilerPath, [string]$RuntimeRoot = '')
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'tools/process.ps1')
$compiler = (Resolve-Path -LiteralPath $CompilerPath).Path
$out = Join-Path (Split-Path $PSScriptRoot -Parent) ('build/linker-map-' + [guid]::NewGuid().ToString('N'))
[void](New-Item -ItemType Directory -Path $out)
$source = Join-Path $out 'probe.c'
$map = Join-Path $out 'probe.map'
$exe = Join-Path $out 'probe.exe'
Set-Content -Encoding ASCII -LiteralPath $source -Value @'
#include <stdio.h>
int map_probe(void) { return 42; }
int main(void) { printf("%llx\n", (unsigned long long)&map_probe); return map_probe() != 42; }
'@
$arguments = @($compiler)
if ($RuntimeRoot) { $arguments += '-B' + (Resolve-Path -LiteralPath $RuntimeRoot).Path }
$result = Invoke-TestProcess ($arguments + @($source, '-O0', '-o', $exe, ('-Wl,-Map=' + $map))) 5
if ($result.exit -ne 0 -or $null -eq $result.exit) { throw "Map build failed: $($result.output)" }
$run = Invoke-TestProcess @($exe) 5
if ($run.exit -ne 0 -or $null -eq $run.exit) { throw 'Map executable failed' }
$entry = @(Get-Content -LiteralPath $map | Where-Object { $_ -match '^([0-9a-fA-F]+) map_probe$' })
if ($entry.Count -ne 1) { throw 'Expected exactly one map_probe map entry' }
$address = [Convert]::ToUInt64(($entry[0] -split ' ')[0], 16)
if ($address -ne [Convert]::ToUInt64($run.output.Trim(), 16)) { throw 'Map address differs from the relocated runtime address' }
$badMap = Join-Path $out 'missing/probe.map'
$failure = Invoke-TestProcess ($arguments + @($source, '-o', $exe, ('-Wl,-Map=' + $badMap))) 5
if ($null -eq $failure.exit -or $failure.exit -eq 0 -or $failure.output -notmatch 'cannot open linker map') {
    throw 'An unwritable map must produce a normal compiler diagnostic'
}
Write-Output 'PASS linker map relocated symbols and write-error diagnostics'
