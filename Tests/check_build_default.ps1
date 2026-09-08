$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$work = Join-Path $root ('build/build default ' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $work | Out-Null
Copy-Item -LiteralPath (Join-Path $root 'build.cmd') -Destination $work
# Exercise dispatch and exit propagation without rebuilding or replacing CPC.
@'
@echo off
echo %*
exit /b %CPC_TEST_EXIT%
'@ | Set-Content -LiteralPath (Join-Path $work 'BuildClang.cmd') -Encoding ASCII
$oldExit = $env:CPC_TEST_EXIT
try {
    foreach ($case in @(
        @{ Arguments = @(); Expected = '--optimised'; Exit = 0 },
        @{ Arguments = @('--self'); Expected = '--self'; Exit = 0 },
        @{ Arguments = @('--optimised'); Expected = '--optimised'; Exit = 7 }
    )) {
        $env:CPC_TEST_EXIT = [string]$case.Exit
        $arguments = $case.Arguments
        $output = & (Join-Path $work 'build.cmd') @arguments
        if ($LASTEXITCODE -ne $case.Exit -or "$output".Trim() -ne $case.Expected) {
            throw "Build dispatch failed: arguments=$arguments output=$output exit=$LASTEXITCODE"
        }
    }
    Write-Host 'PASS optimized default, explicit self-host, and build failure propagation.'
} finally {
    $env:CPC_TEST_EXIT = $oldExit
    $resolved = [IO.Path]::GetFullPath($work)
    $buildRoot = [IO.Path]::GetFullPath((Join-Path $root 'build')) + [IO.Path]::DirectorySeparatorChar
    if (-not $resolved.StartsWith($buildRoot, [StringComparison]::OrdinalIgnoreCase)) {
        throw "Unexpected test output path: $resolved"
    }
    Remove-Item -LiteralPath $resolved -Recurse -Force
}
