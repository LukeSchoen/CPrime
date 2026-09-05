@echo off
setlocal EnableExtensions EnableDelayedExpansion

set "SCRIPT_DIR=%~dp0"
for %%I in ("%SCRIPT_DIR%..") do set "ROOT=%%~fI"
set "COMPILER=%ROOT%\cpc.exe"
set "COMPILER_PATH="

:parse_args
if "%~1"=="" goto args_done
if /I "%~1"=="-CompilerPath" (
  set "COMPILER_PATH=%~2"
  shift
  shift
  goto parse_args
)
shift
goto parse_args

:args_done
if defined COMPILER_PATH set "COMPILER=%COMPILER_PATH%"
set "SRC_DIR=%SCRIPT_DIR%features\MultiSource\pass"
set "WORK_DIR=%TEMP%\cprime-multisource-tests-%RANDOM%-%RANDOM%"

if not exist "%COMPILER%" (
  echo Unable to find cpc.exe at %COMPILER%.
  exit /b 1
)

if exist "%WORK_DIR%" rmdir /s /q "%WORK_DIR%" >nul 2>nul
mkdir "%WORK_DIR%" >nul 2>nul
if errorlevel 1 (
  echo Failed to create test work dir: %WORK_DIR%
  exit /b 1
)

set "A_SRC=%SRC_DIR%\header_inline_member_a.cpp"
set "MAIN_SRC=%SRC_DIR%\header_inline_member_main.cpp"

"%COMPILER%" "%A_SRC%" "%MAIN_SRC%" -o "%WORK_DIR%\whole.exe" >"%WORK_DIR%\whole.out" 2>&1
set "WHOLE_EXIT=%ERRORLEVEL%"
if not "%WHOLE_EXIT%"=="0" (
  echo FAIL header inline member whole-program compile: exit %WHOLE_EXIT%
  type "%WORK_DIR%\whole.out"
  goto fail
)
findstr /I /C:"defined twice" "%WORK_DIR%\whole.out" >nul 2>nul
if not errorlevel 1 (
  echo FAIL header inline member whole-program compile emitted duplicate-symbol diagnostics.
  type "%WORK_DIR%\whole.out"
  goto fail
)
"%WORK_DIR%\whole.exe"
if errorlevel 1 (
  echo FAIL header inline member whole-program executable returned %ERRORLEVEL%.
  goto fail
)

"%COMPILER%" -c "%A_SRC%" -o "%WORK_DIR%\a.o" >"%WORK_DIR%\compile_a.out" 2>&1
if errorlevel 1 (
  echo FAIL header inline member object compile a.cpp.
  type "%WORK_DIR%\compile_a.out"
  goto fail
)
"%COMPILER%" -c "%MAIN_SRC%" -o "%WORK_DIR%\main.o" >"%WORK_DIR%\compile_main.out" 2>&1
if errorlevel 1 (
  echo FAIL header inline member object compile main.cpp.
  type "%WORK_DIR%\compile_main.out"
  goto fail
)
"%COMPILER%" "%WORK_DIR%\a.o" "%WORK_DIR%\main.o" -o "%WORK_DIR%\linked.exe" >"%WORK_DIR%\link.out" 2>&1
set "LINK_EXIT=%ERRORLEVEL%"
if not "%LINK_EXIT%"=="0" (
  echo FAIL header inline member object link: exit %LINK_EXIT%
  type "%WORK_DIR%\link.out"
  goto fail
)
findstr /I /C:"defined twice" "%WORK_DIR%\link.out" >nul 2>nul
if not errorlevel 1 (
  echo FAIL header inline member object link emitted duplicate-symbol diagnostics.
  type "%WORK_DIR%\link.out"
  goto fail
)
"%WORK_DIR%\linked.exe"
if errorlevel 1 (
  echo FAIL header inline member linked executable returned %ERRORLEVEL%.
  goto fail
)

