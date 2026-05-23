@echo off
setlocal

set "SCRIPT_DIR=%~dp0"
call "%SCRIPT_DIR%run.cmd" %*
exit /b %ERRORLEVEL%
