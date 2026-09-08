param(
    [string]$ClangPath = "",
    [string]$OutDir = "",
    [ValidateSet('0', '1', '2', '3', 's', 'z')][string]$Optimization = '3',
    [switch]$Map,
    [switch]$SystemCRT
)

$ErrorActionPreference = "Stop"
if (Get-Variable -Name PSNativeCommandUseErrorActionPreference -Scope Global -ErrorAction SilentlyContinue) {
    $global:PSNativeCommandUseErrorActionPreference = $false
}

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$rootDir = Resolve-Path (Join-Path $scriptDir "..")

if (-not $ClangPath) {
    $ClangPath = Join-Path $rootDir "third-party\clang\bin\clang.exe"
}
if (-not $OutDir) {
    $OutDir = Join-Path $rootDir "build\clang"
}

$clang = Resolve-Path -LiteralPath $ClangPath -ErrorAction SilentlyContinue
if (-not $clang) {
    throw "Unable to find clang: $ClangPath"
}

New-Item -ItemType Directory -Force -Path $OutDir | Out-Null

$oldPath = $env:PATH
$compilerDir = Split-Path -Parent $clang.Path
$compilerExe = Split-Path -Leaf $clang.Path
$env:PATH = $compilerDir + [System.IO.Path]::PathSeparator + $env:PATH

try {
    $exePath = Join-Path $OutDir "cpc-clang.exe"

    $includeArgs = @(
        "-I", (Join-Path $rootDir "include\cprime"),
        "-I", (Join-Path $rootDir "src\compiler\frontend"),
        "-I", (Join-Path $rootDir "src\compiler\middleend"),
        "-I", (Join-Path $rootDir "src\compiler\backend\x64"),
        "-I", $rootDir.Path
    )
    $defines = @(
        "-DCPRIME_TARGET_PE",
        "-DCPRIME_TARGET_X86_64",
        "-Dopen=_open",
        "-Dread=_read",
        "-Dclose=_close",
        "-Dlseek=_lseek",
        "-Dunlink=_unlink",
        "-Dfdopen=_fdopen",
        "-Dgetcwd=_getcwd",
        "-Dstricmp=_stricmp",
        "-Dstrnicmp=_strnicmp",
        "-Dstrlwr=_strlwr",
        "-Wno-pragma-pack",
        "-Wno-comment",
        "-Wno-ignored-attributes",
        "-Wno-implicit-function-declaration",
        "-Wno-incompatible-library-redeclaration",
        "-Wno-deprecated-declarations",
        "-O$Optimization",
        "-g0",
        "-Wl,/DEBUG:NONE,/INCREMENTAL:NO",
        (Join-Path $rootDir "src\compiler\driver\cprime.c"),
        "-o",
        $exePath
    )

    if ($SystemCRT) {
        $defines += @('-D_DLL', '-D_MT', '-Xclang', '--dependent-lib=msvcrt',
            '-Wl,/NODEFAULTLIB:libcmt,/NODEFAULTLIB:libucrt,/NODEFAULTLIB:vcruntime,/NODEFAULTLIB:libvcruntime,/DEFAULTLIB:ucrt',
            '-Xlinker', 'libvcruntime.lib')
    }
    if ($Map) { $defines += '-Wl,/MAP:' + (Join-Path ([IO.Path]::GetFullPath($OutDir)) 'cpc-clang.map') }
    $compileTimer = [System.Diagnostics.Stopwatch]::StartNew()
    & $clang.Path @includeArgs @defines
    $compileTimer.Stop()
    if ($LASTEXITCODE -ne 0) {
        throw "clang CPC build failed with exit code $LASTEXITCODE."
    }
    if (-not (Test-Path -LiteralPath $exePath)) {
        throw "Expected built compiler was not produced: $exePath"
    }
    Write-Host ('Clang compile/link: {0:N3}s (-O{1} -g0; one compiler process).' -f $compileTimer.Elapsed.TotalSeconds, $Optimization)
} finally {
    $env:PATH = $oldPath
}

Write-Host "Wrote $(Join-Path $OutDir 'cpc-clang.exe')"
