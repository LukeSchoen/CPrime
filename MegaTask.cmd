@echo off
setlocal EnableExtensions DisableDelayedExpansion

rem MegaTask.cmd - repeatedly give task.md to Codex until it is complete or time expires.
rem Runtime files live under .git so they do not dirty the working tree.

cd /d "%~dp0" || goto :bad_directory
set "MEGA_ROOT=%CD%"
set "MEGA_TASK=%MEGA_ROOT%\task.md"
set "MEGA_DONE=%MEGA_ROOT%\.git\megatask.done"
set "MEGA_LAST=%MEGA_ROOT%\.git\megatask-last-message.txt"

if not exist "%MEGA_TASK%" (
    echo ERROR: task.md was not found at "%MEGA_TASK%".
    exit /b 2
)
where powershell.exe >nul 2>&1 || (
    echo ERROR: Windows PowerShell is required but was not found.
    exit /b 2
)
where codex.exe >nul 2>&1 || (
    echo ERROR: codex.exe is not on PATH.
    echo Run "codex login" after installing the Codex CLI, then try again.
    exit /b 2
)

call :choose_runtime
if errorlevel 10 (
    echo Cancelled.
    exit /b 1
)
if errorlevel 4 (set "MEGA_HOURS=12") else if errorlevel 3 (set "MEGA_HOURS=4") else if errorlevel 2 (set "MEGA_HOURS=2") else (set "MEGA_HOURS=1")

del /q "%MEGA_DONE%" "%MEGA_LAST%" >nul 2>&1
for /f %%T in ('powershell.exe -NoProfile -Command "[DateTimeOffset]::UtcNow.ToUnixTimeSeconds()"') do set "MEGA_START=%%T"
set /a MEGA_LIMIT=MEGA_HOURS*3600
set /a MEGA_DEADLINE=MEGA_START+MEGA_LIMIT
set "MEGA_MODE=new"
set /a MEGA_RUN=0

echo.
echo MegaTask will work on task.md for up to %MEGA_HOURS% hour(s).
echo Press Ctrl+C if you need to stop it early.
echo Completion marker: "%MEGA_DONE%"
echo.

:run_again
for /f %%T in ('powershell.exe -NoProfile -Command "[DateTimeOffset]::UtcNow.ToUnixTimeSeconds()"') do set "MEGA_NOW=%%T"
set /a MEGA_REMAINING=MEGA_DEADLINE-MEGA_NOW
if %MEGA_REMAINING% LEQ 0 goto :timed_out
if exist "%MEGA_DONE%" goto :complete

set /a MEGA_RUN+=1
echo ============================================================
echo Codex run %MEGA_RUN% - up to %MEGA_REMAINING% seconds remain
echo ============================================================

set "MEGA_PROMPT=Read task.md and continue doing the task it describes. Work persistently: diagnose, implement, build, and test; do not merely report the next failure. Inspect the existing working tree and preserve valid prior progress. Only when every requested deliverable is genuinely complete and verified, create the file .git\megatask.done containing a short completion note. Do not create that marker for partial progress or a blocker."
if /i "%MEGA_MODE%"=="resume" set "MEGA_PROMPT=Continue the task in task.md from the current working tree. Keep implementing and testing. Only create .git\megatask.done when every requested deliverable is genuinely complete and verified."

powershell.exe -NoProfile -ExecutionPolicy Bypass -Command ^
  "$ErrorActionPreference='Stop'; $remaining=[int]$env:MEGA_REMAINING; $dq=[char]34; $root=$dq+$env:MEGA_ROOT+$dq; $last=$dq+$env:MEGA_LAST+$dq; $prompt=$dq+$env:MEGA_PROMPT+$dq; $common=@('-a','never','-s','danger-full-access','-C',$root,'exec'); if($env:MEGA_MODE -eq 'resume'){$a=$common+@('resume','--last','-o',$last,$prompt)}else{$a=$common+@('-o',$last,$prompt)}; try{$p=Start-Process -FilePath 'codex.exe' -ArgumentList $a -NoNewWindow -PassThru; if(-not $p.WaitForExit($remaining*1000)){Write-Host ''; Write-Host 'Time limit reached; stopping Codex...'; & taskkill.exe /PID $p.Id /T /F 2^>$null ^| Out-Null; exit 124}; exit $p.ExitCode}catch{Write-Error $_; exit 125}"
set "MEGA_RESULT=%ERRORLEVEL%"

if exist "%MEGA_DONE%" goto :complete
if "%MEGA_RESULT%"=="124" goto :timed_out
if "%MEGA_RESULT%"=="0" (
    set "MEGA_MODE=resume"
) else (
    echo.
    echo WARNING: Codex exited with code %MEGA_RESULT%. A fresh run will be tried.
    set "MEGA_MODE=new"
)

for /f %%T in ('powershell.exe -NoProfile -Command "[DateTimeOffset]::UtcNow.ToUnixTimeSeconds()"') do set "MEGA_NOW=%%T"
set /a MEGA_REMAINING=MEGA_DEADLINE-MEGA_NOW
if %MEGA_REMAINING% LEQ 0 goto :timed_out
echo.
echo Task is not marked complete. Continuing in 5 seconds...
timeout /t 5 /nobreak >nul
goto :run_again

:complete
echo.
echo SUCCESS: Codex marked task.md complete after %MEGA_RUN% run(s).
echo Review its note in "%MEGA_DONE%".
if exist "%MEGA_LAST%" echo Last Codex response: "%MEGA_LAST%"
exit /b 0

:timed_out
echo.
echo TIME LIMIT: task.md was not completed within %MEGA_HOURS% hour(s).
echo Existing source changes have been left in place for inspection or a later run.
if exist "%MEGA_LAST%" echo Last Codex response: "%MEGA_LAST%"
exit /b 3

:choose_runtime
powershell.exe -NoProfile -ExecutionPolicy Bypass -Command ^
  "$items=@('1 hour','2 hours','4 hours','12 hours'); $i=0; [Console]::CursorVisible=$false; try{while($true){Clear-Host; Write-Host 'How long should MegaTask keep Codex working?'; Write-Host 'Use Up/Down and Enter. Esc cancels.'; Write-Host ''; for($n=0;$n -lt $items.Count;$n++){if($n -eq $i){Write-Host ('  > '+$items[$n]) -ForegroundColor Cyan}else{Write-Host ('    '+$items[$n])}}; $k=[Console]::ReadKey($true).Key; if($k -eq 'UpArrow'){$i=($i+$items.Count-1)%%$items.Count}elseif($k -eq 'DownArrow'){$i=($i+1)%%$items.Count}elseif($k -eq 'Enter'){exit ($i+1)}elseif($k -eq 'Escape'){exit 10}}}catch{Write-Host 'ERROR: An interactive console is required.'; exit 10}finally{[Console]::CursorVisible=$true}"
exit /b %ERRORLEVEL%

:bad_directory
echo ERROR: Could not enter the MegaTask directory.
exit /b 2
