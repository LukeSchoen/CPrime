@echo off
setlocal EnableExtensions

set "SCRIPT_DIR=%~dp0"
for %%I in ("%SCRIPT_DIR%\..") do set "ROOT_DIR=%%~fI"
set "COMPILER_PATH=%ROOT_DIR%\cpc.exe"
set "YASM_PATH=%ROOT_DIR%\third-party\yasm\yasm.exe"

set "RUNTIME_ARGS="

:parse_args
if "%~1"=="" goto args_done
if /I "%~1"=="-RuntimeRoot" (
  for %%I in ("%~2") do set RUNTIME_ARGS="-B%%~fI"
  shift
  shift
  goto parse_args
)
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

if not exist "%YASM_PATH%" (
  echo Unable to find yasm: %YASM_PATH%
  exit /b 1
)

set "WORK_DIR=%TEMP%\cprime-readable-asm-%RANDOM%-%RANDOM%"
if exist "%WORK_DIR%" rmdir /s /q "%WORK_DIR%" >nul 2>nul
mkdir "%WORK_DIR%" >nul 2>nul
if errorlevel 1 (
  echo Failed to create test work dir: %WORK_DIR%
  exit /b 1
)

set "SRC=%WORK_DIR%\basic.c"
set "ASM=%WORK_DIR%\basic.s"
set "OBJ=%WORK_DIR%\basic.o"
set "EXE=%WORK_DIR%\basic.exe"

(
  echo int add^(int a, int b^) { return a + b; }
  echo int main^(void^) { int x = add^(19, 23^); if ^(x == 42^) return x - 42; return 1; }
) > "%SRC%"

"%COMPILER_PATH%" %RUNTIME_ARGS% -S "%SRC%" -o "%ASM%" >"%WORK_DIR%\compile.out" 2>&1
if errorlevel 1 (
  echo FAIL readable asm: compile failed
  type "%WORK_DIR%\compile.out"
  goto fail
)

findstr /C:"movq" "%ASM%" >nul
if errorlevel 1 (
  echo FAIL readable asm: missing movq instruction
  type "%ASM%"
  goto fail
)

findstr /C:"addl" "%ASM%" >nul
if errorlevel 1 (
  echo FAIL readable asm: missing addl instruction
  type "%ASM%"
  goto fail
)

findstr /C:"jne" "%ASM%" >nul
if errorlevel 1 (
  echo FAIL readable asm: missing branch instruction
  type "%ASM%"
  goto fail
)

findstr /C:"section bytes" "%ASM%" >nul
if not errorlevel 1 (
  echo FAIL readable asm: emitted byte-serializer header
  type "%ASM%"
  goto fail
)

"%YASM_PATH%" -p gas -f elf64 "%ASM%" -o "%OBJ%" >"%WORK_DIR%\yasm.out" 2>&1
if errorlevel 1 (
  echo FAIL readable asm: yasm rejected output
  type "%WORK_DIR%\yasm.out"
  type "%ASM%"
  goto fail
)

"%COMPILER_PATH%" %RUNTIME_ARGS% "%OBJ%" -o "%EXE%" >"%WORK_DIR%\link.out" 2>&1
if errorlevel 1 (
  echo FAIL readable asm: link failed
  type "%WORK_DIR%\link.out"
  goto fail
)

"%EXE%"
if errorlevel 1 (
  echo FAIL readable asm: executable returned %ERRORLEVEL%
  goto fail
)

echo PASS readable asm output
rmdir /s /q "%WORK_DIR%" >nul 2>nul
exit /b 0

:fail
rmdir /s /q "%WORK_DIR%" >nul 2>nul
exit /b 1
