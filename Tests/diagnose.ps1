param(
    [Parameter(Mandatory = $true)][string]$Source,
    [string]$CompilerPath = '', [string]$RuntimeRoot = '',
    [string[]]$CompilerArguments = @(), [string]$Out = ''
)
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'gcc/assessment.ps1')
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
if (-not $CompilerPath) { $CompilerPath = Join-Path $root 'cpc.exe' }
if (-not $Out) { $Out = Join-Path $root ('build/diagnose-' + [guid]::NewGuid().ToString('N')) }
[void][IO.Directory]::CreateDirectory($Out)
$command = @((Resolve-Path -LiteralPath $CompilerPath).Path)
if ($RuntimeRoot) { $command += '-B' + (Resolve-Path -LiteralPath $RuntimeRoot).Path }
$command += $CompilerArguments
$command += (Resolve-Path -LiteralPath $Source).Path
$oldState = $env:CPRIME_PARSER_STATE
try {
    $env:CPRIME_PARSER_STATE = '1'
    $preprocess = Invoke-GccProcess ($command + '-E') 5
    [IO.File]::WriteAllText((Join-Path $Out 'preprocessed.txt'), $preprocess.output)
    $compileCommand = $command + @('-c', '-o', (Join-Path ([IO.Path]::GetFullPath($Out)) 'repro.o'))
    $compile = Invoke-GccProcess $compileCommand 5
    [IO.File]::WriteAllText((Join-Path $Out 'diagnostic.json'), (@{
        command=$compileCommand; compile=$compile; preprocess=$preprocess;
        compiler_sha256=(Get-FileHash -LiteralPath $command[0]).Hash;
        source_sha256=(Get-FileHash -LiteralPath $Source).Hash
    } | ConvertTo-Json -Depth 6))
    Write-Output $compile.output
    Write-Output "Diagnostic bundle: $Out"
} finally { $env:CPRIME_PARSER_STATE = $oldState }
if ($preprocess.timed_out -or $compile.timed_out -or $compile.exit -ne 0) { exit 1 }
