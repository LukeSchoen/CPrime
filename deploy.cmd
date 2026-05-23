@echo off
setlocal EnableExtensions

set "ROOT=%~dp0"
if "%ROOT:~-1%"=="\" set "ROOT=%ROOT:~0,-1%"

set "DEPLOY=%ROOT%\deploy"
set "CPC=%ROOT%\cpc.exe"
set "DLL=%DEPLOY%\bin\libcprime.dll"
set "DEF=%DEPLOY%\lib\libcprime.def"
set "IMPLIB=%DEPLOY%\lib\libcprime.lib"
set "PUBLIC_HEADER=%ROOT%\include\cprime\libcprime.h"

if not exist "%CPC%" (
  echo ERROR: Missing "%CPC%".
  exit /b 1
)

if not exist "%PUBLIC_HEADER%" (
  echo ERROR: Missing "%PUBLIC_HEADER%".
  exit /b 1
)

if not exist "%DEPLOY%" mkdir "%DEPLOY%"
if not exist "%DEPLOY%\bin" mkdir "%DEPLOY%\bin"
if not exist "%DEPLOY%\include" mkdir "%DEPLOY%\include"
if not exist "%DEPLOY%\include\cprime" mkdir "%DEPLOY%\include\cprime"
if not exist "%DEPLOY%\lib" mkdir "%DEPLOY%\lib"

copy /y "%PUBLIC_HEADER%" "%DEPLOY%\include\cprime\libcprime.h" >nul
if errorlevel 1 (
  echo ERROR: Failed to copy public libcprime header.
  exit /b 1
)

"%CPC%" ^
  -I"%ROOT%\include\runtime" ^
  -I"%ROOT%\include\cprime" ^
  -I"%ROOT%\third-party\win32-sdk\include" ^
  -I"%ROOT%\third-party\win32-sdk\include\winapi" ^
  -I"%ROOT%\src\compiler\frontend" ^
  -I"%ROOT%\src\compiler\middleend" ^
  -I"%ROOT%\src\compiler\backend\x64" ^
  -I"%ROOT%" ^
  -DCPRIME_TARGET_PE ^
  -DCPRIME_TARGET_X86_64 ^
  -DLIBCPRIME_AS_DLL ^
  -shared "%ROOT%\src\compiler\middleend\libcprime.c" ^
  -o "%DLL%"
if errorlevel 1 (
  echo ERROR: Failed to build "%DLL%".
  exit /b 1
)
if exist "%DEPLOY%\bin\libcprime.def" del /f /q "%DEPLOY%\bin\libcprime.def"

"%CPC%" -impdef "%DLL%" -o "%DEF%"
if errorlevel 1 (
  echo ERROR: Failed to generate "%DEF%".
  exit /b 1
)

set "MADE_IMPLIB=0"
where lib.exe >nul 2>nul
if not errorlevel 1 (
  lib.exe /nologo /def:"%DEF%" /out:"%IMPLIB%" /machine:x64 >nul
  if errorlevel 1 (
    echo ERROR: Failed to generate "%IMPLIB%" with lib.exe.
    exit /b 1
  )
  set "MADE_IMPLIB=1"
  goto :implib_done
)

where llvm-lib.exe >nul 2>nul
if not errorlevel 1 (
  llvm-lib.exe /nologo /def:"%DEF%" /out:"%IMPLIB%" /machine:x64 >nul
  if errorlevel 1 (
    echo ERROR: Failed to generate "%IMPLIB%" with llvm-lib.exe.
    exit /b 1
  )
  set "MADE_IMPLIB=1"
  goto :implib_done
)

where dlltool.exe >nul 2>nul
if not errorlevel 1 (
  dlltool.exe -d "%DEF%" -l "%IMPLIB%" -D libcprime.dll -m i386:x86-64
  if errorlevel 1 (
    echo ERROR: Failed to generate "%IMPLIB%" with dlltool.exe.
    exit /b 1
  )
  set "MADE_IMPLIB=1"
  goto :implib_done
)

where x86_64-w64-mingw32-dlltool.exe >nul 2>nul
if not errorlevel 1 (
  x86_64-w64-mingw32-dlltool.exe -d "%DEF%" -l "%IMPLIB%" -D libcprime.dll -m i386:x86-64
  if errorlevel 1 (
    echo ERROR: Failed to generate "%IMPLIB%" with x86_64-w64-mingw32-dlltool.exe.
    exit /b 1
  )
  set "MADE_IMPLIB=1"
)

:implib_done
echo Deployed libcprime library files to "%DEPLOY%".
echo   bin\libcprime.dll
echo   include\cprime\libcprime.h
echo   lib\libcprime.def
if "%MADE_IMPLIB%"=="1" (
  echo   lib\libcprime.lib
) else (
  echo WARN: No lib.exe, llvm-lib.exe, or dlltool.exe found; skipped lib\libcprime.lib.
)
exit /b 0
