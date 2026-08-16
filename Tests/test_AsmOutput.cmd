@echo off
setlocal EnableExtensions

set "SCRIPT_DIR=%~dp0"
for %%I in ("%SCRIPT_DIR%\..") do set "ROOT_DIR=%%~fI"
set "COMPILER_PATH=%ROOT_DIR%\cpc.exe"
set "YASM_PATH=%ROOT_DIR%\third-party\yasm\yasm.exe"

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

set "BASIC_SRC=%WORK_DIR%\basic.c"
(
  echo int add^(int a, int b^) { return a + b; }
  echo int main^(void^)
  echo {
  echo   return add^(19, 23^) == 42 ? 0 : 1;
  echo }
) > "%BASIC_SRC%"

set "GLOBALS_SRC=%WORK_DIR%\globals.c"
(
  echo int g = 7;
  echo int nums[5] = { 1, 2, 3, 4, 5 };
  echo char msg[] = "abc";
  echo int zeroes[8];
  echo int main^(void^)
  echo {
  echo   zeroes[3] = nums[0] + nums[4] + msg[1];
  echo   if ^(zeroes[3] != 104^) return 1;
  echo   if ^(g != 7^) return 2;
  echo   return 0;
  echo }
) > "%GLOBALS_SRC%"

set "STRUCTS_SRC=%WORK_DIR%\structs.c"
(
  echo typedef struct { int x; int y; } Point;
  echo Point base = { 11, 31 };
  echo Point make^(int n^)
  echo {
  echo   Point p;
  echo   p.x = base.x + n;
  echo   p.y = base.y - n;
  echo   return p;
  echo }
  echo int main^(void^)
  echo {
  echo   Point p = make^(3^);
  echo   if ^(p.x != 14^) return 1;
  echo   if ^(p.y != 28^) return 2;
  echo   return 0;
  echo }
) > "%STRUCTS_SRC%"

call :run_case basic "%BASIC_SRC%"
if errorlevel 1 goto fail

call :run_case globals "%GLOBALS_SRC%"
if errorlevel 1 goto fail

call :run_case array_loop "%ROOT_DIR%\Tests\c_compat\pass\test_array_loop.c"
if errorlevel 1 goto fail

call :run_case function_pointer "%ROOT_DIR%\Tests\c_compat\pass\test_function_pointer.c"
if errorlevel 1 goto fail

call :run_case struct_return "%STRUCTS_SRC%"
if errorlevel 1 goto fail

call :run_case member_initializer "%ROOT_DIR%\Tests\features\Constructors\pass\test_member_initializer_list.cpp"
if errorlevel 1 goto fail

echo PASS asm output
rmdir /s /q "%WORK_DIR%" >nul 2>nul
exit /b 0

:fail
rmdir /s /q "%WORK_DIR%" >nul 2>nul
exit /b 1

:run_case
set "CASE_NAME=%~1"
set "SRC=%~2"
set "ASM=%WORK_DIR%\%CASE_NAME%.s"
set "OBJ=%WORK_DIR%\%CASE_NAME%.o"
set "YASM_OBJ=%WORK_DIR%\%CASE_NAME%-yasm.o"
set "DIRECT_EXE=%WORK_DIR%\%CASE_NAME%-direct.exe"
set "INDIRECT_EXE=%WORK_DIR%\%CASE_NAME%-indirect.exe"
set "YASM_EXE=%WORK_DIR%\%CASE_NAME%-yasm.exe"

if not exist "%SRC%" (
  echo FAIL asm output %CASE_NAME%: source missing: %SRC%
  exit /b 1
)

"%COMPILER_PATH%" -Sbytes "%SRC%" -o "%ASM%" >"%WORK_DIR%\%CASE_NAME%-compile.out" 2>&1
if errorlevel 1 (
  echo FAIL asm output %CASE_NAME%: compile failed
  type "%WORK_DIR%\%CASE_NAME%-compile.out"
  exit /b 1
)

if not exist "%ASM%" (
  echo FAIL asm output %CASE_NAME%: output file missing
  exit /b 1
)

findstr /C:".text" "%ASM%" >nul
if errorlevel 1 (
  echo FAIL asm output %CASE_NAME%: missing .text directive
  type "%ASM%"
  exit /b 1
)

findstr /C:".byte" "%ASM%" >nul
if errorlevel 1 (
  echo FAIL asm output %CASE_NAME%: missing .byte directives
  type "%ASM%"
  exit /b 1
)

"%COMPILER_PATH%" -c "%ASM%" -o "%OBJ%" >"%WORK_DIR%\%CASE_NAME%-assemble.out" 2>&1
if errorlevel 1 (
  echo FAIL asm output %CASE_NAME%: assembler rejected generated .s
  type "%WORK_DIR%\%CASE_NAME%-assemble.out"
  type "%ASM%"
  exit /b 1
)

if not exist "%OBJ%" (
  echo FAIL asm output %CASE_NAME%: assembler did not create object
  exit /b 1
)

"%COMPILER_PATH%" "%SRC%" -o "%DIRECT_EXE%" >"%WORK_DIR%\%CASE_NAME%-direct-link.out" 2>&1
if errorlevel 1 (
  echo FAIL asm output %CASE_NAME%: direct executable build failed
  type "%WORK_DIR%\%CASE_NAME%-direct-link.out"
  exit /b 1
)

"%COMPILER_PATH%" "%OBJ%" -o "%INDIRECT_EXE%" >"%WORK_DIR%\%CASE_NAME%-indirect-link.out" 2>&1
if errorlevel 1 (
  echo FAIL asm output %CASE_NAME%: indirect executable build failed
  type "%WORK_DIR%\%CASE_NAME%-indirect-link.out"
  exit /b 1
)

fc /b "%DIRECT_EXE%" "%INDIRECT_EXE%" >"%WORK_DIR%\%CASE_NAME%-exe-compare.out" 2>&1
if errorlevel 1 (
  echo FAIL asm output %CASE_NAME%: direct and assembly-indirect executables differ
  type "%WORK_DIR%\%CASE_NAME%-exe-compare.out"
  type "%ASM%"
  exit /b 1
)

if exist "%YASM_PATH%" (
  "%YASM_PATH%" -p gas -f elf64 "%ASM%" -o "%YASM_OBJ%" >"%WORK_DIR%\%CASE_NAME%-yasm-assemble.out" 2>&1
  if errorlevel 1 (
    echo FAIL asm output %CASE_NAME%: yasm rejected generated .s
    type "%WORK_DIR%\%CASE_NAME%-yasm-assemble.out"
    type "%ASM%"
    exit /b 1
  )

  "%COMPILER_PATH%" "%YASM_OBJ%" -o "%YASM_EXE%" >"%WORK_DIR%\%CASE_NAME%-yasm-link.out" 2>&1
  if errorlevel 1 (
    echo FAIL asm output %CASE_NAME%: yasm object link failed
    type "%WORK_DIR%\%CASE_NAME%-yasm-link.out"
    exit /b 1
  )

  fc /b "%DIRECT_EXE%" "%YASM_EXE%" >"%WORK_DIR%\%CASE_NAME%-yasm-exe-compare.out" 2>&1
  if errorlevel 1 (
    echo FAIL asm output %CASE_NAME%: direct and yasm-indirect executables differ
    type "%WORK_DIR%\%CASE_NAME%-yasm-exe-compare.out"
    type "%ASM%"
    exit /b 1
  )
)

exit /b 0