echo PASS header inline member multi-source

set "TEMPLATE_A_SRC=%SRC_DIR%\header_template_inline_member_a.cpp"
set "TEMPLATE_MAIN_SRC=%SRC_DIR%\header_template_inline_member_main.cpp"

"%COMPILER%" -c "%TEMPLATE_A_SRC%" -o "%WORK_DIR%\template_a.o" >"%WORK_DIR%\compile_template_a.out" 2>&1
if errorlevel 1 (
  echo FAIL header template inline member object compile a.cpp.
  type "%WORK_DIR%\compile_template_a.out"
  goto fail
)
"%COMPILER%" -c "%TEMPLATE_MAIN_SRC%" -o "%WORK_DIR%\template_main.o" >"%WORK_DIR%\compile_template_main.out" 2>&1
if errorlevel 1 (
  echo FAIL header template inline member object compile main.cpp.
  type "%WORK_DIR%\compile_template_main.out"
  goto fail
)
"%COMPILER%" "%WORK_DIR%\template_a.o" "%WORK_DIR%\template_main.o" -o "%WORK_DIR%\template_linked.exe" >"%WORK_DIR%\link_template.out" 2>&1
set "TEMPLATE_LINK_EXIT=%ERRORLEVEL%"
if not "%TEMPLATE_LINK_EXIT%"=="0" (
  echo FAIL header template inline member object link: exit %TEMPLATE_LINK_EXIT%
  type "%WORK_DIR%\link_template.out"
  goto fail
)
findstr /I /C:"defined twice" "%WORK_DIR%\link_template.out" >nul 2>nul
if not errorlevel 1 (
  echo FAIL header template inline member object link emitted duplicate-symbol diagnostics.
  type "%WORK_DIR%\link_template.out"
  goto fail
)
"%WORK_DIR%\template_linked.exe"
if errorlevel 1 (
  echo FAIL header template inline member linked executable returned %ERRORLEVEL%.
  goto fail
)

echo PASS header template inline member multi-source

set "OUT_CLASS_A_SRC=%SRC_DIR%\out_of_class_template_member_a.cpp"
set "OUT_CLASS_MAIN_SRC=%SRC_DIR%\out_of_class_template_member_main.cpp"

"%COMPILER%" -c "%OUT_CLASS_A_SRC%" -o "%WORK_DIR%\out_class_a.o" >"%WORK_DIR%\compile_out_class_a.out" 2>&1
if errorlevel 1 (
  echo FAIL out-of-class template member object compile a.cpp.
  type "%WORK_DIR%\compile_out_class_a.out"
  goto fail
)
"%COMPILER%" -c "%OUT_CLASS_MAIN_SRC%" -o "%WORK_DIR%\out_class_main.o" >"%WORK_DIR%\compile_out_class_main.out" 2>&1
if errorlevel 1 (
  echo FAIL out-of-class template member object compile main.cpp.
  type "%WORK_DIR%\compile_out_class_main.out"
  goto fail
)
"%COMPILER%" "%WORK_DIR%\out_class_a.o" "%WORK_DIR%\out_class_main.o" -o "%WORK_DIR%\out_class_linked.exe" >"%WORK_DIR%\link_out_class.out" 2>&1
set "OUT_CLASS_LINK_EXIT=%ERRORLEVEL%"
if not "%OUT_CLASS_LINK_EXIT%"=="0" (
  echo FAIL out-of-class template member object link: exit %OUT_CLASS_LINK_EXIT%
  type "%WORK_DIR%\link_out_class.out"
  goto fail
)
findstr /I /C:"defined twice" "%WORK_DIR%\link_out_class.out" >nul 2>nul
if not errorlevel 1 (
  echo FAIL out-of-class template member object link emitted duplicate-symbol diagnostics.
  type "%WORK_DIR%\link_out_class.out"
  goto fail
)
"%WORK_DIR%\out_class_linked.exe"
if errorlevel 1 (
  echo FAIL out-of-class template member linked executable returned %ERRORLEVEL%.
  goto fail
)

