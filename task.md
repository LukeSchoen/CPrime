# Remaining work

Target: 100% of fast tests and the retained pedantic GCC checks, with no missing
coverage, weakened expectations, compiler internal errors, or timeout retries.

Current retained state: 380 unresolved retained GCC rows (357 FAIL_COMPILE,
14 FAIL_RUN, 9 FAIL_RUN_CRASH) of 382 selected on compiler SHA256
`5F4A1115ADE4CA03603C2616050910A9E1B74FCD26B224CF407E579725523405` (result
file `build/pedantic-gcc-enum7-fast/results.jsonl`; the previous fast-tier file
is kept as `build/pedantic-gcc-prev-fast.jsonl` for the zero-regression
compare).
The compact cluster report is `build/compiler-bug-triage.txt` (machine-readable
paths in `build/compiler-bug-triage.json`; 25 clusters, largest is
`g++.dg/coroutines` include-file gaps x24).

The retained corpus is the runner's *fast* tier over `pedantic/gcc/corpus`
(`Tests/run-all.ps1` invokes it that way); `Tests/tiers.json`'s pedantic list is
  the promoted set and currently scores 315/315
  (`build/pedantic-gcc-promote`). Triage must be fed a fast-tier results file,
not a pedantic-tier one, or it trivially reports zero unresolved rows.

This work is time-boxed to 15 minutes total; solve as much as possible within
that time and then stop, do not keep looping past the budget. Work without
stopping after individual repairs or asking for approval: regenerate the
triage with `Tests/triage_retained_failures.ps1`, choose a cluster that shares
one first diagnostic in one source area, fix it, verify it, promote verified
passes to pedantic in `Tests/tiers.json`, then continue immediately with the
next cluster until the 15-minute budget is spent. At the 15-minute mark, stop
regardless of remaining clusters and report what was completed, what remains,
and any blocker encountered. If before the budget is spent you become convinced
that all fast tests and retained pedantic checks are passing, create an empty
`done.x` file in the repository root and stop; `worker.cmd` watches for that
marker and will not start another cycle.

Full inventories stay in the generated result/triage files under `build/`; do
not paste them into task notes or chat. Verified passes are assigned to
pedantic in `Tests/tiers.json`; `Tests/run-all.ps1` then focuses on the
unresolved set. Keep all failing cases active, including runtime crashes and
wrong results.

Prefer clusters with two to five tests sharing one diagnostic and source area
over broad feature headers (for example coroutines or `_Complex`) until each
cluster is reduced to a small local regression.

## Compiler and runtime

- Bring the uncached, serial OTServ Release build below 30 seconds. Measure the
  complete native wrapper with `-Unity -Configuration Release -Rebuild`, including
  preparation and linking; keep all 215 sources and use root `cpc.exe`. The target
  remains unverified with the retained compiler changes. Continue profiling
  template deduction and constructor-conversion probes; record machine load and
  compiler identity alongside elapsed and CPU time in `build/`.
  The OTServ project path and native build entry point are still needed.

- Complete virtual-base construction/destruction semantics and the remaining
  retained GCC runtime failures. Select exact failures from the current inventory
  and retain small local regressions alongside each repair.
- Complete placement-delete unwinding for variadic allocation functions and
  audit class-valued placement arguments for copy and lifetime semantics.
- Complete template deduction, substitution failure, specialization, dependent
  lookup, and declaration-only member calls in the remaining retained cases;
  retain the local members/replay/lookup/substitution gates.
- Complete overloaded function selection and hidden-name lookup in the
  retained GCC cases.
  Include callable conversion candidates with differing parameter lists and
  competition between surrogate function calls and member call operators.
- Complete constexpr evaluation of local object values, assignments, and
  control flow; preserve scoped bindings, side effects, and constant-expression
  rejection checks while extending parameterized-function evaluation.
- Implement coroutine language/runtime support and `<coroutine>`; complete
  missing standard-library facilities exercised through `<tuple>`, `<optional>`,
  and `<bitset>`.
- Complete GNU compatibility exercised by the suite: statement expressions,
  inline assembly/asm-goto constraints, vector operations, `_Complex`, builtins,
  and attributes. Verify target and ABI applicability before implementation.
  Complete canonical Unicode identifier identity across UTF-8 and equivalent
  universal-character-name spellings, preserving preprocessing spelling.
