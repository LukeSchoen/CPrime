@echo off
setlocal

set "SCRIPT_DIR=%~dp0"

call "%SCRIPT_DIR%test_batch.cmd"
if errorlevel 1 exit /b %ERRORLEVEL%

call "%SCRIPT_DIR%run.cmd" -Suite "features/Constructors" -UseSharedBinaries -RequireSharedHits
exit /b %ERRORLEVEL%