echo PASS out-of-class template member multi-source

set "TEMPLATE_FUNC_A_SRC=%SRC_DIR%\header_template_function_a.cpp"
set "TEMPLATE_FUNC_MAIN_SRC=%SRC_DIR%\header_template_function_main.cpp"

"%COMPILER%" -c "%TEMPLATE_FUNC_A_SRC%" -o "%WORK_DIR%\template_func_a.o" >"%WORK_DIR%\compile_template_func_a.out" 2>&1
if errorlevel 1 (
  echo FAIL header template function object compile a.cpp.
  type "%WORK_DIR%\compile_template_func_a.out"
  goto fail
)
"%COMPILER%" -c "%TEMPLATE_FUNC_MAIN_SRC%" -o "%WORK_DIR%\template_func_main.o" >"%WORK_DIR%\compile_template_func_main.out" 2>&1
if errorlevel 1 (
  echo FAIL header template function object compile main.cpp.
  type "%WORK_DIR%\compile_template_func_main.out"
  goto fail
)
"%COMPILER%" "%WORK_DIR%\template_func_a.o" "%WORK_DIR%\template_func_main.o" -o "%WORK_DIR%\template_func_linked.exe" >"%WORK_DIR%\link_template_func.out" 2>&1
set "TEMPLATE_FUNC_LINK_EXIT=%ERRORLEVEL%"
if not "%TEMPLATE_FUNC_LINK_EXIT%"=="0" (
  echo FAIL header template function object link: exit %TEMPLATE_FUNC_LINK_EXIT%
  type "%WORK_DIR%\link_template_func.out"
  goto fail
)
findstr /I /C:"defined twice" "%WORK_DIR%\link_template_func.out" >nul 2>nul
if not errorlevel 1 (
  echo FAIL header template function object link emitted duplicate-symbol diagnostics.
  type "%WORK_DIR%\link_template_func.out"
  goto fail
)
"%WORK_DIR%\template_func_linked.exe"
if errorlevel 1 (
  echo FAIL header template function linked executable returned %ERRORLEVEL%.
  goto fail
)

echo PASS header template function multi-source

set "FRIEND_A_SRC=%SRC_DIR%\header_friend_definition_a.cpp"
set "FRIEND_MAIN_SRC=%SRC_DIR%\header_friend_definition_main.cpp"

"%COMPILER%" -c "%FRIEND_A_SRC%" -o "%WORK_DIR%\friend_a.o" >"%WORK_DIR%\compile_friend_a.out" 2>&1
if errorlevel 1 (
  echo FAIL header friend definition object compile a.cpp.
  type "%WORK_DIR%\compile_friend_a.out"
  goto fail
)
"%COMPILER%" -c "%FRIEND_MAIN_SRC%" -o "%WORK_DIR%\friend_main.o" >"%WORK_DIR%\compile_friend_main.out" 2>&1
if errorlevel 1 (
  echo FAIL header friend definition object compile main.cpp.
  type "%WORK_DIR%\compile_friend_main.out"
  goto fail
)
"%COMPILER%" "%WORK_DIR%\friend_a.o" "%WORK_DIR%\friend_main.o" -o "%WORK_DIR%\friend_linked.exe" >"%WORK_DIR%\link_friend.out" 2>&1
if errorlevel 1 (
  echo FAIL header friend definition object link.
  type "%WORK_DIR%\link_friend.out"
  goto fail
)
"%WORK_DIR%\friend_linked.exe"
if errorlevel 1 (
  echo FAIL header friend definition executable returned %ERRORLEVEL%.
  goto fail
)

echo PASS header friend definition multi-source

set "LIFE_SRC=%SRC_DIR%\external_lifecycle.cpp"
set "LIFE_MAIN_SRC=%SRC_DIR%\external_lifecycle_main.cpp"

