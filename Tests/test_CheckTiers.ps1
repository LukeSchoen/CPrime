$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$work = Join-Path $root ('build/check-tiers-' + [guid]::NewGuid().ToString('N'))
try {
    New-Item -ItemType Directory -Force (Join-Path $work 'tools') | Out-Null
    Copy-Item (Join-Path $PSScriptRoot 'run-checks.ps1') $work
    Copy-Item (Join-Path $PSScriptRoot 'tools/process.ps1') (Join-Path $work 'tools/process.ps1')
    Copy-Item (Join-Path $PSScriptRoot 'tools/run-command.ps1') (Join-Path $work 'tools/run-command.ps1')
    $catalog = @(
        @{name='good';path='good.ps1';tier='fast';seconds=5;compiler=$true;runtime=$true},
        @{name='bad';path='bad.ps1';tier='fast';seconds=5;compiler=$false;runtime=$false},
        @{name='slow';path='slow.ps1';tier='fast';seconds=0.1;compiler=$false;runtime=$false},
        @{name='deep';path='deep.ps1';tier='pedantic';seconds=10;compiler=$false;runtime=$false}
    )
    $catalog | ConvertTo-Json | Set-Content (Join-Path $work 'checks.json')
    @'
param($CompilerPath, $RuntimeRoot)
if (-not $CompilerPath.EndsWith('compiler with spaces.exe') -or -not $RuntimeRoot.EndsWith('runtime with spaces')) { exit 3 }
exit 0
'@ | Set-Content (Join-Path $work 'good.ps1')
    'exit 7' | Set-Content (Join-Path $work 'bad.ps1')
    'Start-Sleep -Seconds 30' | Set-Content (Join-Path $work 'slow.ps1')
    'exit 0' | Set-Content (Join-Path $work 'deep.ps1')
    $compiler = Join-Path $work 'compiler with spaces.exe'
    '' | Set-Content $compiler
    $runtime = Join-Path $work 'runtime with spaces'
    New-Item -ItemType Directory $runtime | Out-Null
    $runner = Join-Path $work 'run-checks.ps1'
    $out = Join-Path $work 'fast-output'
    & powershell -NoProfile -ExecutionPolicy Bypass -File $runner -CompilerPath $compiler -RuntimeRoot $runtime -Out $out | Out-Null
    if ($LASTEXITCODE -ne 1) { throw 'Fast failures did not propagate' }
    $rows = Get-Content (Join-Path $out 'summary.json') -Raw | ConvertFrom-Json
    if ($rows.Count -ne 3 -or 'deep' -in $rows.name -or -not $rows[0].passed -or $rows[1].exit -ne 7 -or -not $rows[2].timed_out) {
        throw 'Tier isolation, forwarding, failure recording or timeout enforcement failed'
    }
    & powershell -NoProfile -ExecutionPolicy Bypass -File $runner -Tier pedantic -CompilerPath $compiler -Out (Join-Path $work 'deep-output') | Out-Null
    if ($LASTEXITCODE -ne 0) { throw 'Explicit pedantic selection failed' }
    $rows = Get-Content (Join-Path $work 'deep-output/summary.json') -Raw | ConvertFrom-Json
    if (@($rows).Count -ne 1 -or $rows.name -ne 'deep' -or -not $rows.passed) { throw 'Pedantic ran unrelated gates' }
    Write-Host 'PASS tier isolation, explicit deep selection, quoted settings, failure propagation, and gate timeout'
} finally {
    $resolved = [IO.Path]::GetFullPath($work)
    $build = [IO.Path]::GetFullPath((Join-Path $root 'build')) + '\'
    if (-not $resolved.StartsWith($build, [StringComparison]::OrdinalIgnoreCase)) { throw 'Unsafe fixture directory' }
    Remove-Item -LiteralPath $resolved -Recurse -Force
}
