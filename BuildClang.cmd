@echo off
setlocal EnableExtensions EnableDelayedExpansion

set "ROOT=%~dp0"
if "%ROOT:~-1%"=="\" set "ROOT=%ROOT:~0,-1%"

set "OLD_CPC=%ROOT%\cpc.exe"
if not exist "%OLD_CPC%" set "OLD_CPC=%ROOT%\cpc.exe"
set "BUILD_SCRIPT=%ROOT%\scripts\windows\build-cprime.bat"
set "BUILD_DIR=%ROOT%\build\compiler"
set "NEW_CPC=%BUILD_DIR%\cpc.exe"
if not exist "%NEW_CPC%" set "NEW_CPC=%BUILD_DIR%\cpc.exe"
set "PACK_SCRIPT=%ROOT%\scripts\windows\pack-portable.ps1"
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

if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
if /I not "%~1"=="--self" goto :clang_host
pushd "%BUILD_DIR%" || (
  echo ERROR: Cannot enter "%BUILD_DIR%".
  exit /b 1
)

call :run_build "%OLD_CPC%"
if errorlevel 1 (
  echo WARN: Build failed with "%OLD_CPC%". Trying tracked bootstrap compilers.
  call :try_git_bootstrap
  if errorlevel 1 (
    echo ERROR: Build failed.
    popd
    exit /b 1
  )
)

popd

if not exist "%NEW_CPC%" (
  echo ERROR: Built compiler not found at "%NEW_CPC%".
  exit /b 1
)

goto :pack_compiler

:clang_host
set "CLANG_OPT=0"
if /I "%~1"=="--optimised" set "CLANG_OPT=3"
echo Building CPrime compiler host with Clang -O%CLANG_OPT% -g0.
powershell -NoProfile -ExecutionPolicy Bypass -File "%ROOT%\BuildProfile\build-cpc-clang.ps1" -OutDir "%BUILD_DIR%" -Optimization %CLANG_OPT%
if errorlevel 1 exit /b 1
move /y "%BUILD_DIR%\cpc-clang.exe" "%NEW_CPC%" >nul
if errorlevel 1 exit /b 1
rem Build only the target runtime with the new host; avoid a redundant self-build.
pushd "%BUILD_DIR%" || exit /b 1
call "%BUILD_SCRIPT%" -runtime-only
set "RUNTIME_RESULT=%ERRORLEVEL%"
popd
if not "%RUNTIME_RESULT%"=="0" exit /b %RUNTIME_RESULT%

:pack_compiler
rem The native content check skips PowerShell and all preparation on a hit.
if /I "%CPC_PACK_PROFILE%"=="full" if exist "%ROOT%\build\portable-cache\portable-payload.exe" (
  "%ROOT%\build\portable-cache\portable-payload.exe" cached "%NEW_CPC%" "%ROOT%\build\portable-cache" "%ROOT%" "%RUNTIME_LIB%"
  if errorlevel 0 if not errorlevel 1 goto :package_ready
)
powershell -NoProfile -ExecutionPolicy Bypass -File "%PACK_SCRIPT%" -ExePath "%NEW_CPC%" -RootPath "%ROOT%" -RuntimeLibPath "%RUNTIME_LIB%" -Profile "%CPC_PACK_PROFILE%"
if errorlevel 1 (
  echo ERROR: Portable packaging failed.
  exit /b 1
)

:package_ready
rem Validate the packaged candidate before replacing the working compiler.
powershell -NoProfile -ExecutionPolicy Bypass -File "%ROOT%\Tests\check_regressions.ps1" -CompilerPath "%NEW_CPC%"
if errorlevel 1 exit /b 1
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

:run_build
call "%BUILD_SCRIPT%" -c "%~1"
exit /b %ERRORLEVEL%

:try_git_bootstrap
where git.exe >nul 2>nul
if errorlevel 1 exit /b 1

set "BOOT_TMP=%TEMP%\cprime-bootstrap-%RANDOM%-%RANDOM%"
mkdir "%BOOT_TMP%" >nul 2>nul
if errorlevel 1 exit /b 1

for /f %%H in ('git.exe -C "%ROOT%" log --format^=%%H -- cpc.exe') do (
  set "BOOT_CPC=%BOOT_TMP%\cpc-%%H.exe"
  git.exe -C "%ROOT%" show %%H:cpc.exe > "!BOOT_CPC!"
  if exist "!BOOT_CPC!" (
    echo Trying bootstrap compiler %%H.
    call :run_build "!BOOT_CPC!"
    if not errorlevel 1 (
      rmdir /s /q "%BOOT_TMP%" >nul 2>nul
      exit /b 0
    )
  )
)

rmdir /s /q "%BOOT_TMP%" >nul 2>nul
exit /b 1
