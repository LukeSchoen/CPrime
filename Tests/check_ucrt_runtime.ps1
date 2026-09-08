param(
    [string]$CompilerPath = '',
    [string]$RuntimeRoot = '',
    [string]$NativeCompilerPath = (Join-Path $PSScriptRoot '../third-party/clang/bin/clang.exe')
)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
if (-not $CompilerPath) { $CompilerPath = Join-Path $root 'cpc.exe' }
$compiler = (Resolve-Path -LiteralPath $CompilerPath).Path
$native = (Resolve-Path -LiteralPath $NativeCompilerPath).Path
$arguments = @(('-I' + (Join-Path $root 'include/runtime')), ('-I' + (Join-Path $root 'third-party/win32-sdk/include')))
if ($RuntimeRoot) { $arguments += '-B' + (Resolve-Path -LiteralPath $RuntimeRoot).Path }
$work = Join-Path ([IO.Path]::GetTempPath()) ('cprime-ucrt-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $work | Out-Null
function Invoke-Checked([string]$program, [string[]]$parameters) {
    $saved = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    try { $output = & $program @parameters 2>&1; $code = $LASTEXITCODE }
    finally { $ErrorActionPreference = $saved }
    if ($code) { throw "Command failed ($code): $program`n$($output -join "`n")" }
}
try {
    $provider = Join-Path $work 'provider.obj'
    $exe = Join-Path $work 'mixed.exe'
    Invoke-Checked $native @('-c', '-O0', '-fno-stack-protector', '-D_CRT_SECURE_NO_WARNINGS', (Join-Path $PSScriptRoot 'native_runtime/ucrt_provider.c'), '-o', $provider)
    Invoke-Checked $compiler ($arguments + @('-Werror', (Join-Path $PSScriptRoot 'native_runtime/ucrt_consumer.c'), $provider, '-o', $exe))
    $marker = Join-Path $work 'exit.txt'
    Invoke-Checked $exe @((Join-Path $work 'roundtrip.txt'), $marker, 'argument with spaces')
    if ([IO.File]::ReadAllText($marker) -ne 'finished') { throw 'UCRT exit callback did not run.' }
    # Imported DLL names are preserved verbatim in the PE import directory.
    $imageText = [Text.Encoding]::ASCII.GetString([IO.File]::ReadAllBytes($exe))
    if ($imageText -notmatch '(?i)ucrtbase\.dll' -or $imageText -match '(?i)msvcrt\.dll') {
        throw 'Mixed-runtime executable does not use a consistent UCRT provider.'
    }
    Write-Output 'PASS mixed CPC/native FILE ownership, formatting, scanning, buffered position, allocation, startup and exit'
    Write-Output 'PASS UCRT import composition excludes legacy MSVCRT'
    $jump = Join-Path $work 'setjmp.exe'
    Invoke-Checked $compiler ($arguments + @('-Werror', (Join-Path $PSScriptRoot 'native_runtime/ucrt_setjmp.c'), '-o', $jump))
    Invoke-Checked $jump @()
    Write-Output 'PASS UCRT setjmp/longjmp across nested calls and zero-to-one return'
    $runMarker = Join-Path $work 'run-exit.txt'
    Invoke-Checked $compiler ($arguments + @('-Werror', '-run', (Join-Path $PSScriptRoot 'native_runtime/ucrt_run.c'), $runMarker, 'argument with spaces'))
    if ([IO.File]::ReadAllText($runMarker) -ne 'run completed') { throw 'Generated-code exit callback did not run.' }
    Write-Output 'PASS -run arguments, callback lifetime and host CRT shutdown'
}
finally {
    $resolved = [IO.Path]::GetFullPath($work)
    $temp = [IO.Path]::GetFullPath([IO.Path]::GetTempPath())
    if (-not $resolved.StartsWith($temp, [StringComparison]::OrdinalIgnoreCase)) { throw 'Unsafe fixture cleanup path.' }
    Remove-Item -LiteralPath $resolved -Recurse -Force
}
