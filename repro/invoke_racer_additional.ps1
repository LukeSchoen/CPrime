param(
    [Parameter(Mandatory = $true)]
    [string]$Name,
    [int]$TimeoutSeconds = 10
)

$ErrorActionPreference = "Stop"

$root = "C:\Luke\Src\OT\cl"
$outDir = Join-Path $root "builds\racer"
$cpc = Join-Path (Resolve-Path "$PSScriptRoot\..").Path "cpc.exe"
$commonLib = Join-Path $root "CommonLib\commonLib"
$flagsFile = Join-Path $root "builds\core\generated_core_cpc_flags.rsp"
$flags = Get-Content -LiteralPath $flagsFile | Where-Object { $_.Trim() }
$flags += "-D_MSC_VER=1900"
$flags += "-DMA_NO_SSE2"
$flags += "-DMA_NO_AVX2"
$flags += ('-I"' + (Join-Path $root "CommonLib\3rdParty\Imagine\include") + '"')

$sources = @{
    cl2DDraw = "Polygon\cl2DDraw.cpp"
    clCamera = "UI\clCamera.cpp"
    clImage = "Raster\clImage.cpp"
    clString = "Strings\clString.cpp"
    clRenderObjectCore = "Polygon\clRenderObjectCore.cpp"
    clRenderObject = "Polygon\clRenderObject.cpp"
}

if (-not $sources.ContainsKey($Name)) {
    throw "Unknown additional source '$Name'"
}

$src = Join-Path $commonLib ("src\" + $sources[$Name])
$obj = Join-Path $outDir ("focus_" + $Name + ".obj")
$log = Join-Path $outDir ("focus_" + $Name + ".log")
$err = $log + ".err"
Remove-Item -LiteralPath $obj, $log, $err -Force -ErrorAction SilentlyContinue

$args = @($flags) + @("-c", $src, "-o", $obj)
$proc = Start-Process -FilePath $cpc -ArgumentList $args `
    -RedirectStandardOutput $log -RedirectStandardError $err `
    -PassThru -NoNewWindow

$timedOut = -not $proc.WaitForExit($TimeoutSeconds * 1000)
if ($timedOut) {
    Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue
    $proc.WaitForExit()
    Add-Content -LiteralPath $log -Encoding ASCII -Value "TIMEOUT"
    $exit = -999
} else {
    # Complete asynchronous redirected-stream handling before reading ExitCode.
    $proc.WaitForExit()
    $proc.Refresh()
    $exit = $proc.ExitCode
}
if (Test-Path -LiteralPath $err) {
    Get-Content -LiteralPath $err | Add-Content -LiteralPath $log
}

Write-Host "exit=$exit timeout=$timedOut objExists=$(Test-Path -LiteralPath $obj) log=$log"
if (Test-Path -LiteralPath $log) {
    Get-Content -LiteralPath $log -Tail 60
}
exit $(if ($exit -eq 0 -and (Test-Path -LiteralPath $obj)) { 0 } else { 1 })
