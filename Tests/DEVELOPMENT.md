# Development loop

Remaining work and the current state line live in [task.md](../task.md). Keep
that file short and current; git history and the tests themselves are the
record of completed work.

Use root `cpc.exe` for every compile, build and regression step. Do not invoke
Clang, GCC, MSVC or another compiler for host builds, comparisons or benchmarks
without an explicit user request; a CPC failure is a bug to reproduce, not a
reason to switch hosts. Compile and build serially, one compiler process at a
time. Language compiler/program invocations and fast gates have five-second
ceilings; record timeouts as failures and never retry with a relaxed budget.

## One session, one cluster

Pick the next item from the wave plan in `task.md` (Wave A while the language
long tail lasts; Waves B and C need a funding decision), or the top cluster in
`build/compiler-bug-triage.txt` when the ordered list is empty. Reproduce,
repair and verify that cluster only. If a fix has not landed after roughly half
the session budget, revert it, record the narrowed lead in `task.md`, and stop.

### Repairing a retained GCC row

1. Reproduce the row exactly:
   `Tests/pedantic/gcc/run.ps1 -Select <path> -Out build/<name>`.
2. Reduce the behavior to a small first-party regression in the nearest
   `Tests/features/...` suite, with every required fixture inside CPrime.
3. Fix the compiler and re-run the exact selection plus the first-party
   regression until both pass.
4. Retire the row: delete its corpus file, remove its `corpus.json` case entry,
   and drop any `Tests/tiers.json` entry for it. A repaired row must not stay in
   the corpus as a permanent external dependency.
5. Re-run the retained gate and the fast tier, then update the `task.md` state
   line and stop.

### Repairing a first-party regression

1. Keep the smallest reproducing test in the relevant suite; reuse helpers and
   remove duplicate fixtures without losing distinct behavior.
2. Rebuild with `Build.cmd` when compiler sources change, then run the exact
   case with `Tests/run.ps1 -Suite <suite> -Select <test>`.
3. Run the other established cases that exercise the changed behavior. Explicit
   `-Select` crosses tiers unless `-Tier` restricts it.
4. Run `Tests/run-all.ps1` for the active verification and any selected
   CPC-only native gates, then update `task.md` and stop.

## Verification tiers

- `Tests/run-all.ps1 -Tier fast` is the routine check: unresolved retained GCC
  rows (expected to fail until repaired) plus new first-party regressions.
- `Tests/run-all.ps1 -Tier pedantic` is the established-pass guard: the
  first-party regression corpus. Run it after large changes or consolidation,
  not after every edit.
- `Tests/pedantic/gcc/run.ps1` defaults to the retained rows. `-Tier pedantic`
  is an empty no-op while every retained row is unresolved; `-Tier all` selects
  the same rows as the default.

`Tests/run-checks.ps1` adds fast CPC-only native/integration gates.
`tests.cmd`, `-IncludeChecks` and `tests_pedantic.cmd` can invoke cross-compiler
ABI gates and need explicit authorization for those invocations.

## Progress log

`worker.cmd` prints test completion: cases left, the rate in real hours and
days, and the time that rate implies. One sample per finished cycle is appended
to `Tests/progress/log.tsv`, a tab-separated, append-only file that git commits
together with the work the sample describes, so stopping and restarting the
worker needs no recovery step. The baseline is the consolidation commit that
created the retained failure corpus; earlier rows are not comparable.

`build/worker-status.exe` (from `src/tools/worker_status.c`) owns the format and
the arithmetic:

```powershell
./cpc.exe -o build/worker-status.exe src/tools/worker_status.c
./build/worker-status.exe --root .                    # completion report
./build/worker-status.exe --root . --last-cycle       # resume the cycle counter
```

Remaining work is the retained `corpus.json` cases plus the paths listed in
`Tests/progress/first-party-failures.txt`. Retiring a first-party failure means
deleting its line there as well as in `task.md`; the worker reports 100% and
stops only when both lists are empty. `test_WorkerProgress.ps1` covers the log
format, the rate arithmetic and the failure exits.

## Consolidation policy

Prefer one focused test per distinct behavior, and combine tests that differ
only in call syntax or receiver shape into one file with named cases. Deleting
coverage, weakening expectations, or turning a reproducible failure into an
expected failure is not consolidation. External rows are retired by deletion
after the behavior is covered first-party, never promoted into the corpus.

Keep tests minimal, deterministic and fast; keep reproducing sources and
fixtures in `Tests/`, and generated output in `build/`. Regression reports carry
only the repository-relative test path, the exact command, compiler/runtime
identity, expected versus actual behavior, and the runner summary with timing.

## Tools

Prefer C sources compiled with root `cpc.exe` for new build drivers and helpers;
avoid new PowerShell implementations unless a native approach is impractical or
measurably slower. Keep tool sources in `src/` and generated executables in
`build/`.

- `Tests/diagnose.ps1 -Source <repro.cpp>` captures commands, preprocessing,
  hashes and diagnostics; `CPRIME_PARSER_STATE=1` enables parser tracing.
- `Tests/triage_retained_failures.ps1 [-ResultsPath <results.jsonl>]` groups
  retained failures by area and first diagnostic into
  `build/compiler-bug-triage.{txt,json}`. It defaults to the newest
  `build/pedantic-gcc-*/results.jsonl`.
- `Tests/check_regressions.ps1` is the compiler publication gate.
- `Tests/run-checks.ps1 -Select <name>` runs one fast native/integration gate.
- Runner or tier changes: `Tests/test_RunnerFixtures.ps1`,
  `test_SuiteRunner.ps1`, `test_CheckTiers.ps1`. GCC adapter changes also need
  its focused `test_*.ps1` checks, including corpus integrity.
- Build dispatch changes: `check_build_default.ps1`,
  `check_build_regression_gate.ps1`. Project cache changes: fast
  `run-checks.ps1 -Select test_IncrementalCache`, plus `check_native_project`,
  `check_incremental_build` and `check_batch_build` for graph, dependency or
  process changes.

Redirect temporary instrumentation output to `build/` and remove the
instrumentation before publishing. Keep the concise triage summary as durable
session state; leave full per-case diagnostics in the generated `results.jsonl`.
