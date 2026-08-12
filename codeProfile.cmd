@echo off
setlocal

set "SCRIPT_DIR=%~dp0"
call "%SCRIPT_DIR%codeProfile\run.cmd" %*
exit /b %ERRORLEVEL%
