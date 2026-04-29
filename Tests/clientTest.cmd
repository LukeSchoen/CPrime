@echo off
setlocal

set "ROOT=%~dp0"
if "%ROOT:~-1%"=="\" set "ROOT=%ROOT:~0,-1%"

set "CPC_EXE=%ROOT%\..\cpc.exe"
if not exist "%CPC_EXE%" (
  echo ERROR: Missing cpc executable in "%ROOT%".
  exit /b 1
)

set "FAIL=0"

call :build_app "%ROOT%\..\..\clGit\clGit" "build.bat"
if errorlevel 1 set "FAIL=1"

call :build_app "%ROOT%\..\..\textSuite\text" "build.cmd"
if errorlevel 1 set "FAIL=1"

if not "%FAIL%"=="0" (
  echo Client test failed.
  exit /b 1
)

echo Client test succeeded.
exit /b 0

:build_app
set "APP_DIR=%~1"
set "BUILD_SCRIPT=%~2"

for %%I in ("%APP_DIR%") do set "APP_DIR=%%~fI"

set "CPC_DATA=%LOCALAPPDATA%\cprime"
if exist "%CPC_DATA%" (
  rmdir /s /q "%CPC_DATA%"
  if exist "%CPC_DATA%" (
    echo ERROR: Could not remove "%CPC_DATA%".
    exit /b 1
  )
)

if not exist "%APP_DIR%\%BUILD_SCRIPT%" (
  echo ERROR: Missing build script "%APP_DIR%\%BUILD_SCRIPT%".
  exit /b 1
)

copy /y "%CPC_EXE%" "%APP_DIR%\cpc.exe" >nul
if errorlevel 1 (
  echo ERROR: Could not stage "%CPC_EXE%" into "%APP_DIR%".
  exit /b 1
)

pushd "%APP_DIR%" || (
  echo ERROR: Could not enter "%APP_DIR%".
  exit /b 1
)

call "%BUILD_SCRIPT%"
set "RC=%errorlevel%"
popd

if not "%RC%"=="0" (
  echo ERROR: Build failed in "%APP_DIR%".
  exit /b %RC%
)

echo OK: %APP_DIR%\%BUILD_SCRIPT%
exit /b 0