- Resolve the bundled Yasm GAS parser limitation in `Tests/test_AsmOutput.cmd`:
  it rejects quoted Microsoft C++ symbol names. Preserve exact symbol identity
  and all CPC/Yasm round-trip expectations; leave third-party tools unchanged.
  Keep object/link/native ABI gates alongside language coverage.

## Test coverage and usability

- Extend the GCC adapter with tested diagnostic matching, standard/target
  selection, extra-source and specialized-driver support, and output/assembly
  expectations. Keep unsupported cases visible until their results can be judged.
- Audit legacy HeapList/Perf fixtures for missing declarations and private
  class access before assigning failures to CPC; retain their intended runtime
  coverage and supply standalone, valid reproducers.
- Keep fast gates within their enforced five-second budget; deeper checks are
  explicit pedantic work after large changes.
- Reach a portable package size of 1,000,000 bytes with the complete SDK/runtime.
  Enforce the target with
  `Tests/test_PortablePackaging.ps1 -CompilerPath cpc.exe -MaxBytes 1000000`.

Reproduce against a fresh build and follow the [development loop](Tests/DEVELOPMENT.md).

## Cycle decisions

- One repair landed this cycle, verified with root `cpc.exe` and promoted
  (retained fast tier 382 selected / 380 unresolved; pedantic tier 316/316).
  - `src/compiler/frontend/cprimegen.c`, C++ conditional enum join:
    `0 ? eb1 : eb2` where both enumerators belonged to the enum currently
    being defined was joined as that incomplete enum type, and converting the
    second operand reported `cast to incomplete type` at
    `g++.dg/template/enum7.C:5`. The same-enum fast path now requires a
    completed enum; otherwise two enum operands are promoted to `VT_INT`.
    Reduced probes `build/scratch-enum/e2.C` and `e11.C` failed before and
    compile after; `e4.C`, `e6.C` and `e9.C` remained passing.
  - The full fast-tier re-run kept every overlapping status unchanged, and the
    promoted pedantic tier passed 316/316. Triage was regenerated from the
    fast-tier file to `build/compiler-bug-triage.{txt,json}` (380 unresolved).
- One repair landed this cycle, verified with root `cpc.exe` and promoted
  (retained fast tier 385 -> 381 unresolved of 383 selected; pedantic tier
  315/315). Final fast and pedantic re-runs compared against their pre-repair
  result files showed zero regressions.
  - `src/compiler/frontend/cprimegen.c`, the early `parse_btype` template-class
    branch: after a template-id resolved through `::` to a typedef naming a
    class-template specialization (`A<T>::type2`, `x<int>::zy`), a block-scope
    declaration stopped after the first member. A following `::type`/
    `::result2` was left unparsed, so `B<double>::type2::type tt = 12;`
    reported `';' expected (got 'tt')` and `x<int>::zy::result2 xxx;` reported
    `';' expected (got 'xxx')`. Added the same trailing
    `while (tok == ':' && (type->t & VT_BTYPE) == VT_STRUCT)
    parse_template_nested_typedef` continuation the namespace and alias paths
    already used. Fixed `g++.dg/template/using17.C` (FAIL_COMPILE -> PASS_RUN)
    and `g++.dg/template/memclass5.C` (FAIL_COMPILE -> PASS_COMPILE).
  - Cluster note: the two rows were singleton clusters by exact diagnostic text
    because the unexpected token differed, but reduced probes showed one shared
    defect. `build/scratch-mini/u7.C`, `u12.C`, `u16.C`, `u19.C` and `u22.C`
    reproduced the block-scope failure before the repair; `u8.C` is the same
    shape with the preceding `B<double> b;`.
