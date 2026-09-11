# Remaining work

Target: 100% of fast tests and the retained pedantic GCC checks, with no missing
coverage, weakened expectations, compiler internal errors, or timeout retries.

State (2026-09-12): 197 retained failures of 197 selected rows -- 188
FAIL_COMPILE, 4 FAIL_RUN, 5 FAIL_RUN_CRASH -- on root `cpc.exe` SHA256
`5489c713259858de47620e78a064c9e9c23bf62263e1e83a433a5eabfa680ea2`. All fifteen
selected fast language suites pass (`Tests/run-all.ps1 -Tier fast`);
the promoted pedantic GCC set scores 501/501 (`build/cycle24-pedantic-gcc`);
the pedantic language partition still fails exactly its 30 known cases; triage is
current as of cycle 24 (`build/compiler-bug-triage.txt`). Cycle 24 promoted
`g++.dg/lookup/template3.C` and `g++.dg/lookup/hidden-class14.C`; the 30
known local language-suite failures are listed below and are unchanged.

## Cycle contract

One session = one cluster. Sessions are expected to be short; a fresh launch is
cheaper than a long one that runs out of context.

1. Pick the next cluster from the ordered work below, or the top cluster in the
   triage when that list is empty.
2. Reproduce one row with
   `Tests/pedantic/gcc/run.ps1 -Select <path> -Out build/<name>`, fix it, and
   re-run that selection until it passes.
3. Promote verified passes to pedantic in `Tests/tiers.json`.
4. Re-run `Tests/run-all.ps1 -Tier fast` once (about 13 s) to confirm zero
   regressions.
5. Update the State line and the ordered work in this file, then stop.

Do not start a second cluster. If a fix has not landed after roughly half the
budget, revert it, record the narrowed lead in the ordered work, and stop.
Keep the session small: run focused selections rather than whole sweeps, print
only the tail of a run, and never open whole test files or inventories when a
path plus first diagnostic is enough. Do not paste inventories into this file.

Create `done.x` and stop the moment all fast tests and retained pedantic checks
pass.

## Ordered work

1. Long tail: mostly one row per fix in template deduction, substitution,
   dependent lookup and overload selection. Batch two or three small
   independent fixes in a session instead of one deep defect.
   Cycle 24 retired the inherited-member/template-parameter hiding pair: a
   class-scope member name now hides an enclosing template parameter of the
   same spelling (`g++.dg/lookup/template3.C`,
   `g++.dg/lookup/hidden-class14.C`).
   Current lead: the remaining single-row `g++.dg/lookup` (`pr87531.C`,
   `two-stage3.C`, `two-stage5.C`) and `g++.old-deja/g++.ns` (`koenig7.C`) rows.
2. Coroutines (~30 rows) and `_Complex` (~13 rows) are whole features with no
   historical yield. Start them only after an explicit decision to fund them as
   multi-cycle projects.

Narrowed leads, each already reduced to one site:

- GNU vectors: `g++.dg/ext/vector29.C` needs deferred `vector_size` for a
  using-declared/qualified dependent constant (`VecSize`, `A<N>::X`; plain
  `vector_size (N)` works), and `c-c++-common/pr105998.c` needs
  `(long long) (vector)` conversion.
- Template parameter hiding: cycle 24 covers plain non-dependent bases; a
  dependent base can still substitute the argument, and probing it must avoid
  recursively instantiating the current specialization (`access28.C`).
- Virtual bases: cycle 17 shipped the base-object/complete split (`*__base`
  variants), but a mem-initializer naming an *indirect* virtual base is still
  dropped (`skip_initializer_emit`) and gets default-initialized instead.

## Known pedantic language-suite failures (30)

Pre-existing, confirmed against earlier published compilers, so not regressions
from the retained-corpus repairs. They count toward the target.

- `features/All` 1: `test_windows_runtime_compatibility.cpp` (crash).
- `features/Classes` 2: `test_member_default_arg_after_overloaded_constructors.cpp`
  (duplicate static member template definition),
  `test_out_of_class_member_nested_range_for.cpp` (const `Owner_Item *`).
- `features/Constructors` 2: `test_implicit_return_move_and_cv_rvalue_binding.cpp`
  (exit 2), `test_value_parameter_destruction.cpp` (exit 1).
- `features/Exceptions` 1 (exit 9): `test_lambda_captured_object_direct_construction.cpp`.
- `features/Expressions` 2 (exit 1): `test_conditional_class_conversions.cpp`,
  `test_functional_conversion_operator.cpp`.
- `features/Includes` 5: `test_chrono_clocks.cpp` (crash),
  `test_cstdio_function_identity.cpp` (undefined `__cpc_ns_std_fclose`),
  `test_{map_balanced,unordered_map,unordered_set}_operations.cpp` (`'>' expected
  after template argument` on the std map spellings).
- `features/StdConcurrency` 10: all compiler crashes.
- `features/Templates` 7: `test_qualified_alias_functional_construction.cpp`
  and `test_static_string_array_before_hashmap.cpp` (`'>' expected after
  template argument`), three crash (`test_integral_partial_specialization.cpp`,
  `test_nested_pointer_alias_deferred_class.cpp`,
  `test_numeric_partial_in_namespace.cpp`),
  `test_member_pointer_cv_deduction.cpp` (`->*` on a const member pointer),
  `test_template_member_index_operator_reference.cpp` (const `Vec2<float> *`).

## Other workstreams

- OTServ Release build under 30 s; project root
  `C:\Luke\Src\CL\CommonLib\commonLib\include\Game\OT\Server` (re-run its
  external build; its `build/` trees were removed).
- Placement-delete unwinding for variadic allocation; constexpr evaluation of
  local objects and control flow; GNU asm constraints; Yasm quoted-symbol GAS
  parser limit; portable package at 1,000,000 bytes; GCC adapter diagnostic
  matching, standard/target selection, extra sources, output and assembly.

Resolved work, per-cycle history and the yield analysis live in git history and
[the development loop](Tests/DEVELOPMENT.md). Keep this file under ~100 lines.
