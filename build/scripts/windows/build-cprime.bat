@rem ------------------------------------------------------
@rem batch file to build cprime using mingw, msvc or cprime itself
@rem ------------------------------------------------------

@echo off
setlocal
if (%1)==(-clean) goto :cleanup
set CC=gcc
set ROOT=..\..\..
set CPRIME_DRIVER=%ROOT%\src\compiler\driver\cprime.c
set LIBCPRIME_SRC=%ROOT%\src\compiler\middleend\libcprime.c
set INC_FLAGS=-I%ROOT%\include\runtime -I%ROOT%\include\cprime -I%ROOT%\third-party\win32-sdk\include -I%ROOT%\third-party\win32-sdk\include\winapi -I%ROOT%\src\compiler\frontend -I%ROOT%\src\compiler\middleend -I%ROOT%\src\compiler\backend\x64 -I%ROOT%
if exist %ROOT%\VERSION (
set /p VERSION= < %ROOT%\VERSION
) else (
set VERSION=0.9.28
)
set CPRIMEDIR=
set BINDIR=
set DOC=no
set XCC=no
goto :a0
:a2
shift
:a3
shift
:a0
if not (%1)==(-c) goto :a1
set CC=%~2
if (%2)==(cl) set CC=@call :cl
goto :a2
:a1
if (%1)==(-t) set T=%2&& goto :a2
if (%1)==(-v) set VERSION=%~2&& goto :a2
if (%1)==(-i) set CPRIMEDIR=%2&& goto :a2
if (%1)==(-b) set BINDIR=%2&& goto :a2
if (%1)==(-d) set DOC=yes&& goto :a3
if (%1)==(-x) set XCC=yes&& goto :a3
if (%1)==() goto :p1
:usage
echo usage: build-cprime.bat [ options ... ]
echo options:
echo   -c prog              use prog (gcc/cprime/cl) to compile cprime
echo   -c "prog options"    use prog with options to compile cprime
echo   -v "version"         set cprime version
echo   -i cprimedir            install cprime into cprimedir
echo   -b bindir            but install cpc.exe and libcprime.dll into bindir
echo   -d                   create cprime-doc.html too (needs makeinfo)
echo   -x                   build the cross compiler too
echo   -clean               delete all previously produced files and directories
exit /B 1

@rem ------------------------------------------------------
@rem sub-routines

:cleanup
set LOG=echo
%LOG% removing files:
for %%f in (*cpc.exe libcprime.dll lib\*.a) do call :del_file %%f
for %%f in (%ROOT%\config.texi) do call :del_file %%f
for %%f in (include\*.h) do @if exist %ROOT%\%%f call :del_file %%f
for %%f in (include\cprimelib.h examples\libcprime_test.c) do call :del_file %%f
for %%f in (lib\*.o *.o *.obj *.def *.pdb *.lib *.exp *.ilk) do call :del_file %%f
%LOG% removing directories:
for %%f in (doc libcprime) do call :del_dir %%f
%LOG% done.
exit /B 0
:del_file
if exist %1 del %1 && %LOG%   %1
exit /B 0
:del_dir
if exist %1 rmdir /Q/S %1 && %LOG%   %1
exit /B 0

:cl
@echo off
set CMD=cl
:c0
set ARG=%1
set ARG=%ARG:.dll=.lib%
if (%1)==(-shared) set ARG=-LD
if (%1)==(-o) shift && set ARG=-Fe%2
set CMD=%CMD% %ARG%
shift
if not (%1)==() goto :c0
%CMD% -O2 -W2 -Zi -MT -GS- -nologo %DEF_GITHASH% -link -opt:ref,icf
@exit /B %ERRORLEVEL%

@rem ------------------------------------------------------
@rem main program

:p1
if defined T goto :p2
set T=64
:p2
set T=%T: =%
if "%CC:~-3%"=="gcc" set CC=%CC% -O2 -s -static
set LIBCPRIME_LINK=libcprime.dll
set CLANG_MSVC=no
echo %CC% | findstr /I "clang" >nul
if not errorlevel 1 (
for /f "tokens=* delims=" %%b in ('%CC% --version 2^>nul ^| findstr /I "Target:.*windows-msvc"') do set CLANG_MSVC=yes
)
if "%CLANG_MSVC%"=="yes" (
call :ensure_msvc_env
if errorlevel 1 goto :the_end
set CC=%CC% -fno-builtin -Dopen=_open -Dread=_read -Dclose=_close -Dlseek=_lseek -Dunlink=_unlink -Dfdopen=_fdopen -Dgetcwd=_getcwd -Dstricmp=_stricmp -Dstrlwr=_strlwr
set LIBCPRIME_LINK=libcprime.lib
set CPRIME_C=%CPRIME_DRIVER%
 set EXTRA_BOOT_SRCS=
)
set CC=%CC% %INC_FLAGS%
if _%CPRIME_C%_==__ (
echo %CC% | findstr /I "cpc.exe" >nul
if not errorlevel 1 (
set CPRIME_C=%CPRIME_DRIVER%
for /f "tokens=1" %%i in ("%CC%") do set SELF_CPRIME_EXE=%%i
set CC=%CC% -B%ROOT%
set SELF_CPRIME=yes
)
)
if (%BINDIR%)==() set BINDIR=%CPRIMEDIR%

