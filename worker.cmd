@echo off
setlocal EnableExtensions EnableDelayedExpansion

rem CPrime worker: run codex in the foreground, one work cycle at a time, and
rem report progress toward the retained-GCC plus first-party test target.
rem
rem The status printed here is backlog inventory, not verified completion.
rem History lives in a machine-readable append-only log
rem (Tests\progress\log.tsv) that git commits together with the work each sample
rem describes, so frequent stops and starts need no recovery step: the log
rem supplies the cycle counter, and each finished cycle
rem appends the next sample. An unreadable log or corpus is reported and the
rem loop continues; a false "complete" is never claimed.
rem
rem Cycle pacing is work driven: codex runs in the foreground and the next
rem cycle starts as soon as it exits. There is no fixed sleep, and a running
rem agent is never killed. Work packages and acceptance are defined in task.md.
rem Run one worker per working tree; each cycle waits for its own codex.
rem
rem Optional environment overrides:
rem   DONE             stop marker file (default done.x)
rem   CODEX_EXE        codex executable (default codex.exe)
rem   TASK_PROMPT      optional replacement for the task-completion prompt below
rem   MAX_CYCLES       stop after N cycles in this session, 0 = unlimited (default 0)
rem   FAIL_EXIT_LIMIT  stop after N consecutive nonzero codex exits (default 3)
rem   PROGRESS_LOG     progress log (default Tests\progress\log.tsv)
rem   FIRST_PARTY      outstanding first-party list (default Tests\progress\first-party-failures.txt)
rem   CORPUS           retained manifest (default Tests\pedantic\gcc\corpus.json)
rem   STATUS_EXE       progress tool (default build\worker-status.exe)
rem   STATUS_SRC       progress tool source (default src\tools\worker_status.c)
cd /d "%~dp0"

if not defined DONE set "DONE=done.x"
if not defined CODEX_EXE set "CODEX_EXE=codex.exe"
if not defined TASK_PROMPT set "TASK_PROMPT=Complete task.md and Performance/task.md, following AGENTS.md. Read the current GCC inventory and first-party outstanding list, then continue the earliest unfinished work package in dependency order. Map related cases and independent blockers before implementing a coherent fix; retain minimal first-party regressions and run focused checks during repairs. Use only root cpc.exe, one compiler process at a time, and the validated Build.cmd publication path. External compiler invocations remain unauthorized; record those blocked checks explicitly. Preserve existing work, revert failed experiments, and never hide failures or weaken acceptance. Continue useful authorized work until the task is complete; if a cycle must end, update remaining-work files with the exact next action and evidence paths so the next cycle can resume. Keep raw commands, identities, diagnostics and timing evidence under build/. Empty backlog alone is not completion. Create done.x only after every completion condition in task.md has recorded passing evidence, including required correctness, self-build and repeatable speed acceptance. End each cycle with the package, result, changes, tests, unresolved blockers and evidence paths."
if not defined MAX_CYCLES set "MAX_CYCLES=0"
if not defined FAIL_EXIT_LIMIT set "FAIL_EXIT_LIMIT=3"
if not defined PROGRESS_LOG set "PROGRESS_LOG=Tests\progress\log.tsv"
if not defined FIRST_PARTY set "FIRST_PARTY=Tests\progress\first-party-failures.txt"
if not defined CORPUS set "CORPUS=Tests\pedantic\gcc\corpus.json"
if not defined STATUS_EXE set "STATUS_EXE=build\worker-status.exe"
if not defined STATUS_SRC set "STATUS_SRC=src\tools\worker_status.c"
set "CYCLE_LOG="
set "TARGET_REACHED=0"
set "SESSION=0"
set "FAILS=0"

if not exist build mkdir build
git rev-parse --git-dir >nul 2>&1
if errorlevel 1 (
    echo [%DATE% %TIME%] not a git repository: %CD%
    exit /b 1
)

call :build_status
call :read_cycle

echo [%DATE% %TIME%] CPrime worker starting
echo [%DATE% %TIME%] directory    : %CD%
echo [%DATE% %TIME%] task prompt  : %TASK_PROMPT%
echo [%DATE% %TIME%] cycle logs   : build\worker-cycle-N.log
echo [%DATE% %TIME%] progress log : %PROGRESS_LOG%
echo [%DATE% %TIME%] stop marker  : %DONE% ^(create this file to stop after the current cycle^)
call :report

