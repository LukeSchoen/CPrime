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
3. Run the relevant exact regressions, including established passing cases where
   they exercise the changed behavior. Explicit `-Select` crosses tiers unless
   an explicit `-Tier` restricts it.
4. Run `Tests/run-all.ps1` for the active unresolved verification and selected CPC-only
   native/integration gates. `tests.cmd` and `-IncludeChecks` also invoke external
   compilers in ABI gates and require explicit authorization for those invocations.
   After each verification, regenerate the retained-failure triage with
   `Tests/triage_retained_failures.ps1` (it defaults to the newest
   `build/pedantic-gcc-*/results.jsonl`). Read the cluster summary and exact
   paths from `build/compiler-bug-triage.txt`/`.json`; keep full diagnostics in
   the result file rather than reproducing inventories in notes or chat.
   When selecting the next cluster, prefer the one whose first diagnostic many
   rows share over a feature header, and land several small independent repairs
   per cycle; the measured per-cycle yield, the current row buckets and the
   cluster order are in `task.md`'s yield analysis.
5. **Only at the end of a large change that needs deeper verification**, run
   the relevant `tests_pedantic.cmd -Group language|gcc|checks|performance` group or the
   complete pedantic entry point. Avoid pedantic sweeps during routine fixes.

Keep each session to one cluster from `task.md`: reproduce, fix, promote to
pedantic, verify with one fast tier run, update the state line, exit. Sessions
are expected to be short and are relaunched fresh; a landed small fix plus an
accurate state line beats a long session that runs out of context. Batch two or
three already-narrow independent fixes in one session, never a second cluster.

Keep compilation and builds serial. Language compiler/program invocations and
fast standalone gates have five-second ceilings. Pedantic standalone gate totals
have explicit longer budgets in `checks.json`. Record timeouts as failures;
never retry with relaxed budgets or silently disable slow checks.

When fixing a retained GCC failure, use an exact selection:
`Tests/pedantic/gcc/run.ps1 -Select g++.dg/template/access27.C -Out build/<run>`.
Preserve its key behavior in a small regression. The default fast tier contains
the unresolved retained GCC cases, including runtime crashes and wrong results,
plus new language regressions. Previously verified passing language and GCC cases
are assigned to pedantic through `Tests/tiers.json`; files stay in place to
preserve includes and provenance. After validating a repair, add its passing paths
to that manifest. Native gates use the tier assignments in `Tests/checks.json`.

`Tests/run-all.ps1 -Tier pedantic` checks established passing language and GCC
cases; `-Tier all` runs both partitions. `-List` shows the selected suites, and
`Tests/pedantic/gcc/run.ps1 -Tier fast|pedantic|all -List` shows exact GCC paths.
Scores cover the selected retained cases, not full GCC conformance. Moving a
passing case to pedantic does not remove its coverage or change its expectations.

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
- `Tests/triage_retained_failures.ps1 -ResultsPath <results.jsonl>` groups the
  retained failures by source area and first diagnostic. It writes compact
  cluster reports to `build/compiler-bug-triage.{txt,json}` and never prints
  the full inventory to the console.
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

When instrumenting compiler source temporarily, redirect stderr to a file in
`build/` (for example `cpc.exe ... 2> build/parser-probe.log`) and remove the
instrumentation before publishing. Keep the compiler identity, result path,
and the concise triage summary as the durable session state; leave full
per-case diagnostics in the `results.jsonl` generated under `build/`.

For Windows CPU sampling, `Build.cmd` writes `build/compiler/cpc.map` for the
published root compiler. Compile `src/tools/profile_process.c` with root CPC
and `-lwinmm`, then run the tool with the map, root compiler, and a CPC response
file. It samples the main thread and its frame-pointer chain (up to 24 frames).
Use separate uninstrumented runs for timings; rebuild the map after compiler edits.
