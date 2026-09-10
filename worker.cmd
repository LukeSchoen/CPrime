@echo off
setlocal EnableExtensions EnableDelayedExpansion

rem Run from the repository root that contains this script.
cd /d "%~dp0"

if not defined DONE set "DONE=done.x"
if not defined CODEX_EXE set "CODEX_EXE=codex.exe"
if not defined TASK_PROMPT set "TASK_PROMPT=implement task.md"
if not defined WAIT_MINUTES set "WAIT_MINUTES=45"
set "CYCLE=0"

echo [%DATE% %TIME%] ==========================================================
echo [%DATE% %TIME%] worker starting
echo [%DATE% %TIME%] directory    : %CD%
echo [%DATE% %TIME%] task prompt  : %TASK_PROMPT%
echo [%DATE% %TIME%] cycle length : %WAIT_MINUTES% minute^(s^)
echo [%DATE% %TIME%] stop marker  : %DONE% ^(create this file to stop after the current cycle^)
echo [%DATE% %TIME%] ==========================================================

:cycle
set /a CYCLE+=1 >nul
echo.
echo [%DATE% %TIME%] ---------- cycle !CYCLE! starting ----------
call :commit_work
if exist "%DONE%" (
    echo [%DATE% %TIME%] !DONE! found, stopping after !CYCLE! cycle^(s^).
    exit /b 0
)
call :clear_codex
call :start_codex
call :wait_cycle
echo [%DATE% %TIME%] ---------- cycle !CYCLE! finished, looping ----------
goto cycle

:commit_work
echo [%DATE% %TIME%] committing any work left from the previous cycle...
git add -A
git commit -m "work"
if errorlevel 1 (
    echo [%DATE% %TIME%] nothing committed ^(working tree clean or no repository^).
) else (
    for /f "delims=" %%H in ('git rev-parse --short HEAD 2^>nul') do set "HEAD=%%H"
    echo [%DATE% %TIME%] committed work as !HEAD!.
)
exit /b 0

:clear_codex
rem Terminate only Codex processes launched for this task and their child trees.
set "KILLED=0"
for /f "tokens=2 delims==" %%P in ('%SystemRoot%\System32\wbem\WMIC.exe process where "name='codex.exe' and CommandLine like '%%implement task.md%%'" get ProcessId /value 2^>nul ^| find "="') do (
    if not "%%P"=="" (
        echo [%DATE% %TIME%] stopping previous codex process %%P
        taskkill /PID %%P /T /F
        set /a KILLED+=1 >nul
    )
)
if "!KILLED!"=="0" echo [%DATE% %TIME%] no previous codex process was running.
exit /b 0

:start_codex
echo [%DATE% %TIME%] launching codex: %CODEX_EXE% exec "%TASK_PROMPT%"
start "" /B "%ComSpec%" /d /c ""%CODEX_EXE%" exec "%TASK_PROMPT%" >nul 2>&1"
echo [%DATE% %TIME%] codex running in the background ^(its own output is suppressed^).
exit /b 0

:wait_cycle
echo [%DATE% %TIME%] waiting %WAIT_MINUTES% minute^(s^), progress prints once a minute
for /l %%M in (1,1,%WAIT_MINUTES%) do (
    %SystemRoot%\System32\ping.exe -n 61 127.0.0.1 >nul 2>&1
    echo [%DATE% %TIME%] cycle !CYCLE!: waited %%M of %WAIT_MINUTES% minute^(s^)
)
echo [%DATE% %TIME%] wait complete
exit /b 0
