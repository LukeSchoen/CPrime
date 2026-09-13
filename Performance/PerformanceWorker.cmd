@echo off
setlocal EnableExtensions EnableDelayedExpansion

rem CPrime performance worker: run codex in the foreground, one work cycle at a
rem time, and drive down the time cpc takes to compile C.
rem
rem Every cycle is bracketed by measurement, not by opinion.  Before codex runs,
rem the worker prints the current leftovers/lookup checks and the cpc against
rem tcc and reference numbers.  After codex exits it measures again and appends
rem the sample to a machine-readable log (Performance\progress\log.tsv) that
rem git commits together with the work it describes, so a restart needs no
rem recovery step.
rem
rem The speed numbers are ratios (cpc/tcc, cpc/reference), not milliseconds, so
rem a busy machine cannot fake a win: the reference compiler and tcc absorb the
rem machine's speed.  The leftover checks fail the cycle when a change adds an
rem environment, loader or registry lookup, a TODO/FIXME/HACK/XXX marker, an
rem "#if 0" block or a scratch file where there was none.
rem
rem The prompt handed to codex is Performance\task.md: the goal, the measurement
rem commands, the constraints and the current leads live there.  Codex decides
rem what to change; this script only measures, reports and loops.
rem
rem Run one worker per working tree: this loop rebuilds root cpc.exe and commits
rem the tree, so it must not share a tree with worker.cmd or another session.
rem
rem Optional environment overrides:
rem   DONE             stop marker file (default Performance\done.x)
rem   CODEX_EXE        codex executable (default codex.exe)
rem   TASK_PROMPT      prompt passed to codex exec (default "implement Performance\task.md")
rem   MAX_CYCLES       stop after N cycles in this session, 0 = unlimited (default 0)
rem   FAIL_EXIT_LIMIT  stop after N consecutive nonzero codex exits (default 3)
rem   PERF_LOG         performance log (default Performance\progress\log.tsv)
rem   PERF_ARGS        extra PerformanceTests.cmd arguments (default "-Fast")
rem   CYCLE_LOG        per-cycle codex log (default build\perf\worker-cycle.log)
rem   SKIP_COMMIT      measure only, never commit (default 0; for testing the
rem                    loop beside manual edits, not for normal work)
cd /d "%~dp0.."

if not defined DONE set "DONE=Performance\done.x"
if not defined CODEX_EXE set "CODEX_EXE=codex.exe"
if not defined TASK_PROMPT set "TASK_PROMPT=implement Performance\task.md"
if not defined MAX_CYCLES set "MAX_CYCLES=0"
if not defined FAIL_EXIT_LIMIT set "FAIL_EXIT_LIMIT=3"
if not defined PERF_LOG set "PERF_LOG=Performance\progress\log.tsv"
if not defined PERF_ARGS set "PERF_ARGS=-Fast"
if not defined CYCLE_LOG set "CYCLE_LOG=build\perf\worker-cycle.log"
set "ROOT=%CD%"
set "TOOLS=%ROOT%\build\perf"
set "SESSION=0"
set "FAILS=0"

if not exist "%TOOLS%" mkdir "%TOOLS%"
git rev-parse --git-dir >nul 2>&1
if errorlevel 1 (
    echo [%DATE% %TIME%] not a git repository: %CD%
    exit /b 1
)

call :build_tools
call :read_cycle

echo [%DATE% %TIME%] ==========================================================
echo [%DATE% %TIME%] CPrime performance worker starting
echo [%DATE% %TIME%] directory    : %CD%
echo [%DATE% %TIME%] task prompt  : %TASK_PROMPT%
echo [%DATE% %TIME%] suite args   : %PERF_ARGS%
echo [%DATE% %TIME%] cycle log    : %CYCLE_LOG%
echo [%DATE% %TIME%] performance  : %PERF_LOG%
echo [%DATE% %TIME%] stop marker  : %DONE% ^(create this file to stop after the current cycle^)
echo [%DATE% %TIME%] pacing       : next cycle starts when codex exits; no fixed wait
call :report
echo [%DATE% %TIME%] ==========================================================

