@echo off
setlocal

set "SCRIPT_DIR=%~dp0"

call "%SCRIPT_DIR%test_Constructors.cmd" %*
if errorlevel 1 exit /b %ERRORLEVEL%

call "%SCRIPT_DIR%test_Destructors.cmd" %*
if errorlevel 1 exit /b %ERRORLEVEL%

call "%SCRIPT_DIR%run.cmd" -Suite "features/InlineLifecycle" %*
if errorlevel 1 exit /b %ERRORLEVEL%

call "%SCRIPT_DIR%run.cmd" -Suite "features/All" %*
if errorlevel 1 exit /b %ERRORLEVEL%

call "%SCRIPT_DIR%test_MultiSource.cmd" %*
if errorlevel 1 exit /b %ERRORLEVEL%

call "%SCRIPT_DIR%test_MemberFunctions.cmd" %*
if errorlevel 1 exit /b %ERRORLEVEL%

call "%SCRIPT_DIR%test_Templates.cmd" %*
if errorlevel 1 exit /b %ERRORLEVEL%

exit /b 0
