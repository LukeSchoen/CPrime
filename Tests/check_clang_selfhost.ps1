param([string]$CompilerPath = '', [string]$RuntimeRoot = '')
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
if (-not $CompilerPath) { $CompilerPath = Join-Path $root 'cpc.exe' }
$compiler = (Resolve-Path -LiteralPath $CompilerPath).Path
$work = Join-Path $root ('build/clang-selfhost-check-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $work | Out-Null
if (-not $RuntimeRoot) {
    $RuntimeRoot = Join-Path $work 'runtime'
    New-Item -ItemType Directory -Path (Join-Path $RuntimeRoot 'include') -Force | Out-Null
    Copy-Item -Path (Join-Path $root 'third-party/win32-sdk/include/*') -Destination (Join-Path $RuntimeRoot 'include') -Recurse
    Copy-Item -Path (Join-Path $root 'include/runtime/*') -Destination (Join-Path $RuntimeRoot 'include') -Recurse -Force
    Copy-Item -LiteralPath (Join-Path $root 'build/compiler/lib') -Destination (Join-Path $RuntimeRoot 'lib') -Recurse
}
$runtime = (Resolve-Path -LiteralPath $RuntimeRoot).Path
$arguments = @('-B' + $runtime)
foreach ($include in @('include/runtime', 'include/cprime', 'third-party/win32-sdk/include',
    'third-party/win32-sdk/include/winapi', 'src/compiler/frontend',
    'src/compiler/middleend', 'src/compiler/backend/x64', '.')) {
    $arguments += '-I' + (Join-Path $root $include)
}
$arguments += @('-DCPRIME_TARGET_PE', '-DCPRIME_TARGET_X86_64',
    (Join-Path $root 'src/compiler/driver/cprime.c'))

# Exercise the supplied host, then two generations built by CPC itself.
& (Join-Path $PSScriptRoot 'run.ps1') -Suite c_compat -CompilerPath $compiler -RuntimeRoot $runtime
if ($LASTEXITCODE -ne 0) { throw 'Host C compatibility suite failed' }
& (Join-Path $PSScriptRoot 'check_regressions.ps1') -CompilerPath $compiler -RuntimeRoot $runtime
if ($LASTEXITCODE -ne 0) { throw 'Host regression gate failed' }
foreach ($generation in 2..3) {
    $output = Join-Path $work "stage$generation.exe"
    & $compiler @arguments -o $output
    if ($LASTEXITCODE -ne 0) { throw "Stage $generation self compilation failed" }
    & $output -v
    if ($LASTEXITCODE -ne 0) { throw "Stage $generation failed to start" }
    $compiler = $output
}
& (Join-Path $PSScriptRoot 'run.ps1') -Suite c_compat -CompilerPath $compiler -RuntimeRoot $runtime
if ($LASTEXITCODE -ne 0) { throw 'Self-hosted C compatibility suite failed' }
& (Join-Path $PSScriptRoot 'check_regressions.ps1') -CompilerPath $compiler -RuntimeRoot $runtime
if ($LASTEXITCODE -ne 0) { throw 'Self-hosted regression gate failed' }
Write-Host 'PASS host and two successive CPC self-builds; host and final C compatibility suites and regression gates.'
