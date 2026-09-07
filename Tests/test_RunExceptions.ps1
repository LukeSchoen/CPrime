param(
    [string]$CompilerPath = "",
    [string]$RuntimeRoot = ""
)
$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
if (!$CompilerPath) { $CompilerPath = Join-Path $root "cpc.exe" }
if (!$RuntimeRoot) { $RuntimeRoot = $root }
$CompilerPath = (Resolve-Path -LiteralPath $CompilerPath).Path
$RuntimeRoot = (Resolve-Path -LiteralPath $RuntimeRoot).Path
$work = Join-Path $root "build/run-exception-tests"
New-Item -ItemType Directory -Path $work -Force | Out-Null
Push-Location $root
try {
    $fixture = "Tests/features/Templates/pass/test_namespace_detection_return_substitution.cpp"
    & $CompilerPath "-B$RuntimeRoot" -Iinclude/runtime -Ithird-party/win32-sdk/include -Ithird-party/win32-sdk/include/winapi -run $fixture
    if ($LASTEXITCODE -ne 0) { throw "Exception -run test failed: $LASTEXITCODE" }
    $hostExe = Join-Path $work "image-lifetime.exe"
    & $CompilerPath "-B$root" -Iinclude/runtime -Iinclude/cprime -Ithird-party/win32-sdk/include -Ithird-party/win32-sdk/include/winapi -Isrc/compiler/frontend -Isrc/compiler/middleend -Isrc/compiler/backend/x64 -DCPRIME_TARGET_PE -DCPRIME_TARGET_X86_64 Tests/runtime/test_exception_image_lifetime.c src/compiler/middleend/libcprime.c -o $hostExe
    if ($LASTEXITCODE -ne 0) { throw "Exception embedding fixture compile failed: $LASTEXITCODE" }
    & $hostExe $RuntimeRoot
    if ($LASTEXITCODE -ne 0) { throw "Repeated exception image deletion failed: $LASTEXITCODE" }
    Write-Output "PASS -run exceptions and 24 exception image lifetimes"
} finally {
    Pop-Location
}
