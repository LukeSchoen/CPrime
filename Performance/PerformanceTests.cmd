@echo off
setlocal EnableExtensions EnableDelayedExpansion

rem CPrime performance suite: cpc against tcc on the C cases in
rem Performance\cases, plus the leftovers/lookup checks in perf-check.
rem
rem The speed gate compares the machine independent ratios (cpc/tcc and
rem cpc/reference) with Performance\baseline\perf-baseline.tsv, so a result
rem recorded on one machine still means something on another.  The reference
rem build defaults to the cpc.exe committed at HEAD, which makes every run
rem answer "has today's compiler lost speed against the published one?".
rem
rem Options:
rem   -Fast             skip the heavy tier (the self-compile case)
rem   -NoGate           report only, never fail on a regression or a finding
rem   -UpdateBaseline   record the current numbers as the baseline
rem   -NoReference      do not extract HEAD's cpc.exe for comparison
rem   -Reference PATH   use PATH as the reference CPC build
rem   -Iterations N     measured runs per case (default 5)
rem   -Warmups N        discarded runs per case (default 1)
rem   -Tolerance PCT    allowed slowdown over the baseline (default 25)
rem   -SpeedOnly        skip the leftover and lookup checks
rem   -ChecksOnly       skip the speed comparison
rem   -Log FILE         append summary rows to a shared performance log
rem   -Cycle N          cycle number recorded in -Log rows
rem   -Head REV         commit hash recorded in -Log rows
rem   -Help             print this text
rem
rem Exit: 0 ok, 1 regression, new finding or compiler failure, 2 usage error.

cd /d "%~dp0.."
set "ROOT=%CD%"
set "OUT=%ROOT%\build\perf"
set "CPC=%ROOT%\cpc.exe"
set "RESULTS=%OUT%\perf-results.tsv"
set "EXTRAS="
set "CHECK_ARGS="
set "COMPARE_ARGS="
set "REFERENCE="
set "NO_REFERENCE=0"
set "SKIP_CHECKS=0"
set "SKIP_SPEED=0"
set "RC=0"

:parse
if "%~1"=="" goto parsed
if /i "%~1"=="-Fast" (set "EXTRAS=%EXTRAS% -Fast" & shift & goto parse)
if /i "%~1"=="-NoGate" (set "EXTRAS=%EXTRAS% -NoGate" & set "CHECK_ARGS=%CHECK_ARGS% -NoGate" & shift & goto parse)
if /i "%~1"=="-UpdateBaseline" (set "EXTRAS=%EXTRAS% -UpdateBaseline" & set "CHECK_ARGS=%CHECK_ARGS% -UpdateBaseline" & shift & goto parse)
if /i "%~1"=="-NoReference" (set "NO_REFERENCE=1" & shift & goto parse)
if /i "%~1"=="-Reference" (set "REFERENCE=%~2" & shift & shift & goto parse)
if /i "%~1"=="-Iterations" (set "EXTRAS=%EXTRAS% -Iterations %~2" & shift & shift & goto parse)
if /i "%~1"=="-Warmups" (set "EXTRAS=%EXTRAS% -Warmups %~2" & shift & shift & goto parse)
if /i "%~1"=="-Tolerance" (set "EXTRAS=%EXTRAS% -Tolerance %~2" & shift & shift & goto parse)
if /i "%~1"=="-NoiseLimit" (set "EXTRAS=%EXTRAS% -NoiseLimit %~2" & shift & shift & goto parse)
if /i "%~1"=="-SpeedOnly" (set "SKIP_CHECKS=1" & shift & goto parse)
if /i "%~1"=="-ChecksOnly" (set "SKIP_SPEED=1" & shift & goto parse)
if /i "%~1"=="-Log" (set "EXTRAS=%EXTRAS% -Log %~2" & set "CHECK_ARGS=%CHECK_ARGS% -Log %~2" & shift & shift & goto parse)
if /i "%~1"=="-Cycle" (set "EXTRAS=%EXTRAS% -Cycle %~2" & set "CHECK_ARGS=%CHECK_ARGS% -Cycle %~2" & shift & shift & goto parse)
if /i "%~1"=="-Head" (set "EXTRAS=%EXTRAS% -Head %~2" & set "CHECK_ARGS=%CHECK_ARGS% -Head %~2" & shift & shift & goto parse)
if /i "%~1"=="-Help" goto usage
echo performance tests: unknown option "%~1"
goto usage

