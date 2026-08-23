# CPrime Racer Completion Task

Last updated: 2026-08-22

## Goal

Produce a testable Racer build with the current CPC compiler changes.

The task is complete only when:

1. `cpc.exe` is rebuilt from source.
2. Focused compiler regressions pass.
3. Focused Racer object compiles pass under bounded timeouts.
4. `.\build_racer.ps1 -SkipLink -CompileTimeoutSeconds 10` succeeds.
5. The full Racer link succeeds.
6. This exact file exists:

```text
C:\Luke\Src\OT\cl\builds\racer\Racer.exe
```

Do not report Racer as ready before verifying that path.

## Current Blocker

Fix the focused `clImage` compile.

Reproduce with:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\repro\invoke_racer_additional.ps1 clImage 260
```

Current failure:

```text
C:/Luke/Src/OT/cl/CommonLib/commonLib/src/Raster/clImage.cpp:4:
error: cannot convert '_Bool' to 'struct clBitRef__BitStorageElementType'
```

Expected focused output after the fix:

```text
C:\Luke\Src\OT\cl\builds\racer\focus_clImage.obj
```

## Required Work

1. Add or reduce a minimal passing regression for the `clBitRef<T>` proxy
   assignment overload case.
2. Fix member overload resolution/registration so `operator=(bool const&)`
   remains selected for scalar proxy assignments and does not fall back to
   canonical copy assignment.
3. Rebuild CPC:

```cmd
cmd /c build.cmd
```

4. Run focused compiler tests, including:
   - the new proxy assignment regression
   - `Tests\features\Templates\pass\test_inline_member_template_auto_operator_vector_factory_member_call.cpp`
   - nearby touched template/member-overload tests
5. Run focused Racer object compiles in order:
   - `clImage`
   - `clCamera`
   - `clRenderObjectCore`
   - `clRenderObject`
6. Continue fixing the next focused object failure until all focused objects
   compile under bounded timeouts.
7. Run:

```powershell
.\build_racer.ps1 -SkipLink -CompileTimeoutSeconds 10
```

8. Run the full link.
9. Verify:

```powershell
Test-Path 'C:\Luke\Src\OT\cl\builds\racer\Racer.exe'
```

## Cleanup Before Completion

- Remove temporary `CPC_*` diagnostics and debug-only `fprintf(stderr, ...)`
  traces from retained compiler changes.
- Remove scratch reducers and logs that are not promoted to passing tests.
- Keep only coherent, reviewable compiler changes plus passing regression
  coverage.
- State any remaining runtime-affecting warnings explicitly, especially
  implicit declarations, pointer/integer casts, incompatible pointer
  assignments, and read-only assignment warnings.
