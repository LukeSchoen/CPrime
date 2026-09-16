@echo off
setlocal EnableExtensions EnableDelayedExpansion

rem CPrime Speed worker: run codex in the foreground, one work cycle at a time,
rem and drive down the time cpc.exe takes to compile.
rem
rem Scope: how long root cpc.exe takes to build itself and to compile C and C++
rem translation units, hardest case first: heavy headers, heavy template
rem instantiation, deep constant evaluation, macro-heavy preprocessing, large
rem translation units, deep call graphs and repeated declaration lookups.
rem
rem Codex decides what to change. This script only checks, measures, reports and
rem loops; Speed\task.md holds the goal, the measurement commands, the rules and
rem the current leads, and codex keeps it current as work completes.
rem
rem This loop is additive and never destructive. It creates build\worker\speed,
rem writes its own logs there, and never deletes, moves or renames any file. It
rem never creates the stop marker: only you do. A failed cycle leaves the tree
rem committed and usable, so the next cycle, or the next day, continues from
rem where it stopped instead of starting over.
rem
rem Run one worker at a time: all three worker folders share this working tree
rem and commit it at cycle boundaries, so two at once would fight over the same
rem files and the same git index. The first cycle commits whatever the tree
rem already holds, so start from a clean tree or expect that work to be
rem committed as "<Area> cycle 0".
rem
rem Optional environment overrides:
rem   DONE             stop marker file (default Speed\done.x)
rem   CODEX_EXE        codex executable (default codex.exe)
rem   MODEL            model name for codex (default: codex configuration)
rem   REASONING        model reasoning effort (default high)
rem   TASK_PROMPT      prompt handed to codex (default: continue Speed\task.md)
rem   MAX_CYCLES       stop after N cycles this session, 0 = unlimited (default 0)
rem   FAIL_EXIT_LIMIT  stop after N consecutive nonzero codex exits, 0 = keep going (default 5)
rem   FAIL_SLEEP       seconds to wait after a failed cycle (default 60)
rem   CHECK_ARGS       per-cycle correctness probe (default "-All -Tier fast")
rem   PERF_ARGS        per-cycle speed measurement (default below)
rem   NO_CHECK         set to 1 to skip the per-cycle correctness probe
rem   NO_MEASURE       set to 1 to skip the per-cycle speed measurement
rem   SKIP_COMMIT      set to 1 to leave the tree dirty instead of committing (default 0)
rem
rem Logs live in build\worker\speed: cycles.csv has one row per cycle,
rem cycle-NNNN.log is the full codex transcript, cycle-NNNN-result.txt its final
rem message, cycle-NNNN-check.log the probe output and perf-cycle-NNNN.tsv the
rem per-case compile times of that cycle.

set "AREA=speed"
set "TITLE=Speed"
set "GOAL=drive down the time cpc.exe takes to build itself and to compile C and C++ sources"

cd /d "%~dp0.."
set "ROOT=%CD%"

if not defined DONE set "DONE=%TITLE%\done.x"
if not defined CODEX_EXE set "CODEX_EXE=codex.exe"
if not defined REASONING set "REASONING=high"
if not defined TASK_PROMPT set "TASK_PROMPT=Read %TITLE%\task.md and continue its earliest unfinished work package, following AGENTS.md. Use only root cpc.exe, one compiler process at a time. Measure before and after with the commands %TITLE%\task.md names and keep the evidence under build\worker\speed. Finish by updating %TITLE%\task.md with the remaining work and the exact next action. Never create %TITLE%\done.x and never delete, move or rewrite %TITLE%\worker.cmd."
if not defined MAX_CYCLES set "MAX_CYCLES=0"
if not defined FAIL_EXIT_LIMIT set "FAIL_EXIT_LIMIT=5"
if not defined FAIL_SLEEP set "FAIL_SLEEP=60"
if not defined CHECK_ARGS set "CHECK_ARGS=-All -Tier fast"
if not defined PERF_ARGS set "PERF_ARGS=-CpcOnly -NoGate -Quiet"

set "LOG_DIR=%ROOT%\build\worker\%AREA%"
set "CYCLES=%LOG_DIR%\cycles.csv"
set "PERF_PREFIX=%LOG_DIR%\perf-cycle-"
set "SESSION=0"
set "FAILS=0"
set "RC=0"
set "CHECK_RC=-"
set "MEASURE_RC=-"
set "HEAD=-"
set "TAG=0000"

if not exist "%LOG_DIR%" mkdir "%LOG_DIR%" >nul 2>&1

git rev-parse --git-dir >nul 2>&1
if errorlevel 1 (
    echo [%DATE% %TIME%] not a git repository: %ROOT%
    exit /b 1
)

