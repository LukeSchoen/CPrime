# Remaining work

Target: 100% of fast tests and the retained pedantic GCC checks, with no missing
coverage, weakened expectations, compiler internal errors, or timeout retries.

State (2026-09-12): 175 retained failures of 175 selected rows -- 167
FAIL_COMPILE, 4 FAIL_RUN, 4 FAIL_RUN_CRASH -- on root `cpc.exe` SHA256
`708137d879bdc37d78bb516735afdc1758fc6344d9035dd4cf063e8d11429e79`. All fifteen
selected fast language suites pass (`Tests/run-all.ps1 -Tier fast`, 0
regressions); the cycle-38 promotion (`g++.dg/template/mem-partial1.C`) passes;
the pedantic GCC partition passes 522/522 and the language partition fails the
16 known cases below; triage is current as of cycle 38
(`build/compiler-bug-triage.txt`). Cycle 38 retired the member class template
spelled with an explicit argument list: a saved initializer keeps the whole
list instead of stopping at its comma, and the injected member class name with
explicit arguments inside the primary member's own body names the template, so
`inner<T,int>::N` selects the partial specialization and `mem-partial1.C`
joins the already-passing `memclass5.C`. Its regressions are
`features/Templates/pass/test_nested_member_template_two_argument_id_in_global_initializer.cpp`
and `test_member_template_specialization_named_in_own_body.cpp`.

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
   Cycles 24-38 retired the clusters recorded in git history, most recently the
   recursive class-typedef lookup (`recurse4.C`), the invalid substituted
   partial-specialization argument (`pr51385.C`), the dependent-base
   class-template registration (`overload14.C`, `template36.C`), the nested
   template-name template argument (`qualttp13.C`), the dependent elaborated
   nested tag (`typename27.C`, `union1.C`), the array static-member template
   argument (`qualified-id2.C`), the member class template partial
   specialization used in the class body (`partial14.C`), the member-template
   address template argument (`ptrmem5.C`, `template-id-4.C`), the result type
   naming a parameterless function template (`partial9.C`) and the member class
   templates completed from an explicit argument list (`spec7.C`,
   `mem-partial1.C`).
   Current lead: `g++.dg/template/local8.C`, the first single-row
   `g++.dg/template` cluster in the regenerated triage (`'kCapacity'
   undeclared`): the constructor of a local class inside a function template
   body cannot read the enclosing function's local constant from its
   mem-initializer.
2. Coroutines (~30 rows) and `_Complex` (~13 rows) are whole features with no
   historical yield. Start them only after an explicit decision to fund them as
   multi-cycle projects.

Narrowed leads, each already reduced to one site:

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
  recursively instantiating the current specialization (`access28.C`).
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

## Known pedantic language-suite failures (16)

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