- One repair landed this cycle, verified with root `cpc.exe` and promoted
  (retained fast tier 385 -> 384 unresolved of 386 selected; pedantic tier grew
  by `g++.dg/template/sizeof13.C` to 312/312). The full `-Tier all` re-run
  (698 rows, `build/probe-all`) showed exactly one status change against
  `build/pedantic-gcc-prev-fast.jsonl` and
  `build/pedantic-gcc-prev-ped.jsonl` - `sizeof13.C` FAIL_COMPILE -> PASS_LINK -
  and the published fast/pedantic re-runs showed zero regressions.
  - `src/compiler/frontend/cprimegen_templates.inc`,
    `template_call_signature_rank_bound`: substituted parameter replay used
    `make_func_type_from_saved_params` with no substitution probe whenever the
    pattern had no `typename`/`sizeof`/`decltype` marker and no member class.
    Forming a parameter type can itself fail - for `sizeof13.C` the binding
    `U = undef` makes the class template's defaulted `int N = sizeof(U)`
    ill-formed, and the hard `template 'A' has no usable default for argument 2`
    from `template_validate_arg_count` escaped the candidate filter, so
    `f<undef>(0)` never got to drop `f(A<U>)` in favour of `f(B<U>)`. Parameter
    replay now always goes through the same `template_probe_type_ex`
    `void (params)` replay the dependent branch already used, so such a failure
    returns -1 and removes the candidate. `CPRIME_PARSER_STATE=1` confirmed the
    shape: `[substitution] unknown type size` while `substitution=1`, then the
    hard default-argument error with `substitution=0`.
  - The check that mattered was a same-invocation comparison: a self-hosted
    `build/compiler/cpc.exe` cannot find `string.h`/`stdio.h` on its own (root
    `cpc.exe` resolves the installed runtime via `-print-search-dirs`,
    `C:/Users/Luke/AppData/Local/cpc/1.4`), so probe runs use
    `-RuntimeRoot C:\Users\Luke\AppData\Local\cpc\1.4`; a run with
    `-RuntimeRoot .` reports a bogus `include file 'string.h' not found` for
    `g++.dg/template/pretty1.C` and looks like a regression.
- Next lead for the other row of that cluster, `g++.dg/template/injected2.C`
  (still `template 'A' has no usable default for argument 2` at line 9): it is
  not a deduction/SFINAE case. `A<B<int> >` supplies only one argument, so the
  default `template<class>class U = T::template B` must resolve through the
  injected class name of `B<int>` to the class template itself (DR 1004);
  fix that lookup in the default-argument replay separately.
- `g++.dg/template/unify13.C` (`no matching function template 'f'`) needs
  deduction from an *unambiguous base* of the argument class
  (`mp_list<Wrap<int>::type, void>` matched from `A*`, PR c++/120161), which the
  candidate path does not attempt today.
- Three repairs landed this cycle, all verified with root `cpc.exe` and promoted
  (retained fast tier 395 -> 385 unresolved of 387 selected; pedantic tier grew
  by `g++.old-deja/g++.brendan/ptrmem2.C`, `g++.dg/init/ptrmem3.C`,
  `g++.dg/other/default6.C`, `g++.dg/other/ptrmem1.C`,
  `g++.dg/other/ptrmem3.C`, `g++.dg/template/ptrmem19.C`,
  `g++.old-deja/g++.jason/pmf3.C`, `g++.old-deja/g++.law/casts2.C` and
  `g++.old-deja/g++.other/pmf5.C`). Fast-tier and pedantic-tier re-runs after
  each publish showed zero regressions.
  - `src/compiler/frontend/cprimegen_member_pointers.inc`,
    `try_parse_cpp_member_pointer_owner`: a nested class of the class enclosing
    the member declaration is registered only under its qualified token
    (`C_N`), so `struct_find`/`find_class_template_def` on the unqualified
    spelling failed and `typedef int (N::*pmfn)(int);` inside `struct C`
    rejected `N` as a member-pointer owner (`')' expected (got ':')`). When the
    unqualified name does not resolve, the owner is now looked up through
    `find_class_scope_type_tok` against `cpp_member_decl_context->owner_tok` (or
    `active_member_class_tok`) and the qualified token is substituted into the
    saved owner-type tokens; the replay string keeps the source spelling.
    Reduced probes `build/scratch-p/q4.C` (nested, failed before) and `q5.C`
    (namespace scope, always passed) isolate it. Fixed
    `g++.old-deja/g++.brendan/ptrmem2.C`.
  - `src/compiler/frontend/cprimegen_member_pointers.inc`,
    `convert_cpp_member_pointer`: two different member-pointer types whose
    members are not compatible always reported `cannot convert`. Explicit
    conversions between pointer-to-member types whose members are both function
    types or both object types are allowed by [expr.reinterpret.cast], and both
    are stored as the same target/adjustment pair, so the explicit-cast path now
    retypes in place. Fixed `g++.dg/init/ptrmem3.C` (virtual member function
    pointer cast), `g++.dg/other/ptrmem3.C` (run: `(aip)(alp)0 != (aip)0`),
    `g++.dg/other/default6.C`, `g++.dg/template/ptrmem19.C`,
    `g++.old-deja/g++.jason/pmf3.C`, `g++.old-deja/g++.law/casts2.C` and
    `g++.old-deja/g++.other/pmf5.C`.
  - `src/compiler/frontend/cprimegen_member_pointers.inc`,
    `try_form_cpp_member_pointer`: a reference data member fell into the
    field branch and produced a pointer-to-member of reference type, so
    `&(D::m)` in a member function was rejected (`cannot convert 'extern extern
    int *' to 'struct __cpc_member_pointer_D_int_ref'`). Reference members now
    return 0 from the member-address path so `D::m` resolves through ordinary
    member access to the referent. Fixed `g++.dg/other/ptrmem1.C` (CWG 4379).
  - Cluster note: the three repairs came from one area (pointer-to-member), but
    the rows did not share a first diagnostic with each other
    (`')' expected (got ':')`, `cannot convert ...`, and two distinct
    `cannot convert` shapes), so they were selected by reduced probe, not by the
    triage cluster key.
