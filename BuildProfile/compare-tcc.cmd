@echo off
setlocal

set "SCRIPT_DIR=%~dp0"
powershell -NoProfile -ExecutionPolicy Bypass -File "%SCRIPT_DIR%compare-tcc.ps1" %*
exit /b %ERRORLEVEL%
