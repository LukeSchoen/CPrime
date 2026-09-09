@echo off
setlocal EnableExtensions
rem CPC is the default and only host here. Other hosts have named entry points.
if not "%~1"=="" (
  echo Usage: Build.cmd
  exit /b 2
)
set "ROOT=%~dp0"
if "%ROOT:~-1%"=="\" set "ROOT=%ROOT:~0,-1%"
if not exist "%ROOT%\cpc.exe" (
  echo ERROR: Missing root cpc.exe.
  exit /b 1
)
echo Building CPC with "%ROOT%\cpc.exe".
call "%ROOT%\scripts\windows\build-cprime.bat" -c "%ROOT%\cpc.exe"
if not "%ERRORLEVEL%"=="0" exit /b 1
call "%ROOT%\scripts\windows\publish-cpc.cmd"
exit /b %ERRORLEVEL%
