param(
    [Parameter(Mandatory = $true)][string]$ExePath,
    [Parameter(Mandatory = $true)][string]$RootPath,
    [Parameter(Mandatory = $true)][string]$RuntimeLibPath,
    [ValidateSet("none", "full")][string]$Profile = "none"
)

$ErrorActionPreference = "Stop"

function New-TempDir {
    $base = [System.IO.Path]::Combine([System.IO.Path]::GetTempPath(), "cprime-pack-" + [System.Guid]::NewGuid().ToString("N"))
    [System.IO.Directory]::CreateDirectory($base) | Out-Null
    return $base
}

function Copy-Tree {
    param([string]$From, [string]$To)
    if (-not (Test-Path $From)) {
        throw "Source path not found: $From"
    }
    [System.IO.Directory]::CreateDirectory($To) | Out-Null
    Copy-Item -Path (Join-Path $From "*") -Destination $To -Recurse -Force
}

function Copy-Winapi-Headers {
    param([string]$From, [string]$To)

    $headers = @(
        "basetsd.h",
        "basetyps.h",
        "commdlg.h",
        "dwmapi.h",
        "guiddef.h",
        "poppack.h",
        "pshpack1.h",
        "pshpack2.h",
        "pshpack4.h",
        "pshpack8.h",
        "shellapi.h",
        "uxtheme.h",
        "winapifamily.h",
        "winbase.h",
        "wincon.h",
        "windef.h",
        "windows.h",
        "windowsx.h",
        "winerror.h",
        "wingdi.h",
        "winnls.h",
        "winnt.h",
        "winreg.h",
        "winuser.h",
        "winver.h"
    )

    if (-not (Test-Path $From)) {
        throw "Source path not found: $From"
    }
    [System.IO.Directory]::CreateDirectory($To) | Out-Null
    foreach ($header in $headers) {
        Copy-Item -Path (Join-Path $From $header) -Destination $To -Force
    }
}

function Remove-C-Comments {
    param([string]$Text)

    $out = New-Object System.Text.StringBuilder
    $state = "normal"
    $i = 0
    while ($i -lt $Text.Length) {
        $c = $Text[$i]
        $next = if ($i + 1 -lt $Text.Length) { $Text[$i + 1] } else { [char]0 }

        switch ($state) {
            "normal" {
                if ($c -eq '/' -and $next -eq '/') {
                    [void]$out.Append(' ')
                    $i += 2
                    $state = "line_comment"
                    continue
                }
                if ($c -eq '/' -and $next -eq '*') {
                    [void]$out.Append(' ')
                    $i += 2
                    $state = "block_comment"
                    continue
                }
                [void]$out.Append($c)
                if ($c -eq '"') {
                    $state = "string"
                } elseif ($c -eq "'") {
                    $state = "char"
                }
            }
            "string" {
                [void]$out.Append($c)
                if ($c -eq '\') {
                    if ($i + 1 -lt $Text.Length) {
                        $i++
                        [void]$out.Append($Text[$i])
                    }
                } elseif ($c -eq '"') {
                    $state = "normal"
                }
            }
            "char" {
                [void]$out.Append($c)
                if ($c -eq '\') {
                    if ($i + 1 -lt $Text.Length) {
                        $i++
                        [void]$out.Append($Text[$i])
                    }
                } elseif ($c -eq "'") {
                    $state = "normal"
                }
            }
            "line_comment" {
                if ($c -eq "`r" -or $c -eq "`n") {
                    [void]$out.Append($c)
                    $state = "normal"
                }
            }
            "block_comment" {
                if ($c -eq "`r" -or $c -eq "`n") {
                    [void]$out.Append($c)
                } elseif ($c -eq '*' -and $next -eq '/') {
                    [void]$out.Append(' ')
                    $i++
                    $state = "normal"
                }
            }
        }

        $i++
    }

    return $out.ToString()
}

function Compress-HeaderText {
    param([string]$Path)

    $text = [System.IO.File]::ReadAllText($Path)
    $text = Remove-C-Comments -Text $text
    $lines = $text -split "\r?\n"
    $kept = New-Object System.Collections.Generic.List[string]
    foreach ($line in $lines) {
        $trimmed = $line.Trim()
        if ($trimmed.Length -eq 0) {
            continue
        }
        if ($trimmed[0] -eq '#') {
            $kept.Add(($trimmed -replace '\s+', ' '))
            continue
        }

        $collapsed = ($trimmed -replace '\s+', ' ')
        $collapsed = $collapsed -replace '\s*([,;:{}()\[\]=+\-*/%&|^!?<>~])\s*', '$1'
        $kept.Add($collapsed)
    }
    [System.IO.File]::WriteAllText($Path, (($kept -join "`n") + "`n"), [System.Text.Encoding]::ASCII)
}

