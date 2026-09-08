@echo off
call "%~dp0Build.cmd" --self %*
exit /b %ERRORLEVEL%
