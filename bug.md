# Batch mode crash handoff

## Symptom

`cpc.exe` batch mode (`@targets.txt`) is unstable on a full test manifest. It succeeds for early jobs, then crashes when many compile jobs are executed in one process.

## Expected vs actual

- Expected: `cpc.exe @"Tests\batch\targets.txt"` compiles all listed jobs and exits `0`.
- Actual: run crashes at job 16 (`Tests\features\All\pass\test_cpp_style_all_in_one.c`) with exit code `-1073740940`.

## Repro (current)

```bat
cmd /c Tests\test_batch.cmd
```

Behavior:
- Jobs 1-15 succeed.
- Crash occurs on job 16 during compile phase.
- Crash point is consistent across runs.

## Repro setup details

`Tests\test_batch.cmd` generates `Tests\batch\targets.txt` from `Tests\*\pass\test_*.c` and runs:

```bat
cpc.exe @"Tests\batch\targets.txt"
```

Each line is an independent compile job with its own `-o` output.

## Why this is likely global state leakage

- Failing source compiles successfully when run alone.
- Failure only appears after prior jobs in the same `cpc.exe` process.
- Crash occurs before per-job completion marker on job 16, so this is in compile-time execution, not final teardown.

## Already attempted / ruled out

- Added and verified per-job create/delete path in batch driver.
- Cleared template-related caches in `src/compiler/frontend/cprimegen.c`.
- Cleared `member_func_overloads` cache.
- Added assembler reset (`last_text_section`, `asmgoto_n`) in `src/compiler/frontend/cprimeasm.c`.
- Added extra preprocessor scalar/token buffer resets in `src/compiler/frontend/cprimepp.c`.
- Confirmed obvious debug counter/hash reset paths exist in `src/compiler/middleend/cprimedbg.c`.
- Result after all above: crash still reproduces at same job.

## Most likely remaining fault areas

- Hidden preprocessor globals not covered by current reset path in `src/compiler/frontend/cprimepp.c`.
- C++ parser/codegen static state not fully reset in `src/compiler/frontend/cprimegen.c`.
- Debug/DWARF side state that survives job boundaries in `src/compiler/middleend/cprimedbg.c`.

## Files involved

- `src/compiler/driver/cprime.c`
- `src/compiler/frontend/cprimegen.c`
- `src/compiler/frontend/cprimeasm.c`
- `src/compiler/frontend/cprimepp.c`
- `include/cprime/cprime.h`
- `Tests/test_batch.cmd`
- `Tests/batch/targets.txt`

dont report where the bug is, fix it
