# Development loop

1. Add a small standalone regression in the relevant fast language suite.
2. Rebuild serially and run the exact case with `Tests/run.ps1 -Suite <suite>
   -Select <test> -CompilerPath <built.exe>`. For an unpackaged host, supply
   `-RuntimeRoot <matching-runtime>`.
3. Run the relevant `Tests/gates.ps1 -Subsystem
   members|replay|lookup|substitution|overloads` gate and affected local suite.
4. Run `tests.cmd` for the normal fast checkpoint, including native integration.
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

## Tools

- `Tests/diagnose.ps1 -Source <repro.cpp>` captures commands, preprocessing,
  hashes, and diagnostics. `CPRIME_PARSER_STATE=1` enables detailed parser state.
- `Tests/check_regressions.ps1` is the compiler publication gate.
- `Tests/run-checks.ps1 -Select <name>` runs a fast native/integration gate.
- Runner changes: `Tests/test_RunnerFixtures.ps1`, `test_SuiteRunner.ps1`, and
  `test_CheckTiers.ps1`. GCC adapter changes also need its focused `test_*.ps1`
  checks, including corpus integrity. These unit checks do not run the inventory.
- Build dispatch changes: `Tests/check_build_default.ps1` and
  `Tests/check_build_regression_gate.ps1`.

Measure identical inputs serially. Keep compiler time separate from driver
startup, reporting, and packaging time. Place logs and measurements in `build/`;
`scripts/windows/clean-dev-logs.ps1 -Days 7 -WhatIf` previews old-log cleanup.
Keep [task.md](../task.md) limited to remaining work; git retains history.