function Ensure-ImpDef {
    param([string]$TccExe, [string]$LibName, [string]$OutDir)
    $def = Join-Path $OutDir ($LibName + ".def")
    if (Test-Path $def) {
        return
    }
    $dll = Join-Path $env:WINDIR ("System32\" + $LibName + ".dll")
    if (-not (Test-Path $dll)) {
        throw "Missing system DLL for import-def generation: $dll"
    }
    & $TccExe -impdef $dll -o $def
    if ($LASTEXITCODE -ne 0 -or -not (Test-Path $def)) {
        throw "Failed to generate import definition for $LibName"
    }
}

function Get-NormalizedRelativePath {
    param([string]$BaseDir, [string]$Path)
    $base = [System.IO.Path]::GetFullPath($BaseDir)
    if (-not $base.EndsWith([System.IO.Path]::DirectorySeparatorChar)) {
        $base += [System.IO.Path]::DirectorySeparatorChar
    }
    $full = [System.IO.Path]::GetFullPath($Path)
    $uriBase = New-Object System.Uri($base)
    $uriFull = New-Object System.Uri($full)
    $rel = $uriBase.MakeRelativeUri($uriFull).ToString()
    return [System.Uri]::UnescapeDataString($rel).Replace('\', '/')
}

function Add-CompressionApi {
    if ("TccPortable.CompressionApi" -as [type]) {
        return
    }

    Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;

namespace TccPortable {
    public static class CompressionApi {
        [DllImport("cabinet.dll", SetLastError = true)]
        public static extern bool CreateCompressor(
            uint Algorithm,
            IntPtr AllocationRoutines,
            out IntPtr CompressorHandle);

        [DllImport("cabinet.dll", SetLastError = true)]
        public static extern bool Compress(
            IntPtr CompressorHandle,
            byte[] UncompressedData,
            UIntPtr UncompressedDataSize,
            byte[] CompressedBuffer,
            UIntPtr CompressedBufferSize,
            out UIntPtr CompressedDataSize);

        [DllImport("cabinet.dll", SetLastError = true)]
        public static extern bool CloseCompressor(IntPtr CompressorHandle);
    }
}
'@
}

function Compress-Payload {
    param([byte[]]$Payload)

    Add-CompressionApi

    $COMPRESSION_FORMAT_LZMS = 5
    $compressor = [IntPtr]::Zero
    if (-not [TccPortable.CompressionApi]::CreateCompressor($COMPRESSION_FORMAT_LZMS, [IntPtr]::Zero, [ref]$compressor)) {
        throw "CreateCompressor failed: $([Runtime.InteropServices.Marshal]::GetLastWin32Error())"
    }

    try {
        $capacity = [Math]::Max(4096, [int]($Payload.Length + 65536))
        while ($true) {
            $compressed = New-Object byte[] $capacity
            $compressedSize = [UIntPtr]::Zero
            $ok = [TccPortable.CompressionApi]::Compress(
                $compressor,
                $Payload,
                [UIntPtr]::new([uint64]$Payload.Length),
                $compressed,
                [UIntPtr]::new([uint64]$compressed.Length),
                [ref]$compressedSize)

            if ($ok) {
                $actual = [int]$compressedSize.ToUInt64()
                if ($actual -eq $compressed.Length) {
                    return ,$compressed
                }

                $trimmed = New-Object byte[] $actual
                [Array]::Copy($compressed, $trimmed, $actual)
                return ,$trimmed
            }

            $err = [Runtime.InteropServices.Marshal]::GetLastWin32Error()
            if ($err -ne 122 -or $capacity -gt ($Payload.Length * 2 + 1048576)) {
                throw "Compress failed: $err"
            }
            $capacity *= 2
        }
    } finally {
        if ($compressor -ne [IntPtr]::Zero) {
            [TccPortable.CompressionApi]::CloseCompressor($compressor) | Out-Null
        }
    }
}


$ExePath = [System.IO.Path]::GetFullPath($ExePath)
$RootPath = [System.IO.Path]::GetFullPath($RootPath)
$RuntimeLibPath = [System.IO.Path]::GetFullPath($RuntimeLibPath)

if (-not (Test-Path $ExePath)) { throw "ExePath not found: $ExePath" }
if (-not (Test-Path $RootPath)) { throw "RootPath not found: $RootPath" }
if (-not (Test-Path $RuntimeLibPath)) { throw "RuntimeLibPath not found: $RuntimeLibPath" }

if ($Profile -eq "none") {
    return
}

$stage = New-TempDir
try {
    $stageInclude = Join-Path $stage "include"
    $stageLib = Join-Path $stage "lib"
    [System.IO.Directory]::CreateDirectory($stageInclude) | Out-Null
    [System.IO.Directory]::CreateDirectory($stageLib) | Out-Null

    Copy-Item -Path (Join-Path $RootPath "third-party\win32-sdk\include\*") -Destination $stageInclude -Recurse -Force -Exclude "winapi"
    Copy-Winapi-Headers -From (Join-Path $RootPath "third-party\win32-sdk\include\winapi") -To (Join-Path $stageInclude "winapi")
    Copy-Tree -From (Join-Path $RootPath "include\runtime") -To $stageInclude

    Get-ChildItem -Path $stageInclude -Filter "*.h" -File -Recurse |
        ForEach-Object { Compress-HeaderText -Path $_.FullName }

    Copy-Tree -From (Join-Path $RootPath "third-party\win32-sdk\lib") -To $stageLib
    Copy-Item -Path (Join-Path $RuntimeLibPath "*") -Destination $stageLib -Force
    Copy-Item -Path (Join-Path $RootPath "lib\libcprime1.a") -Destination $stageLib -Force

    $requiredLibs = @(
        "kernel32", "user32", "gdi32", "ws2_32", "msvcrt",
        "comdlg32", "shell32", "uxtheme", "dwmapi", "msimg32",
        "advapi32", "comctl32", "ole32", "oleaut32", "rpcrt4",
        "shlwapi", "windowscodecs"
    )
    foreach ($lib in $requiredLibs) {
        Ensure-ImpDef -TccExe $ExePath -LibName $lib -OutDir $stageLib
    }

    $entries = Get-ChildItem -Path $stage -File -Recurse | Sort-Object FullName
    $payloadMs = New-Object System.IO.MemoryStream
    $payloadBw = New-Object System.IO.BinaryWriter($payloadMs)
    try {
        $payloadBw.Write([UInt32]$entries.Count)
        foreach ($entry in $entries) {
            $rel = Get-NormalizedRelativePath -BaseDir $stage -Path $entry.FullName
            $relBytes = [System.Text.Encoding]::UTF8.GetBytes($rel)
            if ($relBytes.Length -gt [UInt16]::MaxValue) {
                throw "Relative path too long for payload format: $rel"
            }
            $data = [System.IO.File]::ReadAllBytes($entry.FullName)
            $payloadBw.Write([UInt16]$relBytes.Length)
            $payloadBw.Write([UInt64]$data.Length)
            $payloadBw.Write($relBytes)
            $payloadBw.Write($data)
        }
        $payloadBw.Flush()
    } finally {
        $payloadBw.Dispose()
    }

    $payloadBytes = $payloadMs.ToArray()
    $compressedPayload = [byte[]](Compress-Payload -Payload $payloadBytes)

    $exeBytes = [System.IO.File]::ReadAllBytes($ExePath)
    $payloadOffset = [UInt64]$exeBytes.Length
    $footerMagic = [System.Text.Encoding]::ASCII.GetBytes("CPCPAY11")

    $outMs = New-Object System.IO.MemoryStream
    $outBw = New-Object System.IO.BinaryWriter($outMs)
    try {
        $outBw.Write($exeBytes)
        $outBw.Write([UInt64]$payloadBytes.Length)
        $outBw.Write([UInt64]$compressedPayload.Length)
        $outBw.Write([byte[]]$compressedPayload)
        $outBw.Write($footerMagic)
        $outBw.Write($payloadOffset)
        $outBw.Flush()
        [System.IO.File]::WriteAllBytes($ExePath, $outMs.ToArray())
    } finally {
        $outBw.Dispose()
        $outMs.Dispose()
        $payloadMs.Dispose()
    }
} finally {
    if (Test-Path $stage) {
        Remove-Item -LiteralPath $stage -Recurse -Force
    }
}
