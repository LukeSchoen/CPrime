$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$work = Join-Path $root ('build/regression gate ' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Force -Path "$work/scripts/windows", "$work/Tests" | Out-Null
Copy-Item -LiteralPath "$root/Build.cmd" -Destination $work
Copy-Item -LiteralPath "$root/scripts/windows/publish-cpc.cmd" -Destination "$work/scripts/windows"
@'
@echo off
for %%I in ("%~dp0..\..") do set "ROOT=%%~fI"
if not exist "%ROOT%\build\compiler" mkdir "%ROOT%\build\compiler"
echo candidate>"%ROOT%\build\compiler\cpc.exe"
exit /b 0
'@ | Set-Content "$work/scripts/windows/build-cprime.bat" -Encoding ASCII
@'
param($ExePath, $RootPath, $RuntimeLibPath, $Profile)
exit 0
'@ | Set-Content "$work/scripts/windows/pack-portable.ps1" -Encoding ASCII
@'
param($CompilerPath)
if ((Get-Content -Raw -LiteralPath $CompilerPath).Trim() -ne 'candidate') { exit 99 }
exit ([int]$env:CPRIME_TEST_GATE_EXIT)
'@ | Set-Content "$work/Tests/check_regressions.ps1" -Encoding ASCII
$oldExit = $env:CPRIME_TEST_GATE_EXIT
try {
    foreach ($gateExit in @(1, -1, 0)) {
        Set-Content "$work/cpc.exe" 'previous' -Encoding ASCII
        $env:CPRIME_TEST_GATE_EXIT = [string]$gateExit
        & "$work/Build.cmd" *> "$work/gate-$gateExit.log"
        if (($LASTEXITCODE -eq 0) -ne ($gateExit -eq 0)) { throw 'Build did not propagate regression gate result' }
        $expected = if ($gateExit) { 'previous' } else { 'candidate' }
        if ((Get-Content -Raw "$work/cpc.exe").Trim() -ne $expected) { throw 'Incorrect compiler publication after regression gate' }
    }
    Write-Host 'PASS rejected candidate preserves compiler; passing candidate is published.'
} finally {
    $env:CPRIME_TEST_GATE_EXIT = $oldExit
    $resolved = [IO.Path]::GetFullPath($work)
    $buildRoot = [IO.Path]::GetFullPath((Join-Path $root 'build')) + [IO.Path]::DirectorySeparatorChar
    if (-not $resolved.StartsWith($buildRoot, [StringComparison]::OrdinalIgnoreCase)) { throw 'Unsafe cleanup path' }
    Remove-Item -LiteralPath $resolved -Recurse -Force
}
