# Development loop

Use root `cpc.exe`. Plain `Build.cmd` self-hosts when compiler sources change,
then test the updated root `cpc.exe`. Do not invoke other compilers for host builds,
reference comparisons, or benchmarks without an explicit user request. A test
failure does not authorize switching compilers. Keep CPC as the default host.

1. Reduce the bug to a small standalone regression in the relevant fast language
   suite inside CPrime, including every required fixture. Do not depend on a
   sibling application repository or its libraries.
2. Rebuild serially with `Build.cmd`, then run the exact case with
   `Tests/run.ps1 -Suite <suite> -Select <test>` using root `cpc.exe`.
3. Run the relevant `Tests/gates.ps1 -Subsystem
   members|replay|lookup|substitution|overloads` gate and affected local suite.
4. Run `Tests/run-all.ps1` for the fast language checkpoint and selected CPC-only
   native/integration gates. `tests.cmd` and `-IncludeChecks` also invoke external
   compilers in ABI gates and require explicit authorization for those invocations.
5. **Only at the end of a large change that needs deeper verification**, run
   the relevant `tests_pedantic.cmd -Group gcc|checks|performance` group or the
   complete pedantic entry point. Avoid pedantic sweeps during routine fixes.

Keep compilation and builds serial. Language compiler/program invocations and
fast standalone gates have five-second ceilings. Pedantic standalone gate totals
have explicit longer budgets in `checks.json`. Record timeouts as failures;
never retry with relaxed budgets or silently disable slow checks.

When fixing a retained GCC failure, use an exact selection:
`Tests/pedantic/gcc/run.ps1 -Select g++.dg/template/access27.C -Out build/<run>`.
Promote its key behavior into a small fast regression. Bulk GCC sweeps remain
pedantic; their score covers the retained corpus, not the deleted full inventory.

Keep every test minimal and deterministic, including pedantic tests. Cover one
distinct behavior with the smallest useful input; reuse helpers and consolidate
duplicates while preserving coverage. Avoid sleeps, oversized inputs, and full
application builds unless the behavior specifically requires them. Keep retained
third-party sources unchanged; add reduced first-party regressions separately.

Regression reports should contain only the repository-relative test path, exact
reproduction command, compiler/runtime identity, expected and actual behavior,
and relevant runner summary with timing. Keep reproducers and fixtures in `Tests/`
and generated reports/logs in this repository's `build/`. Remove obsolete wiring
and stale notes as cases evolve; resolved work belongs in tests and git history.

## Tools

Prefer C (.c) compiled with root `cpc.exe` into native .exe tools for new build
drivers, test helpers, and similar automation. Lower startup overhead helps
repeated invocations, and C tools are easy to edit and recompile. Avoid new
PowerShell (.ps1) implementations by default, but consider trying PowerShell
when the C/native approach is impractical or still performs poorly after
optimization attempts. Compare alternatives on identical workloads, including
startup time. Keep tool implementation in `src/`, test-specific sources in
`Tests/`, and generated executables in `build/`.

- `Tests/diagnose.ps1 -Source <repro.cpp>` captures commands, preprocessing,
  hashes, and diagnostics. `CPRIME_PARSER_STATE=1` enables detailed parser state.
- `Tests/check_regressions.ps1` is the compiler publication gate.
- `Tests/run-checks.ps1 -Select <name>` runs a fast native/integration gate.
- Runner changes: `Tests/test_RunnerFixtures.ps1`, `test_SuiteRunner.ps1`, and
  `test_CheckTiers.ps1`. GCC adapter changes also need its focused `test_*.ps1`
  checks, including corpus integrity. These unit checks do not run the inventory.
- Build dispatch changes: `Tests/check_build_default.ps1` and
  `Tests/check_build_regression_gate.ps1`.
- Project cache changes: fast `Tests/run-checks.ps1 -Select test_IncrementalCache`.
  Rebuild native tools with `scripts/windows/build-project-tools.cmd`; run
  `Tests/run-checks.ps1 -Select check_native_project` for graph/resource changes.
  Dependency-invalidation and process changes also need the fast
  `check_incremental_build` and `check_batch_build` selections.

Measure identical inputs serially. Keep compiler time separate from driver
startup, reporting, and packaging time. Place logs and measurements in `build/`;
`scripts/windows/clean-dev-logs.ps1 -Days 7 -WhatIf` previews old-log cleanup.
Keep [task.md](../task.md) limited to remaining work; git retains history.

For Windows CPU sampling, `Build.cmd` writes `build/compiler/cpc.map` for the
published root compiler. Compile `src/tools/profile_process.c` with root CPC
and `-lwinmm`, then run the tool with the map, root compiler, and a CPC response
file. It samples the main thread and its frame-pointer chain (up to 24 frames).
Use separate uninstrumented runs for timings; rebuild the map after compiler edits.
