# Remaining work

Target: 100% of the retained GCC language rows and the first-party pedantic
corpus, with no missing coverage, weakened expectations, compiler internal
errors or timeout retries.

State (2026-09-12, focus wave): 152 retained failures of 152 retained external
rows -- 146 FAIL_COMPILE, 3 FAIL_RUN, 3 FAIL_RUN_CRASH -- on root `cpc.exe`
SHA256 `b6e16ca86ae7a6965b840c3f00dabec3f15c84f06ba5af4098e4a4e7e17ad489`.
The 546 passing external rows were deleted; `corpus.json` lists only unresolved
rows (698 -> 152) and `-Tier pedantic` for the GCC adapter is now an empty no-op.
Their coverage moved first-party: 41 near-duplicate internal tests were merged
into 12 consolidated files and the new `features/GnuExtensions` suite carries
the retired GNU-extension behaviors. `Tests/tiers.json` pedantic is 1567 entries
(1561 internal established passes and 6 GnuExtensions). The fast tier passes
with 0 regressions and the pedantic language partition fails only the 16 known
cases below; triage is current as of this wave
(`build/compiler-bug-triage.txt`).

## Cycle contract

One session = one cluster. Sessions are expected to be short; a fresh launch is
cheaper than a long one that runs out of context.

1. Pick the next cluster from the ordered work below, or the top cluster in the
   triage when that list is empty.
2. Reproduce one row with
   `Tests/pedantic/gcc/run.ps1 -Select <path> -Out build/<name>`.
3. Reduce it to a first-party regression in the nearest `Tests/features/...`
   suite, fix the compiler, and re-run the selection plus the regression until
   both pass.
4. Retire the row: delete the corpus file, remove its `corpus.json` case entry
   and drop any `Tests/tiers.json` entry for it. Repaired rows are deleted, not
   promoted into a permanent external corpus.
5. Re-run `Tests/run-all.ps1 -Tier fast` once (about 20 s) to confirm zero
   regressions, update the State line and the ordered work, then stop.

Do not start a second cluster. If a fix has not landed after roughly half the
budget, revert it, record the narrowed lead in the ordered work, and stop.
Keep the session small: run focused selections rather than whole sweeps, print
only the tail of a run, and never open whole test files or inventories when a
path plus first diagnostic is enough. Do not paste inventories into this file.

Create `done.x` and stop the moment every retained row and pedantic test passes.

## Ordered work

1. Long tail: 113 singleton rows plus small shared clusters in template
   deduction, substitution, dependent lookup, conversion and overload
   selection. Batch two or three small independent fixes per session instead of
   one deep defect. Cycles 24-50 retired the clusters recorded in git history,
   most recently `friend23.C`, `typedef15.C`, `crash53.C`, `local11.C`,
   `friend73.C`, `inherit.C`, `friend49.C`, `qualttp16.C`, `member3.C`,
   `injected2.C` and `sfinae27.C`; cycles 51-53 retired `spec20.C`,
   `sts_partial.C`, `dr408.C`, `deduce8.C`, `typename1.C`, `dr1391-1.C` and
   `conv6.C`; current lead: `conv1.C` (`First__D_operator` undeclared), then
   `conv20.C` (incomplete base).
2. Coroutines: 30 rows in three header/feature clusters. `_Complex`: 5 immediate
   rows plus the remaining `g++.dg/opt` rows. Both are whole features with no
   historical yield; start only after an explicit decision to fund them as
   multi-cycle projects.

Narrowed leads, each already reduced to one site:

- An object declaration materializes its class template specialization even
  where completeness is not required, so `extern B<int> b;` reports `base
  class 'A__int' is incomplete` for a `B` derived from an incomplete `A`;
  `conv20.C` therefore never reaches the arity short-circuit it tests.
- Template template argument through a dependent chain: `T::template AA<U>::template B`
  now names the nested template, but the enclosing argument is lost, so
  `chain<outer<char>, char>` still builds `middle<int>::inner` (probe: `sizeof`
  of the chain type stays 4 where the `typename` spelling gives 1).
- Remaining `'>' expected after template argument` rows (the std map spellings in
  `features/Includes` and `features/Templates`): the saved call argument now keeps
  a qualified member template id whole, but these rows still truncate and report
  `got '__cpc_comma_result_1'`, so the angle opener is a synthesized comma-result
  token rather than the template name `skip_or_save_block_mode` recognises.
- GNU vectors: `g++.dg/ext/vector29.C` needs deferred `vector_size` for a
  using-declared/qualified dependent constant (`VecSize`, `A<N>::X`; plain
  `vector_size (N)` works), and `c-c++-common/pr105998.c` needs
  `(long long) (vector)` conversion.
- Template parameter hiding: cycle 24 covers plain non-dependent bases; a
  dependent base can still substitute the argument, and probing it must avoid
  recursively instantiating the current specialization (`access28.C`; cycle 52
  re-checked that its first diagnostic matches the previous compiler).
- Member function template of a member class template's specialization:
  `template<> template<> template<class V> void A<int>::B<char>::g(V) { }` is
  still a namespace-scope template, so a linked call to `g` is an undefined
  symbol (`spec7.C` is compile-only; probe: call `g` on `A<int>::B<char>`).
- Virtual bases: cycle 17 shipped the base-object/complete split (`*__base`
  variants), but a mem-initializer naming an *indirect* virtual base is still
  dropped (`skip_initializer_emit`) and gets default-initialized instead.
- Array decay: a static member array compared against a pointer where the
  comparison opens a statement or `?:` reports `invalid operand types for
  binary operation` (`return X::p == X::c ? 0 : 1;`); `if (!(...))` works.

## Known first-party pedantic failures (16)

Pre-existing, confirmed against earlier published compilers, so not regressions
from the retained-corpus repairs. They count toward the target.

- `features/Classes` 2: `test_member_default_arg_after_overloaded_constructors.cpp`
  (duplicate static member template definition),
  `test_out_of_class_member_nested_range_for.cpp` (const `Owner_Item *`).
- `features/Constructors` 1: `test_implicit_return_move_and_cv_rvalue_binding.cpp`
  (exit 2).
- `features/Exceptions` 1: `test_value_parameter_destruction.cpp` (exit 1).
- `features/Expressions` 3: `test_conditional_class_conversions.cpp`,
  `test_functional_conversion_operator.cpp` (exit 1),
  `test_lambda_captured_object_direct_construction.cpp` (exit 9).
- `features/Includes` 5: `test_chrono_clocks.cpp` (crash),
  `test_cstdio_function_identity.cpp` (undefined `__cpc_ns_std_fclose`),
  `test_{map_balanced,unordered_map,unordered_set}_operations.cpp` (`'>' expected
  after template argument` on the std map spellings).
- `features/Templates` 4: `test_qualified_alias_functional_construction.cpp`
  and `test_static_string_array_before_hashmap.cpp` (`'>' expected after
  template argument`), `test_member_pointer_cv_deduction.cpp` (`->*` on a const
  member pointer), `test_template_member_index_operator_reference.cpp` (const
  `Vec2<float> *`).

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
