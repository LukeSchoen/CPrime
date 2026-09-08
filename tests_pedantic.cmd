@echo off
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0Tests\pedantic\run.ps1" %*
exit /b %ERRORLEVEL%
