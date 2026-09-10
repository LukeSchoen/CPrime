@echo off
setlocal EnableExtensions

rem Run from the repository root that contains this script.
cd /d "%~dp0"

set "DONE=done.x"
set "CODEX_EXE=codex.exe"
set "TASK_PROMPT=implement task.md"
set "CYCLE=0"

:cycle
set /a CYCLE+=1
call :commit_work
if exist "%DONE%" (
    echo [%DATE% %TIME%] %DONE% found, stopping after %CYCLE% cycle^(s^).
    exit /b 0
)
echo [%DATE% %TIME%] cycle %CYCLE%: starting codex
call :clear_codex
call :start_codex
echo [%DATE% %TIME%] waiting 15 minutes
%SystemRoot%\System32\ping.exe -n 901 127.0.0.1 >nul 2>&1
goto cycle

:commit_work
git add -A >nul 2>&1
git commit -m "work" >nul 2>&1
exit /b 0

:clear_codex
rem Terminate only Codex processes launched for this task and their child trees.
for /f "tokens=2 delims==" %%P in ('%SystemRoot%\System32\wbem\WMIC.exe process where "name='codex.exe' and CommandLine like '%%implement task.md%%'" get ProcessId /value 2^>nul ^| find "="') do (
    if not "%%P"=="" taskkill /PID %%P /T /F >nul 2>&1
)
exit /b 0

:start_codex
start "" /B "%ComSpec%" /d /c ""%CODEX_EXE%" exec "%TASK_PROMPT%" >nul 2>&1"
exit /b 0