"%COMPILER%" -c "%LIFE_SRC%" -o "%WORK_DIR%\external_lifecycle.o" >"%WORK_DIR%\compile_lifecycle.out" 2>&1
if errorlevel 1 (
  echo FAIL external lifecycle object compile.
  type "%WORK_DIR%\compile_lifecycle.out"
  goto fail
)
"%COMPILER%" -c "%LIFE_MAIN_SRC%" -o "%WORK_DIR%\external_lifecycle_main.o" >"%WORK_DIR%\compile_lifecycle_main.out" 2>&1
if errorlevel 1 (
  echo FAIL external lifecycle main object compile.
  type "%WORK_DIR%\compile_lifecycle_main.out"
  goto fail
)
"%COMPILER%" "%WORK_DIR%\external_lifecycle.o" "%WORK_DIR%\external_lifecycle_main.o" -o "%WORK_DIR%\external_lifecycle.exe" >"%WORK_DIR%\link_lifecycle.out" 2>&1
if errorlevel 1 (
  echo FAIL external lifecycle object link.
  type "%WORK_DIR%\link_lifecycle.out"
  goto fail
)
"%WORK_DIR%\external_lifecycle.exe"
if errorlevel 1 (
  echo FAIL external lifecycle linked executable returned %ERRORLEVEL%.
  goto fail
)

echo PASS external lifecycle multi-source

set "DEFAULT_CTOR_SRC=%SRC_DIR%\external_default_constructor_args.cpp"
set "DEFAULT_CTOR_MAIN_SRC=%SRC_DIR%\external_default_constructor_args_main.cpp"

"%COMPILER%" -c "%DEFAULT_CTOR_SRC%" -o "%WORK_DIR%\default_ctor.o" >"%WORK_DIR%\compile_default_ctor.out" 2>&1
if errorlevel 1 (
  echo FAIL default-argument constructor object compile.
  type "%WORK_DIR%\compile_default_ctor.out"
  goto fail
)
"%COMPILER%" -c "%DEFAULT_CTOR_MAIN_SRC%" -o "%WORK_DIR%\default_ctor_main.o" >"%WORK_DIR%\compile_default_ctor_main.out" 2>&1
if errorlevel 1 (
  echo FAIL default-argument constructor main compile.
  type "%WORK_DIR%\compile_default_ctor_main.out"
  goto fail
)
"%COMPILER%" "%WORK_DIR%\default_ctor.o" "%WORK_DIR%\default_ctor_main.o" -o "%WORK_DIR%\default_ctor.exe" >"%WORK_DIR%\link_default_ctor.out" 2>&1
if errorlevel 1 (
  echo FAIL default-argument constructor object link.
  type "%WORK_DIR%\link_default_ctor.out"
  goto fail
)
"%WORK_DIR%\default_ctor.exe"
if errorlevel 1 (
  echo FAIL default-argument constructor executable returned %ERRORLEVEL%.
  goto fail
)

echo PASS default-argument constructor multi-source

set "DEFAULT_ASSIGN_A_SRC=%SRC_DIR%\test_defaulted_assign_a.cpp"
set "DEFAULT_ASSIGN_MAIN_SRC=%SRC_DIR%\test_defaulted_assign_main.cpp"

