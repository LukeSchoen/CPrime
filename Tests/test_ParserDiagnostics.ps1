param([Parameter(Mandatory = $true)][string]$CompilerPath, [string]$RuntimeRoot = '')
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'gcc/assessment.ps1')
$work = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot ('../build/parser-fixtures-' + [guid]::NewGuid().ToString('N'))))
[void][IO.Directory]::CreateDirectory($work)
$source = Join-Path $work 'invalid.cpp'
[IO.File]::WriteAllText($source, 'int main() { return missing_diagnostic_name; }')
$command = @((Resolve-Path -LiteralPath $CompilerPath).Path)
if ($RuntimeRoot) { $command += '-B' + (Resolve-Path -LiteralPath $RuntimeRoot).Path }
$oldState = $env:CPRIME_PARSER_STATE
try {
    $env:CPRIME_PARSER_STATE = $null
    $plain = Invoke-GccProcess ($command + @($source, '-c', '-o', (Join-Path $work 'plain.o'))) 5
    $env:CPRIME_PARSER_STATE = '1'
    $diagnostic = Invoke-GccProcess ($command + @($source, '-c', '-o', (Join-Path $work 'state.o'))) 5
    $stripped = $diagnostic.output -replace '(?m)^\[parser\][^\r\n]*\r?\n', ''
    if ($plain.exit -ne 1 -or $diagnostic.exit -ne 1 -or $stripped -cne $plain.output -or
        $diagnostic.output -notmatch '\[parser\] token=.*parse_flags=.*replay=.*substitution=') {
        throw "Snapshot changed diagnostics or rejection: $($diagnostic.output)"
    }
    $recovery = Join-Path $PSScriptRoot 'features/Templates/pass/test_substitution_declaration_scope_and_recovery.cpp'
    $result = Invoke-GccProcess ($command + @($recovery, '-c', '-o', (Join-Path $work 'recovery.o'))) 5
    if ($result.exit -ne 0 -or $result.timed_out -or $result.output -notmatch 'substitution=1') {
        throw "Diagnostic mode broke or missed substitution recovery: $($result.output)"
    }
} finally { $env:CPRIME_PARSER_STATE = $oldState }
Write-Output 'PASS opt-in parser snapshot, unchanged diagnostics and substitution recovery'
