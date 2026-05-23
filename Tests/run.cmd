@echo off
setlocal EnableExtensions EnableDelayedExpansion

set "SUITE=c_compat"
set "COMPILER_PATH="
set "USE_SHARED_BINARIES=0"
set "SHARED_OUT_DIR="
set "REQUIRE_SHARED_HITS=0"
for %%I in ("%~dp0.") do set "SCRIPT_DIR=%%~fI"

:parse_args
if "%~1"=="" goto args_done
if /I "%~1"=="-Suite" (
  set "SUITE=%~2"
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
if /I "%~1"=="-TccPath" (
  set "COMPILER_PATH=%~2"
  shift
  shift
  goto parse_args
)
if /I "%~1"=="-UseSharedBinaries" (
  set "USE_SHARED_BINARIES=1"
  shift
  goto parse_args
)
if /I "%~1"=="-SharedOutDir" (
  set "SHARED_OUT_DIR=%~2"
  shift
  shift
  goto parse_args
)
if /I "%~1"=="-RequireSharedHits" (
  set "REQUIRE_SHARED_HITS=1"
  shift
  goto parse_args
)
shift
goto parse_args

:args_done
for %%I in ("%SCRIPT_DIR%\..") do set "ROOT_DIR=%%~fI"

if not defined COMPILER_PATH (
  if exist "%ROOT_DIR%\cpc.exe" (
    set "COMPILER_PATH=%ROOT_DIR%\cpc.exe"
  ) else if exist "%ROOT_DIR%\win32\cpc.exe" (
    set "COMPILER_PATH=%ROOT_DIR%\win32\cpc.exe"
  )
)

if not exist "%COMPILER_PATH%" (
  echo Unable to find cpc.exe. Build first or pass -CompilerPath explicitly.
  exit /b 1
)

if defined SHARED_OUT_DIR (
  for %%I in ("%SHARED_OUT_DIR%") do set "SHARED_OUT_DIR=%%~fI"
) else (
  if exist "%SCRIPT_DIR%\batch\out" set "SHARED_OUT_DIR=%SCRIPT_DIR%\batch\out"
)

set "TESTS_ROOT=%SCRIPT_DIR%\%SUITE%"
if not exist "%TESTS_ROOT%" (
  echo Suite directory does not exist: %TESTS_ROOT%
  exit /b 1
)

set "PASS_DIR=%TESTS_ROOT%\pass"
set "FAIL_DIR=%TESTS_ROOT%\fail"

if defined CPRIME_TEST_WORKDIR (
  set "WORK_DIR=%CPRIME_TEST_WORKDIR%"
) else (
  set "WORK_DIR=%TEMP%\cprime-language-tests-%RANDOM%-%RANDOM%"
  if exist "%WORK_DIR%" rmdir /s /q "%WORK_DIR%" >nul 2>nul
)
if not exist "%WORK_DIR%" mkdir "%WORK_DIR%" >nul 2>nul
if not exist "%WORK_DIR%" (
  echo Failed to create test work dir: %WORK_DIR%
  exit /b 1
)

set /a TEST_COUNT=0
if exist "%PASS_DIR%\test_*.c" for /f %%N in ('dir /b /a-d "%PASS_DIR%\test_*.c" ^| find /c /v ""') do set /a TEST_COUNT+=%%N
if exist "%PASS_DIR%\test_*.cpp" for /f %%N in ('dir /b /a-d "%PASS_DIR%\test_*.cpp" ^| find /c /v ""') do set /a TEST_COUNT+=%%N
if exist "%FAIL_DIR%\test_*.c" for /f %%N in ('dir /b /a-d "%FAIL_DIR%\test_*.c" ^| find /c /v ""') do set /a TEST_COUNT+=%%N
if exist "%FAIL_DIR%\test_*.cpp" for /f %%N in ('dir /b /a-d "%FAIL_DIR%\test_*.cpp" ^| find /c /v ""') do set /a TEST_COUNT+=%%N
if "%TEST_COUNT%"=="0" (
  echo No test files found under %TESTS_ROOT%
  exit /b 1
)

set /a PASSED=0
set /a FAILED=0
set /a SHARED_HITS=0
set /a SHARED_MISSES=0

echo Using compiler: %COMPILER_PATH%
echo Running suite: %SUITE%
echo.

if exist "%PASS_DIR%\test_*.c" for %%F in ("%PASS_DIR%\test_*.c") do call :run_one "%%~fF"
if exist "%PASS_DIR%\test_*.cpp" for %%F in ("%PASS_DIR%\test_*.cpp") do call :run_one "%%~fF"
if exist "%FAIL_DIR%\test_*.c" for %%F in ("%FAIL_DIR%\test_*.c") do call :run_one "%%~fF"
if exist "%FAIL_DIR%\test_*.cpp" for %%F in ("%FAIL_DIR%\test_*.cpp") do call :run_one "%%~fF"

echo.
echo Summary: %PASSED% passed, %FAILED% failed
if "%USE_SHARED_BINARIES%"=="1" (
  echo Shared binaries: %SHARED_HITS% hits, %SHARED_MISSES% misses
)

if not "%FAILED%"=="0" exit /b 1
if "%USE_SHARED_BINARIES%"=="1" if "%REQUIRE_SHARED_HITS%"=="1" if "%SHARED_HITS%"=="0" (
  echo Shared mode failed: no shared binary hits were used.
  exit /b 1
)

exit /b 0

:run_one
setlocal EnableExtensions EnableDelayedExpansion
set "TEST_FILE=%~1"
set "TEST_NAME=%~nx1"
set "BASE_NAME=%~n1"
set "EXE_NAME=%BASE_NAME%.exe"
set "OUT_EXE=%WORK_DIR%\%EXE_NAME%"
set "EXPECT_EXIT=0"
set "EXPECT_STDOUT="
set "EXPECT_COMPILE_FAIL=0"
set "EXPECT_COMPILE_ARGS="

set /a _meta_lines=0
for /f "usebackq delims=" %%L in ("%TEST_FILE%") do (
  set /a _meta_lines+=1
  if !_meta_lines! GTR 12 goto meta_done
  set "_line=%%L"
  for /f "tokens=1,* delims=:" %%A in ("!_line!") do (
    if /I "%%A"=="// EXPECT_EXIT" set "EXPECT_EXIT=%%B"
    if /I "%%A"=="// EXPECT_STDOUT" set "EXPECT_STDOUT=%%B"
    if /I "%%A"=="// EXPECT_COMPILE_FAIL" set "EXPECT_COMPILE_FAIL=%%B"
    if /I "%%A"=="// EXPECT_COMPILE_ARGS" set "EXPECT_COMPILE_ARGS=%%B"
  )
)
:meta_done

for /f "tokens=* delims= " %%A in ("%EXPECT_EXIT%") do set "EXPECT_EXIT=%%A"
for /f "tokens=* delims= " %%A in ("%EXPECT_STDOUT%") do set "EXPECT_STDOUT=%%A"
for /f "tokens=* delims= " %%A in ("%EXPECT_COMPILE_FAIL%") do set "EXPECT_COMPILE_FAIL=%%A"
for /f "tokens=* delims= " %%A in ("%EXPECT_COMPILE_ARGS%") do set "EXPECT_COMPILE_ARGS=%%A"

if exist "%OUT_EXE%" del /q "%OUT_EXE%" >nul 2>nul
if not exist "%WORK_DIR%" mkdir "%WORK_DIR%" >nul 2>nul

set "COMPILE_EXIT=0"
set "COMPILE_OUTPUT="
set "EXE_TO_RUN=%OUT_EXE%"
set "SUPPORTS_SHARED=0"
if "%USE_SHARED_BINARIES%"=="1" if not "%EXPECT_COMPILE_FAIL%"=="1" if not defined EXPECT_COMPILE_ARGS if defined SHARED_OUT_DIR (
  set "SUPPORTS_SHARED=1"
)

if "%SUPPORTS_SHARED%"=="1" (
  set "SHARED_EXE=%SHARED_OUT_DIR%\%EXE_NAME%"
  if exist "!SHARED_EXE!" (
    set /a SHARED_HITS+=1
    set "EXE_TO_RUN=!SHARED_EXE!"
  ) else (
    set /a SHARED_MISSES+=1
  )
)

if /I "%EXE_TO_RUN%"=="%OUT_EXE%" (
  if defined EXPECT_COMPILE_ARGS (
    "%COMPILER_PATH%" %EXPECT_COMPILE_ARGS% "%TEST_FILE%" -o "%OUT_EXE%" >"%WORK_DIR%\compile.out" 2>&1
  ) else (
    "%COMPILER_PATH%" "%TEST_FILE%" -o "%OUT_EXE%" >"%WORK_DIR%\compile.out" 2>&1
  )
  set "COMPILE_EXIT=!ERRORLEVEL!"
  for /f "usebackq delims=" %%L in ("%WORK_DIR%\compile.out") do set "COMPILE_OUTPUT=%%L"
)

set "OK=1"
set "REASON="
if "%EXPECT_COMPILE_FAIL%"=="1" (
  if "!COMPILE_EXIT!"=="0" (
    set "OK=0"
    set "REASON=expected compile failure but compilation succeeded"
  )
) else (
  if not "!COMPILE_EXIT!"=="0" (
    set "OK=0"
    set "REASON=compile failed (exit !COMPILE_EXIT!): !COMPILE_OUTPUT!"
  ) else if not exist "%EXE_TO_RUN%" (
    set "OK=0"
    set "REASON=compile succeeded but output executable missing"
  ) else (
    "%EXE_TO_RUN%" >"%WORK_DIR%\run.out" 2>&1
    set "RUN_EXIT=!ERRORLEVEL!"
    set "RUN_STDOUT="
    for /f "usebackq delims=" %%L in ("%WORK_DIR%\run.out") do set "RUN_STDOUT=%%L"
    if not "!RUN_EXIT!"=="%EXPECT_EXIT%" (
      set "OK=0"
      set "REASON=expected exit %EXPECT_EXIT%, got !RUN_EXIT!"
    ) else if defined EXPECT_STDOUT if not "!RUN_STDOUT!"=="%EXPECT_STDOUT%" (
      set "OK=0"
      set "REASON=stdout mismatch. expected '%EXPECT_STDOUT%' got '!RUN_STDOUT!'"
    )
  )
)

if "!OK!"=="1" (
  echo PASS %TEST_NAME%
  endlocal & set /a PASSED+=1 & set /a SHARED_HITS=%SHARED_HITS% & set /a SHARED_MISSES=%SHARED_MISSES% & exit /b 0
) else (
  echo FAIL %TEST_NAME%: !REASON!
  endlocal & set /a FAILED+=1 & set /a SHARED_HITS=%SHARED_HITS% & set /a SHARED_MISSES=%SHARED_MISSES% & exit /b 0
)
