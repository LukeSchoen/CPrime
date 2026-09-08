param(
    [Parameter(Mandatory = $true)][string]$Source,
    [string]$Compiler = '',
    [string]$BaselineCompiler = '',
    [string]$RuntimeRoot = '',
    [ValidateRange(0.001, 5)][double]$Timeout = 5,
    [string[]]$Options = @('-O0'),
    [string]$Out = ''
)
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'assessment.ps1')
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
if (-not $Compiler) { $Compiler = Join-Path $root 'cpc.exe' }
if (-not $Out) { $Out = Join-Path $root ('build/gcc-speed-' + [guid]::NewGuid().ToString('N')) }
$Out = [IO.Path]::GetFullPath($Out)
$Source = (Resolve-Path -LiteralPath $Source).Path
[void][IO.Directory]::CreateDirectory($Out)
$results = @()
# Optionally compare two CPC builds on exactly the same source/options, serially.
$entries = @()
if ($BaselineCompiler) { $entries += @{ name = 'baseline-cpc'; compiler = $BaselineCompiler } }
$entries += @{ name = 'cpc'; compiler = $Compiler }
foreach ($entry in $entries) {
    $tool = (Resolve-Path -LiteralPath $entry.compiler).Path
    $artifact = Join-Path $Out ($entry.name + '.o')
    if (Test-Path -LiteralPath $artifact) { Remove-Item -LiteralPath $artifact }
    $runtimeOptions = @()
    if ($RuntimeRoot) { $runtimeOptions = @('-B' + (Resolve-Path -LiteralPath $RuntimeRoot).Path) }
    $command = @($tool) + $runtimeOptions + @('-x', 'c++') + $Options + @('-c', $Source, '-o', $artifact)
    $result = Invoke-GccProcess $command $Timeout
    $results += [ordered]@{ compiler = $entry.name; executable = $tool;
        sha256 = (Get-FileHash -LiteralPath $tool -Algorithm SHA256).Hash;
        command = $command; process = $result;
        status = Get-GccCompileStatus $result $false 'compile' $artifact }
    Write-Host "$($entry.name): $($results[-1].status), $($result.seconds) seconds"
}
$report = [ordered]@{ source = $Source; source_sha256 = (Get-FileHash -LiteralPath $Source).Hash;
    options = $Options; timeout_seconds = $Timeout; concurrency = 1;
    measurement = 'single cold process per compiler; startup included, driver and cleanup excluded; no automatic deletion';
    results = $results }
[IO.File]::WriteAllText((Join-Path $Out 'comparison.json'), ($report | ConvertTo-Json -Depth 12))
if ($results[-1].status -ne 'PASS_COMPILE') { exit 1 }
