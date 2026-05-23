@echo off
setlocal EnableExtensions

set "ROOT=%~dp0.."
for %%I in ("%ROOT%") do set "ROOT=%%~fI"

call "%ROOT%\deploy.cmd"
if errorlevel 1 exit /b 1

if not exist "%ROOT%\deploy\bin\libcprime.dll" (
  echo ERROR: Missing deploy\bin\libcprime.dll.
  exit /b 1
)

if not exist "%ROOT%\deploy\include\cprime\libcprime.h" (
  echo ERROR: Missing deploy\include\cprime\libcprime.h.
  exit /b 1
)

if not exist "%ROOT%\deploy\lib\libcprime.def" (
  echo ERROR: Missing deploy\lib\libcprime.def.
  exit /b 1
)

set "CLIENT=%TEMP%\cprime-deploy-client-%RANDOM%-%RANDOM%.c"
set "EXE=%TEMP%\cprime-deploy-client-%RANDOM%-%RANDOM%.exe"

> "%CLIENT%" echo #include ^<libcprime.h^>
>>"%CLIENT%" echo int main(void) {
>>"%CLIENT%" echo     CPRIMEState *s = cprime_new();
>>"%CLIENT%" echo     if (!s) return 1;
>>"%CLIENT%" echo     cprime_delete(s);
>>"%CLIENT%" echo     return 0;
>>"%CLIENT%" echo }

"%ROOT%\cpc.exe" -I"%ROOT%\deploy\include\cprime" "%CLIENT%" "%ROOT%\deploy\bin\libcprime.dll" -o "%EXE%"
if errorlevel 1 (
  del /f /q "%CLIENT%" >nul 2>nul
  exit /b 1
)

set "PATH=%ROOT%\deploy\bin;%PATH%"
"%EXE%"
set "RC=%ERRORLEVEL%"

del /f /q "%CLIENT%" "%EXE%" >nul 2>nul

if not "%RC%"=="0" (
  echo ERROR: deploy client exited with %RC%.
  exit /b %RC%
)

echo Deploy test succeeded.
exit /b 0
