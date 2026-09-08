param(
    [string]$CompilerPath = '',
    [string]$RuntimeRoot = '',
    [string]$NativeCompilerPath = (Join-Path $PSScriptRoot '../third-party/clang/bin/clang.exe'),
    [string]$NativeLibraryPath = ''
)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
if (-not $CompilerPath) { $CompilerPath = Join-Path $root 'cpc.exe' }
$compiler = (Resolve-Path -LiteralPath $CompilerPath).Path
$native = (Resolve-Path -LiteralPath $NativeCompilerPath).Path
if (-not $NativeLibraryPath) {
    $candidates = @()
    foreach ($programFiles in @(${env:ProgramFiles(x86)}, $env:ProgramFiles)) {
        if ($programFiles) {
            $candidates += Get-Item -Path (Join-Path $programFiles 'Microsoft Visual Studio/*/*/VC/Tools/MSVC/*/lib/x64') -ErrorAction SilentlyContinue
        }
    }
    $selected = $candidates | Sort-Object FullName -Descending | Select-Object -First 1
    if (-not $selected) { throw 'Native CRT module regression requires an MSVC x64 library directory.' }
    $NativeLibraryPath = $selected.FullName
}
$nativeLibraries = (Resolve-Path -LiteralPath $NativeLibraryPath).Path
$common = @(('-I' + (Join-Path $root 'include/runtime')), ('-I' + (Join-Path $root 'third-party/win32-sdk/include')))
$common += '-I' + (Join-Path $root 'third-party/win32-sdk/include/winapi')
if ($RuntimeRoot) { $common += '-B' + (Resolve-Path -LiteralPath $RuntimeRoot).Path }
$work = Join-Path ([IO.Path]::GetTempPath()) ('cprime-module-exit-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $work | Out-Null
function Invoke-Checked([string]$program, [string[]]$parameters) {
    $saved = $ErrorActionPreference; $ErrorActionPreference = 'Continue'
    try { $output = & $program @parameters 2>&1; $code = $LASTEXITCODE }
    finally { $ErrorActionPreference = $saved }
    if ($code) { throw "Command failed ($code): $program`n$($output -join "`n")" }
}
function Compile([string[]]$parameters) { Invoke-Checked $compiler ($common + $parameters) }
function Expect-Marker([string]$path, [string]$expected) {
    if ([IO.File]::ReadAllText($path) -ne $expected) { throw "Incorrect exit order in $path; expected $expected" }
}
try {
    Push-Location $work
    try {
        $source = Join-Path $PSScriptRoot 'native_runtime'
        Invoke-Checked $native @('-c', '-O0', '-fno-stack-protector', (Join-Path $source 'ucrt_exit_provider.cpp'), '-o', 'provider.obj')
        Compile @('-Werror', (Join-Path $source 'ucrt_module_exit.c'), 'provider.obj', '-o', 'exit.exe')
        Invoke-Checked (Join-Path $work 'exit.exe') @('normal.txt', 'normal')
        Expect-Marker (Join-Path $work 'normal.txt') 'NOC'
        Write-Output 'PASS EXE CPC/native atexit and _onexit share LIFO process table'
        Invoke-Checked (Join-Path $work 'exit.exe') @('quick.txt', 'quick')
        Expect-Marker (Join-Path $work 'quick.txt') 'Q'
        Write-Output 'PASS quick_exit uses only the UCRT quick-exit table'
        Compile @('-Werror', '-shared', (Join-Path $source 'ucrt_exit_dll.c'), 'provider.obj', '-o', 'callbacks.dll')
        Compile @('-Werror', (Join-Path $source 'ucrt_exit_host.c'), '-o', 'host.exe')
        Invoke-Checked (Join-Path $work 'host.exe') @((Join-Path $work 'callbacks.dll'))
        Write-Output 'PASS DLL callbacks drain at each unload without process-table ownership'
        Compile @('provider.obj', '-Werror', '-run', (Join-Path $source 'ucrt_module_exit.c'), 'run.txt', 'normal')
        Expect-Marker (Join-Path $work 'run.txt') 'NOC'
        Write-Output 'PASS -run atexit and _onexit share callback order and lifetime'
        Compile @('-Werror', '-run', (Join-Path $source 'ucrt_gui_run.c'))
        Expect-Marker (Join-Path $work 'gui-run.txt') 'gui exit'
        Write-Output 'PASS GUI -run drains callbacks before generated code is released'
        Invoke-Checked $native @('-c', '-O0', '-fno-stack-protector', (Join-Path $source 'ucrt_native_statics.cpp'), '-o', 'statics.obj')
        Compile @('-Werror', (Join-Path $source 'ucrt_native_statics_host.c'), 'statics.obj', ('-L' + $nativeLibraries), '-o', 'statics.exe')
        Invoke-Checked (Join-Path $work 'statics.exe') @()
        Write-Output 'PASS native CRT thread-static guard initialization and TLS across threads'
    }
    finally { Pop-Location }
}
finally {
    $resolved = [IO.Path]::GetFullPath($work)
    $temp = [IO.Path]::GetFullPath([IO.Path]::GetTempPath())
    if (-not $resolved.StartsWith($temp, [StringComparison]::OrdinalIgnoreCase)) { throw 'Unsafe fixture cleanup path.' }
    Remove-Item -LiteralPath $resolved -Recurse -Force
}