:cycle
call :commit_work
if exist "%DONE%" (
    echo [%DATE% %TIME%] !DONE! present, stopping after !CYCLE! cycle^(s^).
    exit /b 0
)
set /a SESSION+=1 >nul
set /a CYCLE+=1 >nul
if %MAX_CYCLES% gtr 0 if !SESSION! gtr %MAX_CYCLES% (
    call :report
    echo [%DATE% %TIME%] session limit %MAX_CYCLES% reached, stopping after !CYCLE! cycle^(s^).
    exit /b 0
)
echo.
echo [%DATE% %TIME%] ---------- cycle !CYCLE! starting ----------
call :run_codex
call :measure
if !FAILS! geq %FAIL_EXIT_LIMIT% (
    echo [%DATE% %TIME%] codex exited nonzero !FAILS! times in a row; inspect %CYCLE_LOG% and stop.
    echo [%DATE% %TIME%] ---------- stopping after !CYCLE! cycle^(s^) ----------
    exit /b 1
)
echo [%DATE% %TIME%] ---------- cycle !CYCLE! finished, looping ----------
goto cycle

:build_tools
if not exist "%ROOT%\cpc.exe" (
    echo [%DATE% %TIME%] missing root cpc.exe; performance tools not built.
    exit /b 0
)
call "%ROOT%\Performance\PerformanceTests.cmd" %PERF_ARGS% -ChecksOnly -NoGate >nul 2>&1
exit /b 0

:read_cycle
set "CYCLE=0"
if not exist "%TOOLS%\perf-compare.exe" exit /b 0
rem `call` keeps the quoted tool path intact inside the for /f command.
for /f "delims=" %%N in ('call "%TOOLS%\perf-compare.exe" -Root "%ROOT%" -Log "%PERF_LOG%" -LastCycle 2^>nul') do set "CYCLE=%%N"
if not defined CYCLE set "CYCLE=0"
exit /b 0

:report
echo [%DATE% %TIME%] current state:
call :show_history
call "%ROOT%\Performance\PerformanceTests.cmd" %PERF_ARGS% -NoGate
exit /b 0

:measure
rem %1 = append; the sample describes the cycle that just finished.
set "HEAD=-"
for /f "delims=" %%H in ('git rev-parse --short HEAD 2^>nul') do set "HEAD=%%H"
call "%ROOT%\Performance\PerformanceTests.cmd" %PERF_ARGS% -NoGate -Log "%PERF_LOG%" -Cycle !CYCLE! -Head "!HEAD!"
set "MRC=!ERRORLEVEL!"
rem Exit 3 is a disturbed machine (something else was building), which the
rem suite refuses to record; anything else nonzero is a real finding, and its
rem sample is already in the log.
if "!MRC!"=="3" (
    echo [%DATE% %TIME%] machine was busy; no sample logged, the next cycle measures again.
) else (
    if not "!MRC!"=="0" echo [%DATE% %TIME%] measurement finished with findings; the sample is logged.
)
echo [%DATE% %TIME%] last samples:
call :show_history
exit /b 0

:show_history
if not exist "%PERF_LOG%" (
    echo [%DATE% %TIME%] no samples recorded yet.
    exit /b 0
)
if not exist "%TOOLS%\perf-compare.exe" (
    echo [%DATE% %TIME%] %PERF_LOG% exists; build the tools to summarise it.
    exit /b 0
)
call "%TOOLS%\perf-compare.exe" -Root "%ROOT%" -Log "%PERF_LOG%" -History 4
exit /b 0

:commit_work
if "%SKIP_COMMIT%"=="1" (
    echo [%DATE% %TIME%] SKIP_COMMIT=1: leaving the working tree alone.
    exit /b 0
)
git add -A
git diff --cached --quiet >nul 2>&1
if errorlevel 1 (
    git commit -m "performance cycle !CYCLE! %DATE%" >nul 2>&1
    if errorlevel 1 (
        echo [%DATE% %TIME%] commit failed; leaving the working tree as it is.
    ) else (
        for /f "delims=" %%H in ('git rev-parse --short HEAD 2^>nul') do set "HEAD=%%H"
        echo [%DATE% %TIME%] committed work as !HEAD!.
    )
) else (
    echo [%DATE% %TIME%] nothing to commit ^(working tree clean^).
)
exit /b 0

:run_codex
echo [%DATE% %TIME%] running codex: %CODEX_EXE% exec "%TASK_PROMPT%"
rem `call` keeps control flow intact when CODEX_EXE is a wrapper script
rem (.cmd/.bat); it is harmless for codex.exe itself.
call "%CODEX_EXE%" exec "%TASK_PROMPT%" > "%CYCLE_LOG%" 2>&1
set "RC=!ERRORLEVEL!"
if "!RC!"=="0" (
    set "FAILS=0"
) else (
    set /a FAILS+=1 >nul
    echo [%DATE% %TIME%] codex exited with !RC!; consecutive failure !FAILS! of %FAIL_EXIT_LIMIT%.
)
exit /b 0
