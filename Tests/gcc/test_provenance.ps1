$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'provenance.ps1')
$work = Join-Path $PSScriptRoot ('../../build/provenance-fixture-' + [guid]::NewGuid().ToString('N'))
$work = [IO.Path]::GetFullPath($work)
[void][IO.Directory]::CreateDirectory($work)
$a = Join-Path $work 'a.h'
$b = Join-Path $work 'b.lib'
[IO.File]::WriteAllText($a, 'first')
[IO.File]::WriteAllText($b, 'second')
$before = @(Get-GccInputIdentity $work @($b, $a))
$reordered = @(Get-GccInputIdentity $work @($a, $b))
if (($before | ConvertTo-Json) -cne ($reordered | ConvertTo-Json)) { throw 'Enumeration order changed identity' }
if ($before.Count -ne 2 -or $before[0].path -cne 'a.h') { throw 'Missing or non-relative identity' }
[IO.File]::WriteAllText($a, 'changed')
$after = @(Get-GccInputIdentity $work @($a, $b))
if ($before[0].sha256 -eq $after[0].sha256 -or $before[1].sha256 -ne $after[1].sha256) { throw 'Content change not identified precisely' }
if (@(Get-GccInputIdentity $work @()).Count -ne 0) { throw 'Empty input identity is incorrect' }
$rejected = $false
try { Get-GccInputIdentity $work @($PSCommandPath) } catch { $rejected = $true }
if (-not $rejected) { throw 'Outside-root input accepted' }
Write-Output 'PASS provenance order, content changes, relative paths, empty inputs, and root containment'
