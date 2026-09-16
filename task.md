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
cases in `Tests/tiers.json`. Details and reproducer shapes live in
`Tests/CPP17-REMAINING.md`; this is the order to work them in.

1. Baseline `Tests\test.exe -All -Tier fast` plus the suite you will touch.
2. Constexpr object model: both cases share the constant evaluator, so fix the
   mechanism once and cover both.
   `test_constexpr_user_provided_constructor.cpp` first: a local constexpr
   object of a class type with a user-provided constructor is not
   constant-evaluated, which is also what blocks `std::optional` below.
   Then `test_constexpr_reference_member.cpp`: a reference member read has to
   keep referring to the referent object.
   Re-check `fail/test_constexpr_constructor_member_order.cpp` afterwards - it
   must still be rejected, now for declaration order.
3. Libraries: `test_optional_constexpr.cpp` should fall out of step 2;
   `test_optional_copy_constructible_trait.cpp` needs the storage union whose
   copy operation is defaulted behind a conditionally deleted base.
4. `test_function_template_address_argument.cpp` fails with `no matching
   function template 'run_char'`; it is listed in `fast` now and needs a fix.
5. Per closed case: run the selected case, then the suite, then remove that
   case from the `fast` list in `Tests/tiers.json` once it passes. Delete
   scratch reproducers from `build/` as each durable case lands.
6. Boundary: affected suites plus `Tests\test.exe -Regression`, then
   `scripts\build.exe` to publish. Pedantic only as a last confirmation.
