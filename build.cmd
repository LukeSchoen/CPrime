@echo off
setlocal EnableExtensions
rem Prefer the host that makes CPC compile user code fastest.
if "%~1"=="" (
  call "%~dp0BuildClang.cmd" --optimised
) else (
  call "%~dp0BuildClang.cmd" %*
)
exit /b %ERRORLEVEL%