:cycle
call :commit_work
if exist "%DONE%" (
    echo [%DATE% %TIME%] !DONE! present; stopping at cycle !CYCLE!, !SESSION! cycle^(s^) this session.
    exit /b 0
)
if %MAX_CYCLES% gtr 0 if !SESSION! geq %MAX_CYCLES% (
    call :report
    echo [%DATE% %TIME%] session limit %MAX_CYCLES% reached: !SESSION! cycle^(s^) this session; last cycle !CYCLE!.
    exit /b 0
)
set /a SESSION+=1 >nul
set /a CYCLE+=1 >nul
set "CYCLE_LOG=build\worker-cycle-!CYCLE!.log"
echo.
echo [%DATE% %TIME%] ---------- cycle !CYCLE! starting ----------
echo [%DATE% %TIME%] output: !CYCLE_LOG!
call :run_codex
git diff HEAD --stat
git ls-files --others --exclude-standard
call :report append
if "!TARGET_REACHED!"=="1" (
    echo [%DATE% %TIME%] backlog empty; task.md acceptance still controls completion.
    set "TARGET_REACHED=0"
)
if !FAILS! geq %FAIL_EXIT_LIMIT% (
    echo [%DATE% %TIME%] codex exited nonzero !FAILS! times in a row; inspect %CYCLE_LOG% and stop.
    echo [%DATE% %TIME%] ---------- stopping after !CYCLE! cycle^(s^) ----------
    exit /b 1
)
echo [%DATE% %TIME%] ---------- cycle !CYCLE! finished, looping ----------
goto cycle

:build_status
if not exist "%CD%\cpc.exe" (
    echo [%DATE% %TIME%] missing root cpc.exe; progress tool not built.
    exit /b 0
)
if not exist "%STATUS_SRC%" (
    echo [%DATE% %TIME%] missing %STATUS_SRC%; progress report unavailable.
    exit /b 0
)
"%CD%\cpc.exe" -o "%STATUS_EXE%" "%STATUS_SRC%" >nul 2>&1
if errorlevel 1 (
    if exist "%STATUS_EXE%" (
        echo [%DATE% %TIME%] progress tool build failed; reusing %STATUS_EXE%.
    ) else (
        echo [%DATE% %TIME%] progress tool build failed; run "%CD%\cpc.exe" -o "%STATUS_EXE%" "%STATUS_SRC%".
    )
    exit /b 0
)
exit /b 0

:read_cycle
set "CYCLE=0"
if not exist "%STATUS_EXE%" exit /b 0
rem `call` keeps the quoted tool path intact inside the for /f command.
for /f "delims=" %%N in ('call "%STATUS_EXE%" --root "%CD%" --log "%PROGRESS_LOG%" --last-cycle 2^>nul') do set "CYCLE=%%N"
if not defined CYCLE set "CYCLE=0"
exit /b 0

:report
rem %1 = append when the sample describes the cycle that just finished.
if not exist "%STATUS_EXE%" (
    echo [%DATE% %TIME%] no progress tool at %STATUS_EXE%; test completion unknown.
    exit /b 0
)
set "HEAD=-"
for /f "delims=" %%H in ('git rev-parse --short HEAD 2^>nul') do set "HEAD=%%H"
if /i "%~1"=="append" (
    "%STATUS_EXE%" --brief --root "%CD%" --log "%PROGRESS_LOG%" --corpus "%CORPUS%" --first-party "%FIRST_PARTY%" --append --cycle !CYCLE! --event cycle --head "!HEAD!"
) else (
    "%STATUS_EXE%" --brief --root "%CD%" --log "%PROGRESS_LOG%" --corpus "%CORPUS%" --first-party "%FIRST_PARTY%" --head "!HEAD!"
)
set "RC=!ERRORLEVEL!"
if "!RC!"=="10" set "TARGET_REACHED=1"
if not "!RC!"=="0" if not "!RC!"=="10" (
    echo [%DATE% %TIME%] progress report failed with exit !RC!.
)
exit /b 0

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
)
exit /b 0

:run_codex
rem `call` keeps control flow intact when CODEX_EXE is a wrapper script
rem (.cmd/.bat); it is harmless for codex.exe itself.
call "%CODEX_EXE%" exec --model gpt-5.6-terra -c model_reasoning_effort="medium" -C "%CD%" --sandbox danger-full-access -c approval_policy="never" --color never --output-last-message "build\worker-cycle-!CYCLE!-result.txt" "%TASK_PROMPT%" > "%CYCLE_LOG%" 2>&1
set "RC=!ERRORLEVEL!"
echo [%DATE% %TIME%] cycle !CYCLE! process exit !RC!; output: !CYCLE_LOG!
if "!RC!"=="0" (
    set "FAILS=0"
) else (
    set /a FAILS+=1 >nul
    echo [%DATE% %TIME%] codex exited with !RC!; consecutive failure !FAILS! of %FAIL_EXIT_LIMIT%.
)
exit /b 0