set D64=-DCPRIME_TARGET_PE -DCPRIME_TARGET_X86_64
set P64=x86_64-win32
set D=%D64%
set P=%P64%
set DX=%D64%
set PX=%P64%
set TX=64
goto :p3

:p3
git.exe --version >nul 2>nul
if not %ERRORLEVEL%==0 goto :git_done
for /f %%b in ('git.exe rev-parse --abbrev-ref HEAD') do set GITHASH=%%b
for /f %%b in ('git.exe log -1 "--pretty=format:%%cs_%GITHASH%@%%h"') do set GITHASH=%%b
git.exe diff --quiet
if %ERRORLEVEL%==1 set GITHASH=%GITHASH%*
:git_done
@echo off

@rem %CC% -DC2STR ..\conftest.c -o c2str.exe
@rem .\c2str.exe ../include/cprimedefs.h ../cprimedefs_.h

for %%f in (*cpc.exe *cprime.dll) do @del %%f

if _%SELF_CPRIME%_==_yes_ call :bootstrap_libcprime1 || goto :the_end

@if _%CPRIME_C%_==__ goto compiler_2parts
@rem if CPRIME_C was defined then build only cpc.exe
%CC% -o cpc.exe %CPRIME_C% %EXTRA_BOOT_SRCS% %D% %EXTRA_LINK_LIBS%
@if errorlevel 1 goto :the_end
@goto :compiler_done

:compiler_2parts
@if _%LIBCPRIME_C%_==__ set LIBCPRIME_C=%LIBCPRIME_SRC%
%CC% -o libcprime.dll -shared %LIBCPRIME_C% %D% -DLIBCPRIME_AS_DLL %EXTRA_LINK_LIBS%
@if errorlevel 1 goto :the_end
%CC% -o cpc.exe %CPRIME_DRIVER% %LIBCPRIME_LINK% %D% -DONE_SOURCE"=0" %EXTRA_LINK_LIBS%
@if errorlevel 1 goto :the_end
if not _%XCC%_==_yes_ goto :compiler_done
%CC% -o %PX%-cpc.exe %CPRIME_DRIVER% %DX%
@if errorlevel 1 goto :the_end
:compiler_done
@if (%EXES_ONLY%)==(yes) goto :files_done

if not exist libcprime mkdir libcprime
if not exist doc mkdir doc
if not exist include mkdir include
if not exist examples mkdir examples
if not exist lib mkdir lib
copy>nul %ROOT%\include\runtime\*.h include
copy>nul %ROOT%\include\cprime\cprimelib.h include
copy>nul %ROOT%\include\cprime\libcprime.h libcprime
copy>nul %ROOT%\tests\libcprime_test.c examples
copy>nul %ROOT%\win32\cprime-win32.txt doc

if _%CPRIME_C%_==__ (
if exist libcprime.dll .\cpc -impdef libcprime.dll -o libcprime\libcprime.def
@if errorlevel 1 goto :the_end
)

:lib
call :make_lib %T% || goto :the_end
@if exist %PX%-cpc.exe call :make_lib %TX% %PX%- || goto :the_end

:cprime-doc.html
@if not (%DOC%)==(yes) goto :doc-done
echo>%ROOT%\config.texi @set VERSION %VERSION%
cmd /c makeinfo --html --no-split %ROOT%\cprime-doc.texi -o doc/cprime-doc.html
:doc-done

:files_done
for %%f in (*.o *.def) do @del %%f

:copy-install
@if (%CPRIMEDIR%)==() goto :the_end
if not exist %BINDIR% mkdir %BINDIR%
for %%f in (*cpc.exe *cprime.dll) do @copy>nul %%f %BINDIR%\%%f
if not exist %CPRIMEDIR% mkdir %CPRIMEDIR%
@if not exist %CPRIMEDIR%\lib mkdir %CPRIMEDIR%\lib
for %%f in (lib\*.a lib\*.o lib\*.def) do @copy>nul %%f %CPRIMEDIR%\%%f
for %%f in (include examples libcprime doc) do @xcopy>nul /s/i/q/y %%f %CPRIMEDIR%\%%f

:the_end
exit /B %ERRORLEVEL%