"%COMPILER%" -c "%DEFAULT_ASSIGN_A_SRC%" -o "%WORK_DIR%\default_assign_a.o" >"%WORK_DIR%\compile_default_assign_a.out" 2>&1
if errorlevel 1 (
  echo FAIL defaulted assignment object compile a.cpp.
  type "%WORK_DIR%\compile_default_assign_a.out"
  goto fail
)
"%COMPILER%" -c "%DEFAULT_ASSIGN_MAIN_SRC%" -o "%WORK_DIR%\default_assign_main.o" >"%WORK_DIR%\compile_default_assign_main.out" 2>&1
if errorlevel 1 (
  echo FAIL defaulted assignment main compile.
  type "%WORK_DIR%\compile_default_assign_main.out"
  goto fail
)
"%COMPILER%" "%WORK_DIR%\default_assign_a.o" "%WORK_DIR%\default_assign_main.o" -o "%WORK_DIR%\default_assign.exe" >"%WORK_DIR%\link_default_assign.out" 2>&1
if errorlevel 1 (
  echo FAIL defaulted assignment object link.
  type "%WORK_DIR%\link_default_assign.out"
  goto fail
)
"%WORK_DIR%\default_assign.exe"
if errorlevel 1 (
  echo FAIL defaulted assignment executable returned %ERRORLEVEL%.
  goto fail
)

echo PASS defaulted assignment multi-source

"%COMPILER%" -c "%SRC_DIR%\header_implicit_lifecycle_a.cpp" -o "%WORK_DIR%\implicit_lifecycle_a.o" >"%WORK_DIR%\implicit_lifecycle_a.out" 2>&1
if errorlevel 1 (
  type "%WORK_DIR%\implicit_lifecycle_a.out"
  goto fail
)
"%COMPILER%" -c "%SRC_DIR%\header_implicit_lifecycle_main.cpp" -o "%WORK_DIR%\implicit_lifecycle_main.o" >"%WORK_DIR%\implicit_lifecycle_main.out" 2>&1
if errorlevel 1 (
  type "%WORK_DIR%\implicit_lifecycle_main.out"
  goto fail
)
"%COMPILER%" "%WORK_DIR%\implicit_lifecycle_a.o" "%WORK_DIR%\implicit_lifecycle_main.o" -o "%WORK_DIR%\implicit_lifecycle.exe" >"%WORK_DIR%\implicit_lifecycle_link.out" 2>&1
if errorlevel 1 (
  type "%WORK_DIR%\implicit_lifecycle_link.out"
  goto fail
)
findstr /I /C:"defined twice" "%WORK_DIR%\implicit_lifecycle_link.out" >nul 2>nul
if not errorlevel 1 (
  type "%WORK_DIR%\implicit_lifecycle_link.out"
  goto fail
)
"%WORK_DIR%\implicit_lifecycle.exe"
if errorlevel 1 (
  echo FAIL implicit destructor and constructor template executable returned %ERRORLEVEL%.
  goto fail
)
echo PASS implicit destructor and constructor template multi-source
for %%S in (language_linkage_a.cpp language_linkage_c.c language_linkage_main.cpp) do (
  "%COMPILER%" -Werror -c "%SRC_DIR%\%%S" -o "%WORK_DIR%\%%S.o" >"%WORK_DIR%\%%S.out" 2>&1
  if errorlevel 1 (
    type "%WORK_DIR%\%%S.out"
    goto fail
  )
)
"%COMPILER%" "%WORK_DIR%\language_linkage_a.cpp.o" "%WORK_DIR%\language_linkage_c.c.o" "%WORK_DIR%\language_linkage_main.cpp.o" -o "%WORK_DIR%\language_linkage.exe" >"%WORK_DIR%\language_linkage.out" 2>&1
if errorlevel 1 (
  type "%WORK_DIR%\language_linkage.out"
  goto fail
)
findstr /I /C:"defined twice" "%WORK_DIR%\language_linkage.out" >nul 2>nul
if not errorlevel 1 (
  type "%WORK_DIR%\language_linkage.out"
  goto fail
)
"%WORK_DIR%\language_linkage.exe"
if errorlevel 1 (
  echo FAIL C and C++ language linkage executable returned %ERRORLEVEL%.
  goto fail
)
echo PASS C and C++ language linkage multi-source
rmdir /s /q "%WORK_DIR%" >nul 2>nul
exit /b 0

:fail
if exist "%WORK_DIR%" rmdir /s /q "%WORK_DIR%" >nul 2>nul
exit /b 1
