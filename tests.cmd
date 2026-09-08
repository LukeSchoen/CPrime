@echo off
setlocal
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0Tests\run-all.ps1" -IncludeChecks %*
exit /b %ERRORLEVEL%