:ensure_msvc_env
if defined LIB exit /B 0
set VSWHERE=
if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" set VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe
if exist "%ProgramFiles%\Microsoft Visual Studio\Installer\vswhere.exe" set VSWHERE=%ProgramFiles%\Microsoft Visual Studio\Installer\vswhere.exe
if defined VSWHERE (
for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set VSROOT=%%i
if defined VSROOT if exist "%VSROOT%\VC\Auxiliary\Build\vcvars64.bat" (
call "%VSROOT%\VC\Auxiliary\Build\vcvars64.bat" >nul
if defined LIB exit /B 0
)
)
for %%p in (
"%ProgramFiles%\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
"%ProgramFiles%\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
"%ProgramFiles%\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat"
"%ProgramFiles%\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat"
"%ProgramFiles(x86)%\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
"%ProgramFiles(x86)%\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
"%ProgramFiles(x86)%\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat"
"%ProgramFiles(x86)%\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat"
) do (
if exist %%~p (
call %%~p >nul
if defined LIB exit /B 0
)
)
echo Error: unable to locate MSVC linker environment for clang-msvc mode.
echo Hint: run from a Visual Studio Developer Command Prompt, or install VS Build Tools C++ workload.
exit /B 1

:bootstrap_libcprime1
if not exist %ROOT%\lib mkdir %ROOT%\lib
if exist %ROOT%\lib\libcprime1.a exit /B 0
%CC% -m%T% -c %ROOT%\src\runtime\generic\libcprime1.c
if errorlevel 1 exit /B 1
%CC% -m%T% -c %ROOT%\src\runtime\windows\crt1.c
if errorlevel 1 exit /B 1
%CC% -m%T% -c %ROOT%\src\runtime\windows\crt1w.c
if errorlevel 1 exit /B 1
%CC% -m%T% -c %ROOT%\src\runtime\windows\wincrt1.c
if errorlevel 1 exit /B 1
%CC% -m%T% -c %ROOT%\src\runtime\windows\wincrt1w.c
if errorlevel 1 exit /B 1
%CC% -m%T% -c %ROOT%\src\runtime\windows\dllcrt1.c
if errorlevel 1 exit /B 1
%CC% -m%T% -c %ROOT%\src\runtime\windows\dllmain.c
if errorlevel 1 exit /B 1
%CC% -m%T% -c %ROOT%\src\runtime\generic\stdatomic.c
if errorlevel 1 exit /B 1
%CC% -m%T% -c %ROOT%\src\runtime\generic\builtin.c
if errorlevel 1 exit /B 1
%SELF_CPRIME_EXE% -B%ROOT% -ar rcs %ROOT%\lib\libcprime1.a libcprime1.o crt1.o crt1w.o wincrt1.o wincrt1w.o dllcrt1.o dllmain.o stdatomic.o builtin.o
exit /B %ERRORLEVEL%

:make_lib
if not exist lib mkdir lib
.\cpc -B. %INC_FLAGS% -m%1 -c %ROOT%\src\runtime\generic\libcprime1.c
.\cpc -B. %INC_FLAGS% -m%1 -c %ROOT%\src\runtime\windows\crt1.c
.\cpc -B. %INC_FLAGS% -m%1 -c %ROOT%\src\runtime\windows\crt1w.c
.\cpc -B. %INC_FLAGS% -m%1 -c %ROOT%\src\runtime\windows\wincrt1.c
.\cpc -B. %INC_FLAGS% -m%1 -c %ROOT%\src\runtime\windows\wincrt1w.c
.\cpc -B. %INC_FLAGS% -m%1 -c %ROOT%\src\runtime\windows\dllcrt1.c
.\cpc -B. %INC_FLAGS% -m%1 -c %ROOT%\src\runtime\windows\dllmain.c
.\cpc -B. %INC_FLAGS% -m%1 -c %ROOT%\src\runtime\generic\stdatomic.c
.\cpc -B. %INC_FLAGS% -m%1 -c %ROOT%\src\runtime\generic\builtin.c
.\cpc -ar rcs lib/%2libcprime1.a libcprime1.o crt1.o crt1w.o wincrt1.o wincrt1w.o dllcrt1.o dllmain.o stdatomic.o builtin.o
.\cpc -B. %INC_FLAGS% -m%1 -c %ROOT%\src\runtime\generic\bcheck.c -o lib/%2bcheck.o -bt -I%ROOT%
.\cpc -B. %INC_FLAGS% -m%1 -c %ROOT%\src\runtime\generic\bt-exe.c -o lib/%2bt-exe.o
.\cpc -B. %INC_FLAGS% -m%1 -c %ROOT%\src\runtime\generic\bt-log.c -o lib/%2bt-log.o
.\cpc -B. %INC_FLAGS% -m%1 -c %ROOT%\src\runtime\generic\bt-dll.c -o lib/%2bt-dll.o
.\cpc -B. %INC_FLAGS% -m%1 -c %ROOT%\src\runtime\generic\runmain.c -o lib/%2runmain.o
exit /B %ERRORLEVEL%
