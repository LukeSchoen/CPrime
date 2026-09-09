$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$work = Join-Path $root ('build/suite-runner-' + [guid]::NewGuid().ToString('N'))
try {
    New-Item -ItemType Directory -Path $work | Out-Null
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'run-all.ps1') -Destination $work
    foreach ($suite in @('c_compat', 'features/Good', 'features/Bad', 'abi/Excluded', 'pedantic/Slow')) {
        $pass = Join-Path $work ($suite + '/pass')
        New-Item -ItemType Directory -Path $pass -Force | Out-Null
        Set-Content -LiteralPath (Join-Path $pass 'test_probe.cpp') -Value 'int main() { return 0; }'
    }
    @'
param($Suite, $CompilerPath, $RuntimeRoot)
Add-Content -LiteralPath (Join-Path $PSScriptRoot 'calls.txt') -Value "$Suite|$CompilerPath|$RuntimeRoot"
if ($Suite -eq 'features/Bad') { exit 7 }
exit 0
'@ | Set-Content -LiteralPath (Join-Path $work 'run.ps1')
    $runner = Join-Path $work 'run-all.ps1'
    $listed = @(& powershell -NoProfile -File $runner -List)
    if ($LASTEXITCODE -ne 0 -or ($listed -join ',') -ne 'c_compat,features/Bad,features/Good') {
        throw 'Suite discovery included fixtures or missed a language suite'
    }
    $output = & powershell -NoProfile -File $runner -CompilerPath 'compiler with spaces' -RuntimeRoot 'runtime with spaces'
    if ($LASTEXITCODE -ne 1 -or "$output" -notmatch '2 passed, 1 failed') { throw 'Suite failure was not reported' }
    $calls = @(Get-Content -LiteralPath (Join-Path $work 'calls.txt'))
    if ($calls.Count -ne 3 -or $calls[-1] -ne 'features/Good|compiler with spaces|runtime with spaces') {
        throw 'Runner did not continue after failure or forward settings'
    }
    $output = & powershell -NoProfile -File $runner -Suite c_compat
    if ($LASTEXITCODE -ne 0 -or "$output" -notmatch '1 passed, 0 failed') { throw 'Subset selection failed' }
    $savedPreference = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    $output = & powershell -NoProfile -File $runner -Suite missing 2>&1
    $ErrorActionPreference = $savedPreference
    if ($LASTEXITCODE -eq 0) { throw 'Unknown suite passed' }
    Write-Host 'PASS suite discovery, fixture exclusion, selection, settings, and failure propagation'
} finally {
    $resolved = [IO.Path]::GetFullPath($work)
    $buildRoot = [IO.Path]::GetFullPath((Join-Path $root 'build')) + [IO.Path]::DirectorySeparatorChar
    if (-not $resolved.StartsWith($buildRoot, [StringComparison]::OrdinalIgnoreCase)) { throw 'Unexpected fixture path' }
    Remove-Item -LiteralPath $resolved -Recurse -Force
}
