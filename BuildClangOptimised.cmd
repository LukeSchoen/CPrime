@echo off
setlocal EnableExtensions
call "%~dp0BuildClang.cmd" --optimised
exit /b %ERRORLEVEL%