call "%CODEX_EXE%" --version >nul 2>&1
if errorlevel 1 (
    echo [%DATE% %TIME%] cannot run codex "%CODEX_EXE%"; install it, or set CODEX_EXE to it.
    echo [%DATE% %TIME%] nothing was changed.
    exit /b 1
)

for /f "delims=" %%H in ('git rev-parse --short HEAD 2^>nul') do set "HEAD=%%H"
call :read_last

echo [%DATE% %TIME%] ==========================================================
echo [%DATE% %TIME%] CPrime %TITLE% worker starting
echo [%DATE% %TIME%] goal        : %GOAL%
echo [%DATE% %TIME%] task file   : %TITLE%\task.md
echo [%DATE% %TIME%] directory   : %ROOT%
echo [%DATE% %TIME%] codex       : %CODEX_EXE% ^(effort %REASONING%^)
echo [%DATE% %TIME%] cycle logs  : %LOG_DIR%\cycle-NNNN.log
echo [%DATE% %TIME%] cycle rows  : %CYCLES%
echo [%DATE% %TIME%] stop marker : %DONE% ^(create this file to stop after the current cycle^)
echo [%DATE% %TIME%] pacing      : codex runs in the foreground; the next cycle starts when it exits
echo [%DATE% %TIME%] one at once : run this worker or another one, never two: they share this tree
if defined LASTROW echo [%DATE% %TIME%] last cycle  : !LASTROW!
call :show_measure
echo [%DATE% %TIME%] ==========================================================

:cycle
call :commit_work
if exist "%DONE%" (
    echo.
    echo [%DATE% %TIME%] "%DONE%" is present; stopping after cycle !CYCLE!.
    exit /b 0
)
if %MAX_CYCLES% gtr 0 if !SESSION! geq %MAX_CYCLES% (
    echo.
    echo [%DATE% %TIME%] session limit %MAX_CYCLES% reached after cycle !CYCLE!.
    exit /b 0
)
set /a SESSION+=1 >nul
set /a CYCLE+=1 >nul
set "TAG=0000!CYCLE!"
set "TAG=!TAG:~-4!"
set "CYCLE_LOG=%LOG_DIR%\cycle-!TAG!.log"
set "CYCLE_LAST=%LOG_DIR%\cycle-!TAG!-result.txt"
set "CHECK_LOG=%LOG_DIR%\cycle-!TAG!-check.log"
set "MEASURE_LOG=%LOG_DIR%\cycle-!TAG!-measure.log"
set "MEASURE_OUT=%PERF_PREFIX%!TAG!.tsv"
set "START_DATE=%DATE%"
set "START_TIME=%TIME%"

echo.
echo [%DATE% %TIME%] ---------- cycle !CYCLE! starting ----------
call :run_codex
if errorlevel 3 (
    echo [%DATE% %TIME%] ---------- stopping after cycle !CYCLE! ----------
    exit /b 1
)
call :check
call :measure
call :record

if %FAIL_EXIT_LIMIT% gtr 0 if !FAILS! geq %FAIL_EXIT_LIMIT% (
    echo [%DATE% %TIME%] codex exited nonzero !FAILS! times in a row; inspect !CYCLE_LOG!.
    echo [%DATE% %TIME%] ---------- stopping after cycle !CYCLE! ----------
    exit /b 1
)
if not "!RC!"=="0" (
    echo [%DATE% %TIME%] waiting %FAIL_SLEEP%s before the next cycle after a failed run.
    call :sleep %FAIL_SLEEP%
)
echo [%DATE% %TIME%] ---------- cycle !CYCLE! finished, looping ----------
goto cycle

:sleep
rem %1 = seconds.  `timeout` needs a console, so fall back to a quiet ping.
if "%~1"=="" exit /b 0
if "%~1"=="0" exit /b 0
timeout /t %~1 /nobreak >nul 2>&1
if errorlevel 1 ping -n %~1 127.0.0.1 >nul 2>&1
exit /b 0

:run_codex
set "RC=0"
set "MODEL_ARG="
if defined MODEL set "MODEL_ARG=--model "%MODEL%""
set "EFFORT_ARG="
if defined REASONING set "EFFORT_ARG=-c model_reasoning_effort="%REASONING%""
echo [%DATE% %TIME%] codex is working; transcript: %CYCLE_LOG%
call "%CODEX_EXE%" exec !MODEL_ARG! !EFFORT_ARG! -C "%ROOT%" --sandbox danger-full-access -c approval_policy="never" --color never -o "%CYCLE_LAST%" "%TASK_PROMPT%" > "%CYCLE_LOG%" 2>&1 <nul
set "RC=!ERRORLEVEL!"
if "!RC!"=="0" (
    set "FAILS=0"
    echo [%DATE% %TIME%] cycle !CYCLE! codex exit 0; final message: %CYCLE_LAST%
) else (
    set /a FAILS+=1 >nul
    echo [%DATE% %TIME%] cycle !CYCLE! codex exit !RC!; consecutive failure !FAILS!
    rem An account usage limit is not a broken cycle: stop instead of retrying into it.
    findstr /i /c:"usage limit" "%CYCLE_LOG%" >nul 2>&1
    if not errorlevel 1 (
        echo [%DATE% %TIME%] codex reports an account usage limit; this is not a failure of the tree.
        echo [%DATE% %TIME%] the working tree is committed and unchanged; restart this worker when the account allows.
        exit /b 3
    )
)
exit /b 0

