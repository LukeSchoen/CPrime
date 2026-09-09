$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$work = Join-Path $root ('build/build default ' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path "$work/scripts/windows" -Force | Out-Null
Copy-Item -LiteralPath (Join-Path $root 'build.cmd') -Destination $work
Set-Content "$work/cpc.exe" 'bootstrap' -Encoding ASCII
# Exercise dispatch and exit propagation without rebuilding or replacing CPC.
@'
@echo off
>"%~dp0host.txt" echo %~1
>>"%~dp0host.txt" echo %~2
exit /b %CPC_TEST_EXIT%
'@ | Set-Content "$work/scripts/windows/build-cprime.bat" -Encoding ASCII
@'
@echo off
echo published>"%~dp0published.txt"
exit /b 0
'@ | Set-Content "$work/scripts/windows/publish-cpc.cmd" -Encoding ASCII
@'
@echo off
echo forbidden>"%~dp0clang-called.txt"
exit /b 99
'@ | Set-Content "$work/BuildClang.cmd" -Encoding ASCII
$oldExit = $env:CPC_TEST_EXIT
try {
    foreach ($case in @(
        @{ Arguments = @(); CompilerExit = 0; Exit = 0 },
        @{ Arguments = @('--self'); CompilerExit = 0; Exit = 2 },
        @{ Arguments = @(); CompilerExit = 7; Exit = 1 },
        @{ Arguments = @(); CompilerExit = -1; Exit = 1 },
        @{ Arguments = @('--optimised'); CompilerExit = 0; Exit = 2 }
    )) {
        Remove-Item "$work/scripts/windows/host.txt", "$work/scripts/windows/published.txt" -ErrorAction SilentlyContinue
        $env:CPC_TEST_EXIT = [string]$case.CompilerExit
        $arguments = $case.Arguments
        $output = & (Join-Path $work 'build.cmd') @arguments
        if ($LASTEXITCODE -ne $case.Exit) {
            throw "Build dispatch failed: arguments=$arguments output=$output exit=$LASTEXITCODE"
        }
        if ($case.Exit -eq 2) {
            if (Test-Path "$work/scripts/windows/host.txt") { throw 'Unsupported options invoked a compiler' }
        } else {
            $hostCall = Get-Content "$work/scripts/windows/host.txt"
            if ($hostCall[0] -ne '-c' -or $hostCall[1] -ne "$work\cpc.exe") { throw 'Build did not select root CPC' }
        }
        if ((Test-Path "$work/scripts/windows/published.txt") -ne ($case.Exit -eq 0)) { throw 'Incorrect publication after build' }
        if (Test-Path "$work/clang-called.txt") { throw 'Build invoked Clang entry point' }
    }
    Write-Host 'PASS CPC self-hosting by default; failures stop; host-selection flags rejected.'
} finally {
    $env:CPC_TEST_EXIT = $oldExit
    $resolved = [IO.Path]::GetFullPath($work)
    $buildRoot = [IO.Path]::GetFullPath((Join-Path $root 'build')) + [IO.Path]::DirectorySeparatorChar
    if (-not $resolved.StartsWith($buildRoot, [StringComparison]::OrdinalIgnoreCase)) {
        throw "Unexpected test output path: $resolved"
    }
    Remove-Item -LiteralPath $resolved -Recurse -Force
}
