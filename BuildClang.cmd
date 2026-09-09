@echo off
setlocal EnableExtensions
if not "%~1"=="" if /I not "%~1"=="--optimised" exit /b 2
set "ROOT=%~dp0"
if "%ROOT:~-1%"=="\" set "ROOT=%ROOT:~0,-1%"
set "BUILD_DIR=%ROOT%\build\compiler"
set "CLANG_OPT=0"
if /I "%~1"=="--optimised" set "CLANG_OPT=3"
echo Building CPC with Clang -O%CLANG_OPT% -g0.
powershell -NoProfile -ExecutionPolicy Bypass -File "%ROOT%\BuildProfile\build-cpc-clang.ps1" -OutDir "%BUILD_DIR%" -Optimization %CLANG_OPT%
if errorlevel 1 exit /b 1
move /y "%BUILD_DIR%\cpc-clang.exe" "%BUILD_DIR%\cpc.exe" >nul
if errorlevel 1 exit /b 1
call "%ROOT%\scripts\windows\build-cprime.bat" -runtime-only
if errorlevel 1 exit /b 1
call "%ROOT%\scripts\windows\publish-cpc.cmd"
exit /b %ERRORLEVEL%
