# Development loop

1. Add a minimal regression in `Tests/features/<suite>/pass|fail` or `Tests/c_compat`.
2. Rebuild serially and run the exact case:
   `Tests/run.ps1 -Suite features/Templates -Select test_name.cpp -CompilerPath <built.exe>`.
   For an unpackaged host, also provide `-RuntimeRoot <matching-runtime>`.
3. Run `Tests/gates.ps1 -Subsystem members|replay|lookup|substitution|overloads`
   with the same compiler/runtime, then the affected full suite. Extend gates
   when a bug escapes; retain positive and negative coverage.
4. Run `tests.cmd` for broad local coverage. At compiler checkpoints and before
   merge/release, run the [GCC assessment](gcc/README.md) with a fresh output
   directory and compare against a baseline made with the same inputs.

Compile and build serially: one compiler process at a time. Local and GCC
language runners enforce five seconds per compilation/execution, including
startup. Crashes and timeouts are failures, including for negative tests.
Do not weaken expectations or remove slow cases to improve totals.

## Diagnostics and verification

- `Tests/diagnose.ps1 -Source <repro.cpp> -CompilerPath <built.exe> -RuntimeRoot <runtime>`:
  capture preprocessing, commands, hashes, and diagnostics under `build/`.
  `CPRIME_PARSER_STATE=1` adds parser/replay state for minimized reproducers.
- Build dispatch changes: `Tests/check_build_default.ps1` and
  `Tests/check_build_regression_gate.ps1`.
- Compiler release gate: `Tests/check_regressions.ps1 -CompilerPath <built.exe>`.
- Host/runtime changes: rebuild, then `Tests/check_clang_selfhost.ps1` for
  C compatibility and regression checks across two serial self-build generations.
- Runner changes: `Tests/test_RunnerFixtures.ps1` and the GCC adapter tests.
- Packaging/runtime/ABI changes: choose the relevant tools in [README.md](README.md).

Measure identical inputs and record compiler/runtime hashes and settings.
Separate compiler process time from driver overhead; startup/wait timings are
parts of process wall time, not additional compiler time.

## Repository upkeep

Keep [task.md](../task.md) limited to unfinished work. Documentation contains
usage and current requirements; git retains decisions, investigations, and
completed work. Regression tests retain the behavior we need to protect.

`scripts/windows/clean-dev-logs.ps1 -Days 7 -WhatIf` previews old text-log cleanup
under `build/`; omit `-WhatIf` to remove them while retaining JSON evidence and
upstream sources.
