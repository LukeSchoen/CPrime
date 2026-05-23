@echo off
setlocal EnableExtensions

set "SCRIPT_DIR=%~dp0"
set "CPRIME_TEST_WORKDIR=%TEMP%\cprime-language-tests-%RANDOM%-%RANDOM%"
if exist "%CPRIME_TEST_WORKDIR%" rmdir /s /q "%CPRIME_TEST_WORKDIR%" >nul 2>nul
mkdir "%CPRIME_TEST_WORKDIR%" >nul 2>nul
if errorlevel 1 (
  echo Failed to create shared test work dir: %CPRIME_TEST_WORKDIR%
  exit /b 1
)

call "%SCRIPT_DIR%Tests\test_batch.cmd"
if errorlevel 1 goto finish

cmd /c "%SCRIPT_DIR%Tests\test__all_new_features.cmd"
set "EXIT_CODE=%ERRORLEVEL%"

echo.
if "%EXIT_CODE%"=="0" (
  echo Full test suite passed.
) else (
  echo Full test suite failed with exit code %EXIT_CODE%.
)

timeout /t 3 /nobreak >nul
goto finish

:finish
if not defined EXIT_CODE set "EXIT_CODE=%ERRORLEVEL%"
if exist "%CPRIME_TEST_WORKDIR%" rmdir /s /q "%CPRIME_TEST_WORKDIR%" >nul 2>nul
exit /b %EXIT_CODE%
