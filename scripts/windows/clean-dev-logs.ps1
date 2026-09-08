param([ValidateRange(0, 3650)][int]$Days = 7, [switch]$WhatIf)
$ErrorActionPreference = 'Stop'
$buildRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../build'))
$cutoff = (Get-Date).AddDays(-$Days)
$count = 0; $bytes = 0L
# Only disposable text logs. Keep sources, binaries, runtime and JSON evidence.
$pending = New-Object 'Collections.Generic.Stack[string]'
if (Test-Path -LiteralPath $buildRoot) {
    if ((Get-Item -LiteralPath $buildRoot).Attributes -band [IO.FileAttributes]::ReparsePoint) {
        throw 'Refusing cleanup through a redirected build directory'
    }
    $pending.Push($buildRoot)
}
while ($pending.Count) {
    foreach ($entry in Get-ChildItem -LiteralPath $pending.Pop() -Force) {
        if ($entry.FullName -eq (Join-Path $buildRoot 'gcc-upstream')) { continue }
        if ($entry.Attributes -band [IO.FileAttributes]::ReparsePoint) { continue }
        if ($entry.PSIsContainer) { $pending.Push($entry.FullName); continue }
        if ($entry.Extension -notin @('.log', '.err', '.out') -or $entry.LastWriteTime -ge $cutoff) { continue }
        $target = [IO.Path]::GetFullPath($entry.FullName)
        if (-not $target.StartsWith($buildRoot + '\', [StringComparison]::OrdinalIgnoreCase)) { throw "Outside build: $target" }
        if (-not $WhatIf) { Remove-Item -LiteralPath $target }
        ++$count; $bytes += $entry.Length
    }
}
Write-Output ('{0} logs, {1:n2} MiB; preview={2}' -f $count, ($bytes / 1MB), [bool]$WhatIf)
