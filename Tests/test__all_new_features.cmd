@echo off
setlocal

set "SCRIPT_DIR=%~dp0"

call "%SCRIPT_DIR%test_Constructors.cmd" %*
if errorlevel 1 exit /b %ERRORLEVEL%

call "%SCRIPT_DIR%test_Destructors.cmd" %*
if errorlevel 1 exit /b %ERRORLEVEL%

powershell -NoProfile -ExecutionPolicy Bypass -File "%SCRIPT_DIR%run.ps1" -Suite "features/InlineLifecycle" %*
if errorlevel 1 exit /b %ERRORLEVEL%

powershell -NoProfile -ExecutionPolicy Bypass -File "%SCRIPT_DIR%run.ps1" -Suite "features/All" %*
if errorlevel 1 exit /b %ERRORLEVEL%

call "%SCRIPT_DIR%test_MemberFunctions.cmd" %*
if errorlevel 1 exit /b %ERRORLEVEL%

call "%SCRIPT_DIR%test_Templates.cmd" %*
if errorlevel 1 exit /b %ERRORLEVEL%

exit /b 0