:check
set "CHECK_RC=-"
if "%NO_CHECK%"=="1" exit /b 0
if not exist "%ROOT%\Tests\test.exe" (
    echo [%DATE% %TIME%] no Tests\test.exe; correctness probe skipped.
    exit /b 0
)
"%ROOT%\Tests\test.exe" %CHECK_ARGS% > "%CHECK_LOG%" 2>&1
set "CHECK_RC=!ERRORLEVEL!"
if "!CHECK_RC!"=="0" (
    echo [%DATE% %TIME%] probe ok   : Tests\test.exe %CHECK_ARGS%
) else (
    echo [%DATE% %TIME%] probe FAIL : Tests\test.exe %CHECK_ARGS% returned !CHECK_RC!; %CHECK_LOG%
    type "%CHECK_LOG%"
)
exit /b 0

:measure
set "MEASURE_RC=-"
if "%NO_MEASURE%"=="1" exit /b 0
if not exist "%ROOT%\cpc.exe" (
    echo [%DATE% %TIME%] no root cpc.exe; speed measurement skipped.
    exit /b 0
)
if not exist "%ROOT%\scripts\performance.exe" (
    echo [%DATE% %TIME%] no scripts\performance.exe; speed measurement skipped.
    exit /b 0
)
"%ROOT%\scripts\performance.exe" -Root "%ROOT%" %PERF_ARGS% -Results "%MEASURE_OUT%" > "%MEASURE_LOG%" 2>&1
set "MEASURE_RC=!ERRORLEVEL!"
if "!MEASURE_RC!"=="0" (
    echo [%DATE% %TIME%] measured     : %MEASURE_OUT%
) else (
    echo [%DATE% %TIME%] measure FAIL : exit !MEASURE_RC!; %MEASURE_LOG%
    type "%MEASURE_LOG%"
)
if exist "%MEASURE_OUT%" findstr /b /c:"c.self.driver" /c:"c.empty.main" "%MEASURE_OUT%"
exit /b 0

:record
set "ROW=!CYCLE!;%TITLE%;!START_DATE!;!START_TIME!;%DATE%;%TIME%;codex=!RC!;check=!CHECK_RC!;measure=!MEASURE_RC!;head=!HEAD!"
>>"%CYCLES%" echo !ROW!
echo [%DATE% %TIME%] cycle !CYCLE! recorded: codex=!RC! check=!CHECK_RC! measure=!MEASURE_RC! head=!HEAD!
exit /b 0

:read_last
set "CYCLE=0"
set "LASTROW="
if not exist "%CYCLES%" exit /b 0
for /f "usebackq tokens=1,* delims=;" %%A in ("%CYCLES%") do (
    set "CYCLE=%%A"
    set "LASTROW=%%A;%%B"
)
echo !CYCLE!|findstr /r "^[0-9][0-9]*$" >nul
if errorlevel 1 (
    set "CYCLE=0"
    set "LASTROW="
)
exit /b 0

:show_measure
set "NEWEST="
if not exist "%PERF_PREFIX%*.tsv" exit /b 0
for /f "delims=" %%F in ('dir /b /o-n "%PERF_PREFIX%*.tsv" 2^>nul') do if not defined NEWEST set "NEWEST=%%F"
if not defined NEWEST exit /b 0
echo [%DATE% %TIME%] last measured: %LOG_DIR%\!NEWEST!
findstr /b /c:"c.self.driver" /c:"c.empty.main" "%LOG_DIR%\!NEWEST!"
exit /b 0

:commit_work
if "%SKIP_COMMIT%"=="1" (
    echo [%DATE% %TIME%] SKIP_COMMIT=1: leaving the working tree alone.
    exit /b 0
)
git add -A
git diff --cached --quiet >nul 2>&1
if not errorlevel 1 (
    echo [%DATE% %TIME%] working tree already clean.
    exit /b 0
)
git commit -m "%TITLE% cycle !CYCLE! %DATE%" >nul 2>&1
if errorlevel 1 (
    echo [%DATE% %TIME%] commit failed; the working tree keeps the work as it is.
    exit /b 0
)
for /f "delims=" %%H in ('git rev-parse --short HEAD 2^>nul') do set "HEAD=%%H"
echo [%DATE% %TIME%] committed the work of cycle !CYCLE! as !HEAD!.
exit /b 0
