@echo off
setlocal
if not exist "%~dp0build\build_project.exe" (
  call "%~dp0scripts\windows\build-project-tools.cmd"
  if errorlevel 1 exit /b 1
)
"%~dp0build\build_project.exe" %*
exit /b %errorlevel%
