# Task: finish the C++17 queue

Autonomous C/C++ compiler engineer in `C:\Luke\Src\CPrime`. Root `cpc.exe` is
the only compiler, one process at a time.

## Read first

1. `AGENTS.md` - binding rules. Follow it exactly; it is not repeated here.
2. `Tests/CPP17-REMAINING.md` - the only open-work list. It owns the gaps,
   reproducer shapes, and the fixed-but-unretained backlog.

Those two files plus `Tests/DEVELOPMENT.md` are the whole brief. Do not restate
them, and do not keep status here: completed work is code, and git is the record
of what changed.

## Loop

- Baseline: `Tests\test.exe -All -Tier fast` plus the suites you will touch.
- Per gap: minimal reproducer in `build/` -> reproduce with root `cpc.exe` ->
  repair the shared mechanism -> retain one case in `Tests/features/...` ->
  run that case, then the suite -> delete the scratch reproducer.
- Boundaries: fast tier; pedantic tier and `Tests\test.exe -Regression` before
  publication; `scripts\build.exe` to publish.

## Order for speed

- Batch by mechanism, not by file. The five constexpr object-model gaps share
  the constant evaluator, and the two pack-expansion gaps unblock `make_tuple`,
  `tie` and `apply` together. Fix a mechanism once, then cover every gap it
  explains.
- Highest leverage first: pack expansion, then constexpr object model, then
  libraries, then the two diagnostics.
- The retained-case backlog needs no compiler change. Do it first, from the
  existing suites, and add only what is genuinely missing.
- Reuse an existing case when it already proves the behavior. If inspection
  shows a mechanism is already correct and covered, close the gap and move on.
- Failures stay failures: no expected-failure relabelling, no name-specific
  hacks, revert failed experiments.

## Done

- Every queue item fixed with a retained case, or blocked with the exact
  command and evidence.
- Pedantic tier, `-Regression`, and `scripts\build.exe` green; root `cpc.exe`
  still a working published compiler.
- Tree holds only intended source, test, and queue updates. Report files
  changed, mechanisms fixed, exact commands run, and any blockers.

## Next steps

Verified against root `cpc.exe` on 2026-09-16: fast is red by exactly the
cases in `Tests/tiers.json` - 20 `features/Cpp17Gaps` cases and one
`features/CompilerCrash` case. Follow the ordered completion plan in
`Tests/CPP17-REMAINING.md`; it owns the current mechanism grouping and case
list.

1. Baseline the affected suite and `Tests\test.exe -All -Tier fast`.
2. Fix the structured-binding selection-initializer crash first.
3. Fill the type-traits foundation, then finish the remaining language
   semantics.
4. Work the runtime/library groups in dependency order: core surfaces,
   algorithms/utilities, conversion-heavy numerics, containers/memory/variant,
   then filesystem.
5. Per closed case: run the selected case, run the suite, delete the scratch
   reproducer, then remove that case from the `fast` list.
6. Boundary: affected suites plus `Tests\test.exe -Regression`, then
   `scripts\build.exe` to publish. Pedantic only as a last confirmation.

RTMPose performance context and the two external CPC bug reports are recorded
in `KNOWN-ISSUES.md`.
