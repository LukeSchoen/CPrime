@echo off
setlocal EnableExtensions
call "%~dp0BuildClang.cmd" --self
exit /b %ERRORLEVEL%
