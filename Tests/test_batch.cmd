@echo off
setlocal EnableExtensions EnableDelayedExpansion

set "ROOT=%~dp0.."
set "MANIFEST=%ROOT%\Tests\batch\targets.txt"
set "OUTDIR=%ROOT%\Tests\batch\out"

if exist "%OUTDIR%" rmdir /s /q "%OUTDIR%"
mkdir "%OUTDIR%" >nul 2>nul

> "%MANIFEST%" (
  for /f "delims=" %%F in ('dir /b /s /a-d "%ROOT%\Tests\test_*.c" "%ROOT%\Tests\test_*.cpp" ^| sort') do (
    set "FILE=%%~fF"
    set "NORM=!FILE:\=/!"
    echo !NORM! | findstr /I /C:"/pass/" >nul
    if not errorlevel 1 (
      set "EXE=%OUTDIR%\%%~nF.exe"
      set "EXENORM=!EXE:\=/!"
      echo !NORM! -o !EXENORM!
    )
  )
)

pushd "%ROOT%" >nul
cpc.exe @"%MANIFEST%"
set "EXIT_CODE=%ERRORLEVEL%"
popd >nul

if not "%EXIT_CODE%"=="0" (
  echo Batch compile failed with exit code %EXIT_CODE%.
  exit /b %EXIT_CODE%
)

echo Batch compile succeeded.
exit /b 0
