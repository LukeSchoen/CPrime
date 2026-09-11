@echo off
setlocal EnableExtensions EnableDelayedExpansion

rem Run from the repository root that contains this script.
rem
rem Cycle pacing is work driven: codex runs in the foreground and the next
rem cycle starts as soon as it exits. There is no fixed sleep, and a running
rem agent is never killed. The 45-minute budget is enforced by task.md.
rem Run one worker per working tree; each cycle waits for its own codex.
rem
rem Optional environment overrides:
rem   DONE             stop marker file (default done.x)
rem   CODEX_EXE        codex executable (default codex.exe)
rem   TASK_PROMPT      prompt passed to codex exec (default "implement task.md")
rem   MAX_CYCLES       stop after N cycles, 0 = unlimited (default 0)
rem   FAIL_EXIT_LIMIT  stop after N consecutive nonzero codex exits (default 3)
cd /d "%~dp0"

if not defined DONE set "DONE=done.x"
if not defined CODEX_EXE set "CODEX_EXE=codex.exe"
if not defined TASK_PROMPT set "TASK_PROMPT=implement task.md"
if not defined MAX_CYCLES set "MAX_CYCLES=0"
if not defined FAIL_EXIT_LIMIT set "FAIL_EXIT_LIMIT=3"
set "CYCLE_LOG=build\worker-cycle.log"
set "CYCLE=0"
set "FAILS=0"

if not exist build mkdir build
git rev-parse --git-dir >nul 2>&1
if errorlevel 1 (
    echo [%DATE% %TIME%] not a git repository: %CD%
    exit /b 1
)

echo [%DATE% %TIME%] ==========================================================
echo [%DATE% %TIME%] worker starting
echo [%DATE% %TIME%] directory    : %CD%
echo [%DATE% %TIME%] task prompt  : %TASK_PROMPT%
echo [%DATE% %TIME%] cycle log    : %CYCLE_LOG%
echo [%DATE% %TIME%] stop marker  : %DONE% ^(create this file to stop after the current cycle^)
echo [%DATE% %TIME%] pacing       : next cycle starts when codex exits; no fixed wait
echo [%DATE% %TIME%] ==========================================================

:cycle
call :commit_work
if exist "%DONE%" (
    echo [%DATE% %TIME%] !DONE! found, stopping after !CYCLE! cycle^(s^).
    exit /b 0
)
set /a CYCLE+=1 >nul
if %MAX_CYCLES% gtr 0 if !CYCLE! gtr %MAX_CYCLES% (
    echo [%DATE% %TIME%] cycle limit %MAX_CYCLES% reached, stopping.
    exit /b 0
)
echo.
echo [%DATE% %TIME%] ---------- cycle !CYCLE! starting ----------
call :run_codex
if !FAILS! geq %FAIL_EXIT_LIMIT% (
    echo [%DATE% %TIME%] codex exited nonzero !FAILS! times in a row; inspect %CYCLE_LOG% and stop.
    echo [%DATE% %TIME%] ---------- stopping after !CYCLE! cycle^(s^) ----------
    exit /b 1
)
echo [%DATE% %TIME%] ---------- cycle !CYCLE! finished, looping ----------
goto cycle

:commit_work
git add -A
git diff --cached --quiet >nul 2>&1
if errorlevel 1 (
    git commit -m "work cycle !CYCLE! %DATE%" >nul 2>&1
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
    echo [%DATE% %TIME%] codex exited with %RC%; consecutive failure !FAILS! of %FAIL_EXIT_LIMIT%.
)
exit /b 0