:usage
echo usage: Performance\PerformanceTests.cmd [options]
echo   -Fast -NoGate -UpdateBaseline -NoReference -Reference PATH
echo   -Iterations N -Warmups N -Tolerance PCT -SpeedOnly -ChecksOnly
echo   -NoiseLimit PCT
echo   -Log FILE -Cycle N -Head REV
exit /b 2

:parsed
if not exist "%CPC%" (
    echo [perf] missing root cpc.exe; nothing to measure.
    exit /b 2
)
if not exist "%OUT%" mkdir "%OUT%"

rem Tools are first-party C programs built with the compiler under test, so a
rem broken cpc leaves the previous executables in place and the suite reports
rem what it measured with them.
call :build_tool "%CPC%" "%OUT%\perf-check.exe" "%ROOT%\Performance\src\perf_check.c"
call :build_tool "%CPC%" "%OUT%\perf-compare.exe" "%ROOT%\Performance\src\perf_compare.c"
if not exist "%OUT%\perf-compare.exe" (
    echo [perf] no perf-compare.exe available.
    exit /b 2
)

if "%NO_REFERENCE%"=="1" goto have_reference
if defined REFERENCE goto have_reference
git rev-parse --git-dir >nul 2>&1
if errorlevel 1 goto have_reference
git cat-file -e HEAD:cpc.exe >nul 2>&1
if errorlevel 1 goto have_reference
git show HEAD:cpc.exe > "%OUT%\cpc-reference.exe"
if errorlevel 1 goto have_reference
if not exist "%OUT%\cpc-reference.exe" goto have_reference
set "REFERENCE=%OUT%\cpc-reference.exe"
:have_reference
set REFARG=
if defined REFERENCE set REFARG=-Reference "%REFERENCE%"

if "%SKIP_CHECKS%"=="1" goto run_speed
echo.
echo [perf] leftovers, environment and loader lookups
"%OUT%\perf-check.exe" -Root "%ROOT%" %CHECK_ARGS%
if errorlevel 1 set "RC=1"

:run_speed
if "%SKIP_SPEED%"=="1" goto done
echo.
echo [perf] C compilation speed
"%OUT%\perf-compare.exe" -Root "%ROOT%" %REFARG% %EXTRAS%
set "SPEED_RC=%ERRORLEVEL%"
if "%SPEED_RC%"=="3" (
    echo [perf] the machine was busy during the run; these numbers are not usable.
    if "%RC%"=="0" set "RC=3"
)
if "%SPEED_RC%"=="1" set "RC=1"

:done
if "%RC%"=="3" (
    echo [perf] suite could not measure: the machine was busy. Re-run it alone.
    exit /b 3
)
if not "%RC%"=="0" (
    echo [perf] suite failed.
    exit /b 1
)
echo [perf] suite passed.
exit /b 0

:build_tool
rem %1 compiler, %2 executable, %3 source
rem Build beside the target and replace it only on success, so a compiler that
rem cannot compile the tool leaves the previous executable usable.
"%~1" -o "%~2.new" "%~3" >nul 2>&1
if errorlevel 1 goto tool_failed
if not exist "%~2.new" goto tool_failed
move /y "%~2.new" "%~2" >nul
exit /b 0

:tool_failed
if exist "%~2.new" del "%~2.new" >nul 2>&1
if exist "%~2" (
    echo [perf] rebuild of %~nx2 failed; using the existing executable.
) else (
    echo [perf] cannot build %~nx2 with the compiler under test.
)
exit /b 0
