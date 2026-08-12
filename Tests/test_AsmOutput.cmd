@echo off
setlocal EnableExtensions

set "SCRIPT_DIR=%~dp0"
for %%I in ("%SCRIPT_DIR%\..") do set "ROOT_DIR=%%~fI"
set "COMPILER_PATH=%ROOT_DIR%\cpc.exe"

:parse_args
if "%~1"=="" goto args_done
if /I "%~1"=="-CompilerPath" (
  set "COMPILER_PATH=%~2"
  shift
  shift
  goto parse_args
)
shift
goto parse_args

:args_done
if not exist "%COMPILER_PATH%" (
  echo Unable to find compiler: %COMPILER_PATH%
  exit /b 1
)

set "WORK_DIR=%TEMP%\cprime-asm-output-%RANDOM%-%RANDOM%"
if exist "%WORK_DIR%" rmdir /s /q "%WORK_DIR%" >nul 2>nul
mkdir "%WORK_DIR%" >nul 2>nul
if errorlevel 1 (
  echo Failed to create test work dir: %WORK_DIR%
  exit /b 1
)

set "SRC=%WORK_DIR%\hello.c"
set "ASM=%WORK_DIR%\hello.s"
set "OBJ=%WORK_DIR%\hello.o"
set "DIRECT_EXE=%WORK_DIR%\direct.exe"
set "INDIRECT_EXE=%WORK_DIR%\indirect.exe"
(
  echo int add^(int a, int b^) { return a + b; }
  echo int main^(void^)
  echo {
  echo   return add^(19, 23^) == 42 ? 0 : 1;
  echo }
) > "%SRC%"

"%COMPILER_PATH%" -S "%SRC%" -o "%ASM%" >"%WORK_DIR%\compile.out" 2>&1
if errorlevel 1 (
  echo FAIL asm output: compile failed
  type "%WORK_DIR%\compile.out"
  rmdir /s /q "%WORK_DIR%" >nul 2>nul
  exit /b 1
)

if not exist "%ASM%" (
  echo FAIL asm output: output file missing
  rmdir /s /q "%WORK_DIR%" >nul 2>nul
  exit /b 1
)

findstr /C:".text" "%ASM%" >nul
if errorlevel 1 (
  echo FAIL asm output: missing .text directive
  type "%ASM%"
  rmdir /s /q "%WORK_DIR%" >nul 2>nul
  exit /b 1
)

findstr /C:".byte" "%ASM%" >nul
if errorlevel 1 (
  echo FAIL asm output: missing .byte directives
  type "%ASM%"
  rmdir /s /q "%WORK_DIR%" >nul 2>nul
  exit /b 1
)

"%COMPILER_PATH%" -c "%ASM%" -o "%OBJ%" >"%WORK_DIR%\assemble.out" 2>&1
if errorlevel 1 (
  echo FAIL asm output: assembler rejected generated .s
  type "%WORK_DIR%\assemble.out"
  rmdir /s /q "%WORK_DIR%" >nul 2>nul
  exit /b 1
)

if not exist "%OBJ%" (
  echo FAIL asm output: assembler did not create object
  rmdir /s /q "%WORK_DIR%" >nul 2>nul
  exit /b 1
)

"%COMPILER_PATH%" "%SRC%" -o "%DIRECT_EXE%" >"%WORK_DIR%\direct-link.out" 2>&1
if errorlevel 1 (
  echo FAIL asm output: direct executable build failed
  type "%WORK_DIR%\direct-link.out"
  rmdir /s /q "%WORK_DIR%" >nul 2>nul
  exit /b 1
)

"%COMPILER_PATH%" "%OBJ%" -o "%INDIRECT_EXE%" >"%WORK_DIR%\indirect-link.out" 2>&1
if errorlevel 1 (
  echo FAIL asm output: indirect executable build failed
  type "%WORK_DIR%\indirect-link.out"
  rmdir /s /q "%WORK_DIR%" >nul 2>nul
  exit /b 1
)

fc /b "%DIRECT_EXE%" "%INDIRECT_EXE%" >"%WORK_DIR%\exe-compare.out" 2>&1
if errorlevel 1 (
  echo FAIL asm output: direct and assembly-indirect executables differ
  type "%WORK_DIR%\exe-compare.out"
  type "%ASM%"
  rmdir /s /q "%WORK_DIR%" >nul 2>nul
  exit /b 1
)

echo PASS asm output
rmdir /s /q "%WORK_DIR%" >nul 2>nul
exit /b 0