- Three repairs landed this cycle, all verified with root `cpc.exe` and promoted
  (retained fast tier 400 -> 397 unresolved of 399 selected; pedantic tier grew
  by `g++.dg/template/non-type1.C`, `g++.dg/template/unify1.C` and
  `g++.dg/opt/pr82159.C`). Fast-tier and pedantic-tier re-runs after publishing
  showed zero regressions.
  - `src/compiler/frontend/cprimegen_templates.inc`, member-function body replay:
    when a member body spelled the enclosing class name as an explicit
    template-id (`unit<I1 - Q1, I2 - Q2>` in a `unit<I1,I2>` member template),
    the body substitution consumed the `<...>` and emitted only the current
    instantiation's mangled token, so the written result type collapsed to the
    enclosing specialization (`unit<1,0>` instead of `unit<-1,0>`) and the
    member template could not return its own computed type. The body path now
    keeps the argument list, substituting the enclosing class and member
    parameters exactly as the written return-type path already did, and falls
    back to the old mangled self token when every argument is one of this
    template's own parameters (`foo<T>`, which `crash18.C` needs for
    `&foo<T>::template priv<U>`). The first attempt without that fallback
    regressed `g++.old-deja/g++.pt/crash18.C` with `'foo__long_template'
    undeclared`; the fallback restored it and both tiers re-ran clean.
  - Cluster note: `non-type1.C`/`unify1.C` shared only the diagnostic
    `cannot convert 'struct unit<...1...>' to 'struct unit<...n1...>'`; the
    earlier lead that blamed integer constant-expression folding was wrong --
    `build/scratch-t/n3.C` shows `A - B` folding is fine (`sum<1,2>::v == -1`).
    The reduced reproducers `build/scratch-t/n{1,2,4,5,6,7}.C` all fail at the
    member body line, not at the caller, and `n6.C` (a `typedef` of the written
    result type) fails the same way, which is what localized the defect to the
    body replay rather than to deduction or to `get_tok_str` interning.
