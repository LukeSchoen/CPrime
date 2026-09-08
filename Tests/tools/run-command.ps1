param([string]$ScriptPath, [string]$CompilerPath = '', [string]$RuntimeRoot = '')
$ErrorActionPreference = 'Stop'
$arguments = @()
if ($CompilerPath) { $arguments += @('-CompilerPath', $CompilerPath) }
if ($RuntimeRoot) { $arguments += @('-RuntimeRoot', $RuntimeRoot) }
& $ScriptPath @arguments
exit $LASTEXITCODE
