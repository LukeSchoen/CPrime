@echo off
setlocal EnableExtensions
for %%I in ("%~dp0..\..") do set "ROOT=%%~fI"
set "BUILD_DIR=%ROOT%\build\compiler"
set "NEW_CPC=%BUILD_DIR%\cpc.exe"
set "OLD_CPC=%ROOT%\cpc.exe"
set "BACKUP_CPC=%ROOT%\cpc.exe.bak"
set "PACK_SCRIPT=%ROOT%\scripts\windows\pack-portable.ps1"
set "RUNTIME_LIB=%BUILD_DIR%\lib"
if "%CPC_PACK_PROFILE%"=="" set "CPC_PACK_PROFILE=full"
if not exist "%NEW_CPC%" exit /b 1
:pack_compiler
rem The native content check skips PowerShell and all preparation on a hit.
if /I "%CPC_PACK_PROFILE%"=="full" if exist "%ROOT%\build\portable-cache\portable-payload.exe" (
  "%ROOT%\build\portable-cache\portable-payload.exe" cached "%NEW_CPC%" "%ROOT%\build\portable-cache" "%ROOT%" "%RUNTIME_LIB%"
  if errorlevel 0 if not errorlevel 1 goto :package_ready
)
powershell -NoProfile -ExecutionPolicy Bypass -File "%PACK_SCRIPT%" -ExePath "%NEW_CPC%" -RootPath "%ROOT%" -RuntimeLibPath "%RUNTIME_LIB%" -Profile "%CPC_PACK_PROFILE%"
if not "%ERRORLEVEL%"=="0" (
  echo ERROR: Portable packaging failed.
  exit /b 1
)

:package_ready
rem Validate the packaged candidate before replacing the working compiler.
powershell -NoProfile -ExecutionPolicy Bypass -File "%ROOT%\Tests\check_regressions.ps1" -CompilerPath "%NEW_CPC%"
if not "%ERRORLEVEL%"=="0" exit /b 1
if exist "%BACKUP_CPC%" del /f /q "%BACKUP_CPC%"

move /y "%OLD_CPC%" "%BACKUP_CPC%" >nul
if errorlevel 1 (
  echo ERROR: Could not back up current compiler executable.
  exit /b 1
)

move /y "%NEW_CPC%" "%ROOT%\cpc.exe" >nul
if errorlevel 1 (
  echo ERROR: Could not replace cpc.exe. Restoring backup.
  move /y "%BACKUP_CPC%" "%OLD_CPC%" >nul
  exit /b 1
)

del /f /q "%BACKUP_CPC%"
echo Success: Rebuilt and replaced "%ROOT%\cpc.exe".
exit /b 0



