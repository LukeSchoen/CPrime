@echo off
setlocal EnableExtensions EnableDelayedExpansion

rem CPrime Compatibility worker: run codex in the foreground, one work cycle at
rem a time, and drive cpc.exe towards endlessly correct C++17.
rem
rem Scope: accept every valid C++17 program and reject invalid ones with the
rem right diagnostic, so real projects compile unmodified. Work comes from the
rem retained internal cases, from compiler and language test suites elsewhere
rem (clang, gcc, msvc) reduced to local cases, and from large C++ projects used
rem as compatibility probes.
rem
rem Codex decides what to change. This script only checks, reports and loops;
rem Compatibility\task.md holds the goal, the evidence sources, the rules and the
rem current leads, and codex keeps it current as work completes.
rem
rem This loop is additive and never destructive. It creates
rem Compatibility\build, writes its own logs there, and never deletes,
rem moves or renames any file. It never creates the stop marker: only you do. A
rem failed cycle leaves the tree committed and usable, so the next cycle, or the
rem next day, continues from where it stopped instead of starting over.
rem
rem Run one worker at a time: all three worker folders share this working tree
rem and commit it at cycle boundaries, so two at once would fight over the same
rem files and the same git index. The first cycle commits whatever the tree
rem already holds, so start from a clean tree or expect that work to be
rem committed as "<Area> cycle 0".
rem
rem Optional environment overrides:
rem   DONE             stop marker file (default Compatibility\done.x)
rem   CODEX_EXE        codex executable (default codex.exe)
rem   MODEL            model name for codex (default: codex configuration)
rem   REASONING        model reasoning effort (default high)
rem   TASK_PROMPT      prompt handed to codex (default: continue Compatibility\task.md)
rem   MAX_CYCLES       stop after N cycles this session, 0 = unlimited (default 0)
rem   FAIL_EXIT_LIMIT  stop after N consecutive nonzero codex exits, 0 = keep going (default 5)
rem   FAIL_SLEEP       seconds to wait after a failed cycle (default 60)
rem   CHECK_ARGS       per-cycle correctness probe (default "-Regression")
rem   NO_CHECK         set to 1 to skip the per-cycle correctness probe
rem   SKIP_COMMIT      set to 1 to leave the tree dirty instead of committing (default 0)
rem
rem Logs live in Compatibility\build: cycles.csv has one row per cycle,
rem cycle-NNNN.log is the full codex transcript, cycle-NNNN-result.txt its final
rem message and cycle-NNNN-check.log the probe output.

set "AREA=compatibility"
set "TITLE=Compatibility"
set "GOAL=make cpc.exe endlessly correct for C++17, measured by retained cases and real projects"

cd /d "%~dp0.."
set "ROOT=%CD%"

if not defined DONE set "DONE=%TITLE%\done.x"
if not defined CODEX_EXE set "CODEX_EXE=codex.exe"
if not defined REASONING set "REASONING=high"
if not defined TASK_PROMPT set "TASK_PROMPT=Read %TITLE%\task.md and continue its earliest unfinished work package, following AGENTS.md. Use only root cpc.exe, one compiler process at a time. Reproduce each gap with the exact case before repairing it, keep one minimal retained case, and keep the evidence under Compatibility\build. Finish by updating %TITLE%\task.md with the remaining work and the exact next action. Never create %TITLE%\done.x, never delete or weaken a retained case, and never delete, move or rewrite %TITLE%\worker.cmd."
if not defined MAX_CYCLES set "MAX_CYCLES=0"
if not defined FAIL_EXIT_LIMIT set "FAIL_EXIT_LIMIT=5"
if not defined FAIL_SLEEP set "FAIL_SLEEP=60"
if not defined CHECK_ARGS set "CHECK_ARGS=-Regression"

set "LOG_DIR=%ROOT%\%TITLE%\build"
set "CYCLES=%LOG_DIR%\cycles.csv"
set "CHECK_EXE=%ROOT%\%TITLE%\tests\test.exe"
if not defined GATE set "GATE=0"
set "SESSION=0"
set "FAILS=0"
set "RC=0"
set "CHECK_RC=-"
set "MEASURE_RC=skipped"
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
echo [%DATE% %TIME%] probe       : %CHECK_EXE% %CHECK_ARGS%
echo [%DATE% %TIME%] stop marker : %DONE% ^(create this file to stop after the current cycle^)
echo [%DATE% %TIME%] pacing      : codex runs in the foreground; the next cycle starts when it exits
echo [%DATE% %TIME%] one at once : run this worker or another one, never two: they share this tree
if defined LASTROW echo [%DATE% %TIME%] last cycle  : !LASTROW!
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
if not exist "%CHECK_EXE%" (
    echo [%DATE% %TIME%] no %CHECK_EXE%; correctness probe skipped.
    exit /b 0
)
"%CHECK_EXE%" %CHECK_ARGS% > "%CHECK_LOG%" 2>&1
set "CHECK_RC=!ERRORLEVEL!"
if "!CHECK_RC!"=="0" (
    echo [%DATE% %TIME%] probe ok   : %CHECK_ARGS% [%CHECK_EXE%]
) else (
    echo [%DATE% %TIME%] probe FAIL : %CHECK_ARGS% exit !CHECK_RC!; %CHECK_LOG%
    type "%CHECK_LOG%"
)
exit /b 0

:record
set "ROW=!CYCLE!;%TITLE%;!START_DATE!;!START_TIME!;%DATE%;%TIME%;codex=!RC!;check=!CHECK_RC!;head=!HEAD!"
>>"%CYCLES%" echo !ROW!
echo [%DATE% %TIME%] cycle !CYCLE! recorded: codex=!RC! check=!CHECK_RC! head=!HEAD!
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