- Five repairs landed this cycle, all verified with root `cpc.exe` and promoted
  (retained fast tier 405 -> 400 unresolved of 402 selected; pedantic tier grew
  by `g++.dg/tree-ssa/pr24439.C`, `g++.old-deja/g++.brendan/operators5.C`,
  `g++.dg/template/static19.C`, `g++.old-deja/g++.brendan/copy5.C`,
  `g++.dg/overload/arg5.C`). Each fast-tier re-run after publishing showed zero
  regressions against the previous results file.
  - `src/compiler/frontend/cprimegen_exceptions.inc`, `cpp_eh_throw_expression`:
    the rethrow terminator check listed `;`, `)`, `,`, `:` and `}` but not end
    of stream. A conditional arm is evaluated from its own saved token string,
    so `v ? true : throw;` arrived with `throw` followed by `TOK_EOF`, fell into
    the operand path and reported `expression expected before '<eof>'`. Adding
    `TOK_EOF` fixed `g++.dg/tree-ssa/pr24439.C`; `void f(){ throw` and
    `v?1:throw` without the closing `;`/`}` still report `';' expected (got
    '<eof>')`, and a rethrow from a conditional arm inside a handler propagates
    to the outer handler at runtime (`build/scratch-t/t-rethrow.C`).
  - `src/compiler/frontend/cprimegen_cpp_names_overload.inc`,
    `try_call_cpp_unary_operator`: only member operators were resolved, so a
    non-member `operator!`/`operator~`/prefix `operator++` on a class operand
    fell through to the builtin path (`invalid operand types for binary
    operation`). Added `try_call_cpp_free_unary_operator` (one `CType`
    argument, `cpp_resolve_free_operator`, then `gfunc_param_typed_with_conversions`
    + `finish_cpp_member_func_call(func_type, 1)`) and used it when no member
    operator exists. Fixed `g++.old-deja/g++.brendan/operators5.C` and
    `g++.dg/template/static19.C`.
  - `src/compiler/frontend/cprimegen_cpp_names_overload.inc`,
    `call_arg_matches_param_type`: the by-value arm accepted only exact types
    and single-argument constructor conversions, so a derived class argument
    could not initialize a by-value base parameter (`void f(B); D d; f(d);`
    failed, while the reference arm already handled it through
    `class_value_is_derived_from`). Added the derived-to-base copy arm;
    codegen already sliced in `gen_assign_cast`. Fixed
    `g++.old-deja/g++.brendan/copy5.C` (a friend `operator!=` taking
    `const BaseClass` by value) and `g++.dg/overload/arg5.C`.
  - Cluster note: `copy5.C`/`operators5.C` shared only the diagnostic
    `invalid operand types for binary operation`. Reduced probes showed the two
    rows had distinct roots - a free unary operator for `operators5.C`, a
    by-value derived-to-base argument for `copy5.C` - and the hidden-friend ADL
    path was *not* involved (`build/scratch-t/p8.C` visits base `B` of `D`).
- Next lead for `g++.dg/template/mem_func_ptr.C` (`unknown type size`): the
  partial specializations `order_member_ptrs<R (Klasse::*)(T1)>` and its
  `const`/`volatile`/`const volatile` variants are not matched for member
  function pointer arguments, so the primary template stays incomplete (the
  plain `T Klasse::*` partial specialization does work). Reduced probes:
  `build/scratch-t/p17.C` (non-cv, `sizeof` form matching the test) and `p18.C`
  (`const` form) both report `nested template type member
  'order_member_ptrs____cpc_template_type_struct___cpc_member_pointer_X_func_void_float::type'
  must be a typedef`, i.e. the partial specialization never wins over the
  primary template; p15.C/p16.C (`O<...>::type v;` at global scope) show the
  same non-match behind `static data member definition requires class member
  declaration`. Start at the partial-specialization matcher for
  member-function-pointer patterns, not at the member lookup.
- Previous cycle: two repairs landed, both verified with root `cpc.exe` and promoted
  (pedantic tier went 283/291 -> 291/291; retained fast tier 409 -> 405 rows):
  - `src/compiler/frontend/cprimegen.c`, `class_base_path_count`: a *direct*
    base edge was never counted (the guard required an indirect base via
    `class_has_base(info->base_tok, base_tok)`, which is false for the direct
    base itself). Every directly derived class therefore looked like it had no
    unique base path, so `class_has_unique_base` was false and derived-to-base
    pointer/reference argument conversion was rejected in overload resolution
    ("no matching overloaded function"). Reduced to `struct B{}; struct C:B{};`
    + `int f(B*); f(&c);`. Fixed `g++.dg/{abi/covariant3,abi/covariant5,
    inherit/covariant2,ipa/devirt-c-4}` and `g++.old-deja/g++.eh/catch11,
    catch12` (pointer catch through virtual bases resolves through the same
    predicate).
  - `src/compiler/frontend/cprimegen_templates.inc`, explicit class
    instantiation: `put_extern_sym(function, NULL, 0, 0)` was called after
    `instantiate_template_member_body_for_func_tok` had already emitted the
    out-of-line member body, and `put_extern_sym2` rewrites an existing symbol
    entry to `SHN_UNDEF`/value 0. The destructor body was emitted but orphaned
    (leading `.uw_base`-only `.text` in the linker map, no destructor symbol in
    `-Wl,-Map`), and the call went to address 0. The call is now made only while
    the symbol is still undefined. Fixed `g++.old-deja/g++.pt/explicit74.C`
    (FAIL_RUN_CRASH -> PASS_RUN) without changing the required "declared but not
    defined" link error for the `template class foo<int>;`-only case.
