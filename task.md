# Remaining work

Target: 100% of fast tests and the retained pedantic GCC checks, with no missing
coverage, weakened expectations, compiler internal errors, or timeout retries.

State (2026-09-11): 238 retained failures of 238 selected rows -- 221
FAIL_COMPILE, 11 FAIL_RUN, 6 FAIL_RUN_CRASH -- on root `cpc.exe` SHA256
`40a14ce74b47e21871dc7c26f8a29e137b49379ae19d03355c21ffd020b2d1ec`. All
fourteen selected fast language suites pass
(`build/pedantic-gcc-1c9ec3728b3e45f38e84291fc9a4fd1d`); the promoted pedantic
GCC set scores 460/460 (`build/ped-after-va-arg-pack-final`). Triage is
`build/compiler-bug-triage.txt` with JSON beside it. The 22 known local
language-suite failures are listed below.

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

1. Builtins: the legacy `__sync_*` family, the C++ allocation builtins and the
   same-function jump builtins landed. `__builtin_setjmp`/`__builtin_longjmp`
   save the frame in a runtime helper instead of wrapping the CRT `setjmp`,
   whose x86-64 `jmp_buf` is larger than GCC's `void *[5]` contract.
   `__builtin_va_arg_pack`/`_len` now expand variadic always-inline bodies at
   the call site. Remaining: `__builtin_[dynamic_]object_size` 6.
2. Runtime themes: copy elision 4 (`nrv23`/`nrv24`, `eh/return1`,
   `init/elide1`), two-stage lookup 2 (`lookup/template1.C`,
   `lookup/two-stage1.C`), virtual bases 3.
3. Long tail: mostly one row per fix in template deduction, substitution,
   dependent lookup and overload selection. Batch two or three small
   independent fixes in a session instead of one deep defect.
4. Coroutines (~30 rows) and `_Complex` (~13 rows) are whole features with no
   historical yield. Start them only after an explicit decision to fund them as
   multi-cycle projects.

Narrowed leads, each already reduced to one site:

- `__builtin_[dynamic_]object_size` and the FAM `bos` rows are one whole
  feature (object/subobject chain, malloc/alloc_size extent, single-definition
  pointer locals, four unknown sentinels); fund it beside coroutines.
- `c-c++-common/builtin_location.c`: only `e0..e3 = __FILE__ - __FILE__`
  fails, because `gen_opic` cancels a relocation difference only for one
  symbol; equal-content string literals need merging before that fold.
- GNU vectors: braced/compound-literal init, element subscript, element-wise
  arithmetic/logic/comparison and the convert/shuffle builtins are done. Two
  rows remain: `g++.dg/ext/vector29.C` needs deferred `vector_size` for a
  using-declared/qualified dependent constant (`VecSize`, `A<N>::X`; plain
  `vector_size (N)` works), and `c-c++-common/pr105998.c` needs
  `(long long) (vector)` conversion.
- `g++.dg/ext/is_base_of_incomplete.C`: the trait's type argument materializes
  the class template; fix where `cpp_type_trait_name_tok` calls `parse_type`.
- `g++.dg/template/ptrmem3.C` and `g++.old-deja/g++.pt/ptrmem4.C`: overloaded
  member address whose target member-pointer shape is unbound; fix where the
  contextual member-pointer type is established, not in the candidate matcher.
- `g++.cpp1y/vla10.C` and `g++.dg/opt/pr78201.C`: initialized VLAs with a
  non-constant bound need GNU's initialized-VLA extension in
  `decl_initializer`.
- `g++.dg/opt/{nrv23,nrv24}.C`, `g++.dg/eh/return1.C`, `g++.dg/init/elide1.C`:
  `return <named local>;` and class-typed initializers from a call still emit a
  copy-constructor reference the tests leave undefined.

## Known pedantic language-suite failures (22)

Pre-existing, confirmed against earlier published compilers, so not regressions
from retained-corpus repairs. They count toward the target.

- `features/All` 1: `test_windows_runtime_compatibility.cpp` (compiler crash).
- `features/Classes` 2: `test_member_default_arg_after_overloaded_constructors.cpp`
  (duplicate static member template definition);
  `test_out_of_class_member_nested_range_for.cpp` (const `Owner_Item *`).
- `features/Expressions` 2: `test_conditional_class_conversions.cpp` and
  `test_functional_conversion_operator.cpp` (runtime exit 1).
- `features/Includes` 2: `test_chrono_clocks.cpp` (compiler crash);
  `test_cstdio_function_identity.cpp` (undefined `__cpc_ns_std_fclose`).
- `features/StdConcurrency` 10: all compiler crashes.
- `features/Templates` 5: three crash
  (`test_integral_partial_specialization.cpp`,
  `test_nested_pointer_alias_deferred_class.cpp`,
  `test_numeric_partial_in_namespace.cpp`),
  `test_member_pointer_cv_deduction.cpp` (`->*` on a const member pointer) and
  `test_template_member_index_operator_reference.cpp` (const `Vec2<float> *`).

## Other workstreams

- OTServ Release build under 30 s; project root
  `C:\Luke\Src\CL\CommonLib\commonLib\include\Game\OT\Server` (its build trees
  under `build/` were removed, so re-run the external project build).
- Placement-delete unwinding for variadic allocation; constexpr evaluation of
  local objects and control flow; GNU asm constraints; Yasm quoted-symbol GAS
  parser limit; portable package at 1,000,000 bytes.
- GCC adapter: diagnostic matching, standard/target selection, extra sources,
  output and assembly expectations.

Resolved work, per-cycle history and the yield analysis live in git history and
[the development loop](Tests/DEVELOPMENT.md). Keep this file under ~100 lines.
