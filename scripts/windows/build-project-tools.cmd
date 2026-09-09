@echo off
setlocal
pushd "%~dp0..\.."
if not exist build mkdir build
cpc.exe -O2 src\tools\build_project.c -ladvapi32 -o build\build_project.new.exe
if errorlevel 1 goto failed
cpc.exe -O2 src\tools\check_project_build.c -o build\check_project_build.new.exe
if errorlevel 1 goto failed
move /y build\build_project.new.exe build\build_project.exe >nul
if errorlevel 1 goto failed
move /y build\check_project_build.new.exe build\check_project_build.exe >nul
if errorlevel 1 goto failed
popd
exit /b 0
:failed
popd
exit /b 1
