@echo off
setlocal

set "ROOT=%~dp0"
if "%ROOT:~-1%"=="\" set "ROOT=%ROOT:~0,-1%"

set "OLD_CPC=%ROOT%\cpc.exe"
if not exist "%OLD_CPC%" set "OLD_CPC=%ROOT%\cpc.exe"
set "BUILD_SCRIPT=%ROOT%\build\scripts\windows\build-cprime.bat"
set "BUILD_DIR=%ROOT%\build\scripts\windows"
set "NEW_CPC=%BUILD_DIR%\cpc.exe"
if not exist "%NEW_CPC%" set "NEW_CPC=%BUILD_DIR%\cpc.exe"
set "PACK_SCRIPT=%ROOT%\build\scripts\windows\pack-portable.ps1"
set "RUNTIME_LIB=%BUILD_DIR%\lib"
set "BACKUP_CPC=%ROOT%\cpc.exe.bak"
if "%CPC_PACK_PROFILE%"=="" (
  set "CPC_PACK_PROFILE=full"
)

if not exist "%OLD_CPC%" (
  echo ERROR: Missing "%ROOT%\cpc.exe".
  exit /b 1
)

if not exist "%BUILD_SCRIPT%" (
  echo ERROR: Missing "%BUILD_SCRIPT%".
  exit /b 1
)

if not exist "%PACK_SCRIPT%" (
  echo ERROR: Missing "%PACK_SCRIPT%".
  exit /b 1
)

pushd "%BUILD_DIR%" || (
  echo ERROR: Cannot enter "%BUILD_DIR%".
  exit /b 1
)

set "CPRIME_C=..\..\..\src\compiler\driver\cprime.c"
call "%BUILD_SCRIPT%" -c "%OLD_CPC%"
if errorlevel 1 (
  echo ERROR: Build failed.
  popd
  exit /b 1
)

popd

if not exist "%NEW_CPC%" (
  echo ERROR: Built compiler not found at "%NEW_CPC%".
  exit /b 1
)

powershell -NoProfile -ExecutionPolicy Bypass -File "%PACK_SCRIPT%" -ExePath "%NEW_CPC%" -RootPath "%ROOT%" -RuntimeLibPath "%RUNTIME_LIB%" -Profile "%CPC_PACK_PROFILE%"
if errorlevel 1 (
  echo ERROR: Portable packaging failed.
  exit /b 1
)

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