- Trap that cost a false regression scare: `build/compiler/cpc.exe` does not
  find `stdio.h`/`string.h` unless the runner is given `-RuntimeRoot` (it then
  passes `-B`). Runs of the corpus made with that binary and no runtime root
  report bogus `include file 'stdio.h' not found` failures. Always compare runs
  made the same way (root `cpc.exe`, or `-RuntimeRoot <root>`).
- Next lead for the `g++.dg/template` non-type cluster (`non-type1.C`,
  `unify1.C`, 2 rows, "cannot convert 'unit<...const_1...>' to
  'unit<...const_n1...>'"): the conversion *source* is `unit<1,0>` where the
  folded `operator/` result should be `unit<-1,0>`, so the deduction of
  `unit<I1 - Q1, I2 - Q2>` binds the wrong value; this is not only an interned
  -name mismatch. Unifying `template_integer_expression_arg_tok`'s
  `__cpc_template_const_expr_*` namespace with `__cpc_template_const_*` was
  tried, changed the diagnostic to the same shape and fixed nothing, and was
  reverted.
- The earlier `delete` analysis was wrong and is withdrawn: `-S` output is not
  a faithful record of emitted code, so reading a body from `build/*.s` and
  concluding that "the operand load and spill store are missing" was an
  artifact of the listing. Retained runtime evidence with root `cpc.exe`
  (SHA256 `98877b9826ab98fc88d07f6a4ad9cf83d8f9dd1199b2a49eea1cde5f6de7ebde`):
  `struct A { int v; ~A(); }; static A* p;` with `p = (A*)malloc(...);
  p->v = 42;` followed by another function doing `delete p` prints
  `dtor 42` and exits 0, so the operand reaches the destructor and `free`
  intact. `build/probe-del-{1,3,5,6,8}.C` all compile and run with exit 0,
  including `probe-del-8.C`, which dirties the frame with a `volatile char`
  fill before the `delete`. `cpp_delete_pointer`
  (`src/compiler/frontend/cprimegen_exceptions.inc`) is not the cause of
  `g++.old-deja/g++.pt/../explicit74.C`.
- `g++.old-deja/g++.pt/explicit74.C` reproduces as FAIL_RUN_CRASH
  (0xC0000005) with root `cpc.exe`. It is not a `delete` bug: the reduced
  reproducer below crashes with the destructor body empty. Explicit
  instantiation plus an out-of-class destructor plus any object construction
  is the trigger; removing the explicit instantiation or defining the
  destructor in-class both fix it.
  `template<class T> struct foo { ~foo(); };` /
  `template<class T> foo<T>::~foo(){}` / `template class foo<int>;` /
  `int main(){ foo<int>(); return 0; }` (`build/rv5.C`) crashes, while the same
  file without `template class foo<int>;` (`build/rv8.C`) and the in-class
  destructor variant (`build/rv9.C`) run clean. `build/rv{1,3,10,11,12,14}.C`
  also crash; `build/rv13.C` reports `undefined symbol
  '??1?$foo@H@@QEAA@XZ'`, so the explicit instantiation registers the
  destructor under the MSVC spelling while the out-of-line definition emits a
  different symbol. Next step: reconcile explicit-instantiation destructor
  symbol naming with the out-of-line member definition, then recheck the
  object-construction path (`foo<int>` local and `new foo<int>()`) that faults
  at address 0. `g++.old-deja/g++.pt/ttp34.C` shares only the crash exit code;
  it has no `delete` and is a separate defect.
- Tooling trap for the next cycle: do not diagnose codegen from `-S`. Verified
  counter-example: `int* gp; int v = 7; void f(){ int* r; r = &v; gp = r; }`
  prints `7` at runtime, yet the `-S` listing for `f` contains the two operand
  loads and no `movq %rax, -8(%rbp)`/`movq %rax, gp(%rip)` stores at all.
  `x86_64_asm_body` coverage is incomplete (`gen_modrm`/`gen_modrm64` paths are
  not recorded), so the listing omits real stores and `lea`s. Confirm codegen
  findings by running the binary, or by disassembly, before changing the
  compiler.
- `explicit74.C` narrowed further, all measured with root `cpc.exe`
  (SHA256 `98877b9826ab98fc88d07f6a4ad9cf83d8f9dd1199b2a49eea1cde5f6de7ebde`).
  The trigger is not the destructor call and not `delete`: it is automatic
  storage for a class that was explicitly instantiated while its destructor is
  defined out of line. `template<class T> struct foo { int i; ~foo(); };` +
  out-of-line `~foo()` + `template class foo<int>;` leaves *any* later
  `foo<int> x;` (or `foo<int>()`) faulting with 0xC0000005 at address 0 before a
  single statement in `main` runs (`printf` in the destructor body never
  prints). Controls, same translation unit, same compiler: `sizeof(foo<int>)`
  is 4 and returns 5 (`t14`), member access through `malloc`ed storage returns 7
  (`t15`/`t16`), dropping the explicit instantiation (`t8`) or defining the
  destructor in class (`t11`) runs clean. So class layout, member address
  computation, and the heap path are all intact; the defect is in how the
  explicit-instantiation path (`parse_template_decl` class branch, which walks
  `member_func_overloads` for `struct_tok == owner`,
  `instantiate_template_member_body_for_func_tok`, then
  `put_extern_sym(function, NULL, 0, 0)`) leaves that instantiated class's
  automatic-object address/zero-init lowering. Next step: check whether the
  emitted `memset` destination for such a local is a relocation against a
  symbol left undefined by `put_extern_sym(..., NULL, 0, 0)` instead of a
  `VT_LOCAL` stack slot. Probes are `build/scratch-rv/t{1..16}.C`.

- Repair loop cost is low: `scripts\windows\build-cprime.bat -c <absolute>\cpc.exe`
  self-hosts into `build/compiler` in about two seconds, and
  `Tests/pedantic/gcc/run.ps1 -Compiler build/compiler/cpc.exe -Select <prefix>`
  rechecks a cluster in about two seconds. Verify fixes there, then publish with
  `Build.cmd` so root `cpc.exe` stays the single active compiler. Pass the
  compiler to `build-cprime.bat` as an absolute path; a relative name resolves
  against `build/compiler` after its `cd` and fails before compiling anything.
- Dependent-base lookup attempt, reverted without source changes: the
  `g++.dg/lookup/template1.C` / `two-stage1.C` pair is not fixed by tagging the
  direct `C<B> : T` base edge as dependent and skipping those edges in the
  final unqualified-member replay around the
  `type_has_member_func_name(this_target, unqualified_call_tok)` path.  With
  that instrumentation the saved `C` specialization still contained plain
  `foo`, while the emitted body called `B_foo`, and the new non-dependent
  lookup helper was never reached.  The replay is downstream of the binding
  that selects `B::foo`; the next trace should start at the earlier
  class-scope lookup/replay path, not from base metadata.
- Clusters that remain blocked on whole features, with the exact blocking site:
  `_Complex` (`src/compiler/frontend/cprimegen.c` hard error), vector types
  (`__attribute__((vector_size))` has no frontend support), `<coroutine>`, and
  the RTTI `typeid(...).name()` expectations, which need Itanium type mangling
  while `cpp_rtti_type_name` currently emits the internal spelling from
  `append_type_mangle_string`.
- Flexible-array clusters split: `g++.dg/ext/flexary40.C` needs class-typed
  member initialization, while `g++.dg/ext/flexary24.C` fails in
  `decl_design_flex` with `ref != p->flex_array_ref` for a brace-initialized
  struct member, which is the smaller of the two and is compile-only. Root
  cause, measured this session: the shape is a trailing `const char *a[]`
  initialized from a runtime parameter (`{ 1, { a, "b" } }`), which forces the
  deferred (dynamic) initialization path for the local static in
  `can_lower_local_static_dynamic_init`/`emit_local_static_dynamic_init`, and
  that path allocates its backing object through
  `decl_initializer_alloc(..., has_init == 0)`, so the flex dry run that sizes
  the object never runs and `decl_design_flex` sees `p->flex_array_ref == NULL`
  with the array symbol still at `c == -1`. The same failure reproduces for a
  global dynamic initializer (`const char *p; S g = { 1, { p, "b" } };`), so
  this is a deferred-initialization bug, not a static-storage one; the
  all-constant spelling (`static S t = { 1, { "a", "b" } };`) already passes
  because it stays on the static-data path. A size-only probe of the
  initializer can complete the bound (the array symbol then holds the element
  count), but the object is still sized from the struct's cached size, so the
  replay overflows it and trips `init_assert`. Completing the bound is
  necessary but not sufficient: the deferred path also needs the flex extent in
  the allocated object and in the replayed aggregate walk, or a per-object
  struct type that carries the completed bound.
