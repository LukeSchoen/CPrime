# Remaining work

Target: 100% of the retained GCC language rows and the first-party pedantic
corpus, with no missing coverage, weakened expectations, compiler internal
errors or timeout retries.

State (2026-09-12, CL integration wave): 145 retained rows -- 39 PASS_COMPILE,
106 FAIL_COMPILE -- on root `cpc.exe`. Cycle 5 retired the variadic
out-of-class member-template cluster and the line-record collision in the
member-class scan. An out-of-class member-template definition consumed its
`template<>` header, so the pack mark was lost: the replay no longer recovered
the declared pack parameter, and every pack call either failed to deduce or
expanded each element twice. The saved declaration stream also begins with a
line record, and the member-class scan read that record's line number as a
token, so any definition whose line number collided with a token id (`:` is
58, which `clVector3.inl`'s `operator==` hits) was classified as a
namespace-scope template and never defined the member the call reached. Both
repairs are locked by
`features/Templates/pass/test_variadic_static_member_definition_across_inputs.cpp`
(repaired) and
`features/Templates/pass/test_member_template_definition_line_record.cpp`. The
first-party pedantic list is now 11 cases after re-verification removed seven
stale entries. Cycle 4 retired the two-part CL
conversion cluster: a concrete overload the call's argument types associate at
the point of instantiation now owns the call instead of the generic template
whose body then failed, and a saved expression whose `<` follows the
unqualified spelling of a template keeps its argument list instead of taking a
comma-result marker. Cycle 3
retired `g++.dg/cpp1z/eval-order14.C`: the left operand of an assignment whose
form is a parenthesized conditional is saved as tokens, the right operand is
generated first (C++17 [expr.ass]), and the replayed conditional stores that
operand in the arm it selects, so a bit-field arm no longer needs an address
(first-party `features/Expressions/test_conditional_bitfield_assignment.cpp`).
Cycle 2 retired `g++.old-deja/g++.pt/spec36.C`, `g++.dg/abi/rtti2.C` and
`g++.old-deja/g++.other/op2.C` with one overload-ranking repair plus
ABI-shaped pointer type_info descriptors.
The 546 passing external rows were deleted in cycle 1 and their coverage moved
first-party: 41 near-duplicate internal tests merged into 12 files plus the
`features/GnuExtensions` suite (6 tests). `Tests/tiers.json` pedantic is 1573
entries. The fast tier and the retained gate pass with 0 regressions; the
pedantic language partition currently fails 11 first-party cases; the list was
re-verified on 2026-09-12 after the CL integration fixes. The remaining cases are
tracked in `Tests/progress/first-party-failures.txt`, which is authoritative.
Coroutine stage 2 landed: `<coroutine>`, `<experimental/coroutine>`,
`-fcoroutines`, `__cpp_impl_coroutine = 201902L`, `co_await`, `co_yield`,
`co_return`, promise member lowering and `await_transform`; 39 of 40 retained
coroutine rows now pass compile. `pr113457.C` remains blocked on qualified
class-template argument deduction for `ranges::elements_of(...)` after
comma-fold pack expansion.

First workstream: finish the CPC-driven CL/OTServer build. All 206 CL
translation units now compile; the link fails on a single reference to the
deleted `clSocket` copy constructor. See the CL/OTServer workstream below for
the standalone CPrime reproducers.

## Cycle contract

One session = one cluster; a fresh launch beats a long session that runs out of
context.

1. Fix the CL/OTServer build first. Reproduce each remaining CL compile/link
   failure as a standalone CPrime test (never include `CommonLib` or other CL
   headers), fix the compiler, and re-run the CL build. Only after the
   CL/OTServer build passes, pick the next item from the wave plan below.
2. Reproduce one row:
   `Tests/pedantic/gcc/run.ps1 -Select <path> -Out build/<name>`.
3. Reduce it to a first-party regression in the nearest `Tests/features/...`
   suite, fix the compiler, and re-run the selection plus the regression until
   both pass.
4. Retire the row: delete the corpus file, remove its `corpus.json` case entry
   and drop any `Tests/tiers.json` entry for it. Repaired rows are deleted, not
   promoted into a permanent external corpus.
5. Re-run `Tests/run-all.ps1 -Tier fast` once (about 20 s) to confirm zero
   regressions, update the State line, then stop. Repairing one of the
   first-party failures below also means deleting its line from
   `Tests/progress/first-party-failures.txt`; the worker counts the retained
   corpus plus that list, records the rate in `Tests/progress/log.tsv`, and
   stops only when both are empty.

Do not start a second cluster. If a fix has not landed after roughly half the
budget, revert it, record the narrowed lead in the retained GCC README, and
stop. Run focused selections, print only the tail of a run, and never open whole
test files or inventories when a path plus first diagnostic is enough. Do not
paste inventories into this file. Create `done.x` and stop the moment every
retained row and pedantic test passes.

## First workstream: CL/OTServer compilation

The immediate priority is a complete CPC-driven CL/OTServer build. The CL
compile reaches all 206 translation units. Cycle 5 reduced the link from
twelve undefined symbols to one; the four clusters below were all that
remained, and only the last one is unresolved.

Retired with local reproducers:

- `features/Templates/pass/test_variadic_static_member_definition_across_inputs.cpp`
  covered the seven `clString::Format<...>` undefined symbols: a variadic
  static member template defined out of class in a header is instantiated in
  two inputs, and every input now emits the weak definition.
- `clSparseArray2<T>` move constructor and `clVector3<T>::operator==`
  instantiations shared one root cause, `clVector3.inl`'s definition line
  number colliding with the `':'` token id in the saved declaration stream;
  locked by
  `features/Templates/pass/test_member_template_definition_line_record.cpp`.
- `clMatrix4x4<T>::M` const member instantiations no longer appear; the CL
  member-call qualification wave already covered them.

Still open, narrowed to one site:

- A reference to the deleted `clSocket` copy constructor. `clSessionHost` has
  a `clList<clSocket>` member, so merely including `clSession.h` instantiates
  `clList<clSocket>`; the compiler then materializes that class template's
  copy-assignment body (and its `Insert`/`PushBack` helpers) even though no
  input calls them, and those bodies copy their elements through
  `clCopyConstruct`, which names the deleted copy constructor. Minimal shape:
  a class template with an in-class-defined member and an out-of-class member
  whose body copy-assigns an element type with a deleted copy assignment,
  used only as a member of a non-template struct. The demanded-body signal in
  `queue_demanded_template_member_bodies` currently reads a published
  interface (`func_sym->c`) as a call, so a full fix has to distinguish a
  referenced member from a merely published one instead of suppressing the
  deleted-function use.

Standalone coverage already added for the fixable categories:

- `features/Templates/pass/test_class_template_member_definition_across_inputs.cpp`
  locks in linkage of out-of-class class-template member functions across
  inputs.
- `features/Templates/pass/test_member_template_definition_line_record.cpp`
  locks in member-class classification of an out-of-class member-template
  definition whose line record collides with a token id.
- `features/Constructors/pass/test_move_only_copy_deleted_across_inputs.cpp`
  locks in move selection when the copy constructor is deleted.

## Wave plan

145 rows = 40 coroutines + 17 `_Complex` + 88 general language long tail.
Without Waves B and C the ceiling is ~88/145, so both features are required.

### Wave A - general language long tail (after CL/OTServer compilation)

Batch two or three independent repairs per session, in this order:

1. Qualified member-template-id truncation: the saved call argument keeps a
   qualified member template id whole, but rows still truncate and report
   `got '__cpc_comma_result_1'`, so the angle opener is a synthesized
   comma-result token rather than the template name `skip_or_save_block_mode`
   recognises. One root fix retires 7 first-party pedantic failures
   (`features/Includes` 5, `features/Templates` 2) and likely external parse
   rows.
2. GNU-extension leftovers: `c-c++-common/pr105998.c` `(long long)(vector)`
   conversion; `g++.dg/ext/flexary24.C` / `flexary40.C` zero-size flexible
   arrays; `g++.dg/ext/vector29.C` deferred `vector_size` for a using-declared
   qualified dependent constant (`VecSize`, `A<N>::X`; plain `vector_size (N)`
   works).
3. Overload and deduction singletons (9): `call7.C`, `access39.C`, `ttp58.C`,
   `spec4.C`, `mem_fun`, `defaultHandler` and the remaining `no matching
   function template` / `no matching overloaded function` rows.
4. Const and conversion cluster: `test_member_pointer_cv_deduction.cpp`,
   `test_template_member_index_operator_reference.cpp` (both first-party) and
   the 6 `cannot convert` external rows.
5. The narrowed leads recorded under
   [retained GCC checks](Tests/pedantic/gcc/README.md#narrowed-leads), one site
   each: `conv20.C`, `access28.C`, `spec7.C`, the dependent template template
   chain, the indirect virtual base mem-initializer, array decay in a `?:`
   condition, then `conv1.C` (current lead), `synth7.C`, `template25.C`,
   `canon-type-3.C`, `syshdr1.C` and `anon3.C`.

### Wave B - `_Complex` (17 rows, 3 sessions)

1. Type/declarator parsing and imaginary literals for `_Complex` and
   `__complex__`, retiring the `invalid number` and `';' expected (got
   'double'/'__complex__')` clusters.
2. Arithmetic, conversions and the `__real__` / `__imag__` / `conj` builtins.
3. Runtime layout, ABI and the `g++.dg/opt` optimization rows.

### Wave C - coroutines (40 rows, 4 sessions)

1. Ship `<coroutine>` and `<experimental/coroutine>` and accept `-fcoroutines`;
   this single step unblocks 28 rows.
2. Parse `co_await`, `co_yield`, `co_return` and awaitables.
3. Promise machinery and coroutine frame lowering.
4. ABI, symmetric transfer, exceptions and destruction.

## Known first-party pedantic failures (11)

Pre-existing or surfaced by the coroutine wave; they count toward the target.
`Tests/progress/first-party-failures.txt` is the authoritative path list.
CPrime tests that reproduce CL/OTServer failures must stay standalone; do not
include `CommonLib` or other CL headers.

- Classes 2
- Constructors 1
- Declarations 1
- Exceptions 1
- Expressions 3
- Includes 1
- Templates 2

Seven of the previous entries were re-verified as already passing on the
cycle-4 compiler (six Constructors braced-initializer cases and
`test_member_template_binary_operator_same_type_fallback`), and cycle 5
repaired `test_variadic_static_member_definition_across_inputs.cpp`.  Earlier
repairs: `test_default_argument_array_element_reuse.cpp` (the CL `CMD.cpp:133`
conversion failure) closed with the default-argument parameter scope keeping
the nodes the parsed argument still reads.  The comma-result marker and
angle-opener pair retired `test_chrono_clocks.cpp`,
`test_map_balanced_operations.cpp`, `test_unordered_map_operations.cpp`,
`test_unordered_set_operations.cpp`,
`test_qualified_alias_functional_construction.cpp` and
`test_static_string_array_before_hashmap.cpp`: a saved expression whose `<`
follows the unqualified spelling of a template keeps its argument list, and an
unqualified call in a template body still reaches the concrete overload its
argument types associate at the point of instantiation.

Resolved work, per-cycle history and yield analysis live in git history and
[the development loop](Tests/DEVELOPMENT.md).
