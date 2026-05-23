@echo off
setlocal EnableExtensions EnableDelayedExpansion

set "SCRIPT_DIR=%~dp0"
for %%I in ("%SCRIPT_DIR%..") do set "ROOT=%%~fI"
set "COMPILER=%ROOT%\cpc.exe"
set "SRC_DIR=%SCRIPT_DIR%features\MultiSource\pass"
set "WORK_DIR=%TEMP%\cprime-multisource-tests-%RANDOM%-%RANDOM%"

if not exist "%COMPILER%" (
  echo Unable to find cpc.exe at %COMPILER%.
  exit /b 1
)

if exist "%WORK_DIR%" rmdir /s /q "%WORK_DIR%" >nul 2>nul
mkdir "%WORK_DIR%" >nul 2>nul
if errorlevel 1 (
  echo Failed to create test work dir: %WORK_DIR%
  exit /b 1
)

set "A_SRC=%SRC_DIR%\header_inline_member_a.cpp"
set "MAIN_SRC=%SRC_DIR%\header_inline_member_main.cpp"

"%COMPILER%" "%A_SRC%" "%MAIN_SRC%" -o "%WORK_DIR%\whole.exe" >"%WORK_DIR%\whole.out" 2>&1
set "WHOLE_EXIT=%ERRORLEVEL%"
if not "%WHOLE_EXIT%"=="0" (
  echo FAIL header inline member whole-program compile: exit %WHOLE_EXIT%
  type "%WORK_DIR%\whole.out"
  goto fail
)
findstr /I /C:"defined twice" "%WORK_DIR%\whole.out" >nul 2>nul
if not errorlevel 1 (
  echo FAIL header inline member whole-program compile emitted duplicate-symbol diagnostics.
  type "%WORK_DIR%\whole.out"
  goto fail
)
"%WORK_DIR%\whole.exe"
if errorlevel 1 (
  echo FAIL header inline member whole-program executable returned %ERRORLEVEL%.
  goto fail
)

"%COMPILER%" -c "%A_SRC%" -o "%WORK_DIR%\a.o" >"%WORK_DIR%\compile_a.out" 2>&1
if errorlevel 1 (
  echo FAIL header inline member object compile a.cpp.
  type "%WORK_DIR%\compile_a.out"
  goto fail
)
"%COMPILER%" -c "%MAIN_SRC%" -o "%WORK_DIR%\main.o" >"%WORK_DIR%\compile_main.out" 2>&1
if errorlevel 1 (
  echo FAIL header inline member object compile main.cpp.
  type "%WORK_DIR%\compile_main.out"
  goto fail
)
"%COMPILER%" "%WORK_DIR%\a.o" "%WORK_DIR%\main.o" -o "%WORK_DIR%\linked.exe" >"%WORK_DIR%\link.out" 2>&1
set "LINK_EXIT=%ERRORLEVEL%"
if not "%LINK_EXIT%"=="0" (
  echo FAIL header inline member object link: exit %LINK_EXIT%
  type "%WORK_DIR%\link.out"
  goto fail
)
findstr /I /C:"defined twice" "%WORK_DIR%\link.out" >nul 2>nul
if not errorlevel 1 (
  echo FAIL header inline member object link emitted duplicate-symbol diagnostics.
  type "%WORK_DIR%\link.out"
  goto fail
)
"%WORK_DIR%\linked.exe"
if errorlevel 1 (
  echo FAIL header inline member linked executable returned %ERRORLEVEL%.
  goto fail
)

echo PASS header inline member multi-source

set "LIFE_SRC=%SRC_DIR%\external_lifecycle.cpp"
set "LIFE_MAIN_SRC=%SRC_DIR%\external_lifecycle_main.cpp"

"%COMPILER%" -c "%LIFE_SRC%" -o "%WORK_DIR%\external_lifecycle.o" >"%WORK_DIR%\compile_lifecycle.out" 2>&1
if errorlevel 1 (
  echo FAIL external lifecycle object compile.
  type "%WORK_DIR%\compile_lifecycle.out"
  goto fail
)
"%COMPILER%" -c "%LIFE_MAIN_SRC%" -o "%WORK_DIR%\external_lifecycle_main.o" >"%WORK_DIR%\compile_lifecycle_main.out" 2>&1
if errorlevel 1 (
  echo FAIL external lifecycle main object compile.
  type "%WORK_DIR%\compile_lifecycle_main.out"
  goto fail
)
"%COMPILER%" "%WORK_DIR%\external_lifecycle.o" "%WORK_DIR%\external_lifecycle_main.o" -o "%WORK_DIR%\external_lifecycle.exe" >"%WORK_DIR%\link_lifecycle.out" 2>&1
if errorlevel 1 (
  echo FAIL external lifecycle object link.
  type "%WORK_DIR%\link_lifecycle.out"
  goto fail
)
"%WORK_DIR%\external_lifecycle.exe"
if errorlevel 1 (
  echo FAIL external lifecycle linked executable returned %ERRORLEVEL%.
  goto fail
)

echo PASS external lifecycle multi-source
rmdir /s /q "%WORK_DIR%" >nul 2>nul
exit /b 0

:fail
if exist "%WORK_DIR%" rmdir /s /q "%WORK_DIR%" >nul 2>nul
exit /b 1
