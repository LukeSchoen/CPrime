@echo off
setlocal EnableExtensions
pushd "%~dp0..\.."
if not exist build mkdir build
cpc.exe -O2 -Iinclude/runtime -Iinclude/cprime -Ithird-party/win32-sdk/include -Ithird-party/win32-sdk/include/winapi -Isrc/compiler/frontend -Isrc/compiler/middleend -Isrc/compiler/backend/x64 -I. -DCPRIME_TARGET_PE -DCPRIME_TARGET_X86_64 Tests/tools/test_compiler_cold_paths.c -o build/test-compiler-cold-paths.exe > build/test-compiler-cold-paths-build.log 2>&1
if errorlevel 1 (
  type build\test-compiler-cold-paths-build.log
  popd
  exit /b 1
)
build\test-compiler-cold-paths.exe
if errorlevel 1 (popd & exit /b 1)
popd
exit /b 0
