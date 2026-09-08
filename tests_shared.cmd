@echo off
setlocal
call "%~dp0Tests\test_batch.cmd"
if errorlevel 1 exit /b %ERRORLEVEL%
call "%~dp0tests.cmd" -UseSharedBinaries %*
exit /b %ERRORLEVEL%
