@echo off
setlocal
set "FAILED=0"
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0Tests\run-all.ps1" %*
if errorlevel 1 set "FAILED=1"
call "%~dp0Tests\test_AsmOutput.cmd"
if errorlevel 1 set "FAILED=1"
call "%~dp0Tests\test_ReadableAsmOutput.cmd"
if errorlevel 1 set "FAILED=1"
call "%~dp0Tests\test_MultiSource.cmd"
if errorlevel 1 set "FAILED=1"
exit /b %FAILED%
