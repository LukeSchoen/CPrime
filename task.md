# Remaining work

Target: 100% of fast tests and the retained pedantic GCC checks, with no missing
coverage, weakened expectations, compiler internal errors, or timeout retries.

Current retained state: 324 unresolved retained GCC rows (307 FAIL_COMPILE,
10 FAIL_RUN, 7 FAIL_RUN_CRASH) of the 326 rows recorded by the last fast-tier
run on compiler SHA256
`15c97e5326d2f2941d45c30f234b8791719c62b585d273c84f6854b382ace51f` (result
file `build/scratch-cycle/fast-after14/results.jsonl`; its zero-regression
compare was `build/scratch-cycle/fast-after13`, and the promoted pedantic tier
was re-run as `build/scratch-cycle/ped-after6` (372/372) after
`build/scratch-cycle/ped-after5` (371/371), `build/scratch-cycle/ped-after4`
(370/370), `build/scratch-cycle/ped-after3` (369/369),
`build/scratch-cycle/ped-after2` (366/366) and
`build/scratch-cycle/ped-after` (361/361)).
The compact cluster report is `build/compiler-bug-triage.txt` (machine-readable
paths in `build/compiler-bug-triage.json`; 25 clusters, largest is
`g++.dg/coroutines` include-file gaps x24). It now lists 324 unresolved rows
of 326.

The retained corpus is the runner's *fast* tier over `pedantic/gcc/corpus`
(`Tests/run-all.ps1` invokes it that way); `Tests/tiers.json`'s pedantic list is
  the promoted set and currently scores 372/372
  (`build/scratch-cycle/ped-after6`). Triage must be fed a fast-tier results file,
not a pedantic-tier one, or it trivially reports zero unresolved rows.

This work is time-boxed to 45 minutes total; solve as much as possible within
that time and then stop, do not keep looping past the budget. Work without
stopping after individual repairs or asking for approval: regenerate the
triage with `Tests/triage_retained_failures.ps1`, choose a cluster that shares
one first diagnostic in one source area, fix it, verify it, promote verified
passes to pedantic in `Tests/tiers.json`, then continue immediately with the
next cluster until the 45-minute budget is spent. At the 45-minute mark, stop
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
- `g++.dg/template/ttp41.C` (`initialization of incomplete type`) is fixed and
  promoted (see the cycle note below). Its sibling `g++.dg/template/typename27.C`
  shares the diagnostic text but is a separate dependent elaborated-type lookup
  defect. The related TT-shaped probes are still open: `build/scratch-c2/p4.C`
  (`template<class U> struct B<TT, const U*>;` where the pattern argument is the
  *primary's* template-template parameter and the candidate declares only `U`)
  and `p10`/`p11`/`p12`/`p23` still fail with `base class ...`/incomplete-type
  diagnostics. Next step for that shape: treat a pattern argument that names a
  parameter of the primary template as dependent, exactly as
  `deduce_nested_class_pattern` already does when the argument is followed by
  `<`.

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

- Sixteen retained rows were repaired and promoted in this cycle, all verified
  with root `cpc.exe` (SHA256
  `15c97e5326d2f2941d45c30f234b8791719c62b585d273c84f6854b382ace51f`):
  fast tier 340 -> 324 unresolved of 326 selected, pedantic GCC tier
  356/356 -> 372/372 in `build/scratch-cycle/ped-after6`. Each fast-tier
  re-run (`fast-after` through `fast-after14`) reported zero regressions
  against its predecessor; the pedantic re-runs were `ped-after` through
  `ped-after6`.
  - `cprimegen_rtti.inc` now emits Itanium-compatible `type_info::name()`
    spellings for classes, arrays and local classes; `g++.dg/rtti/cv1.C` and
    `g++.dg/rtti/typeid9.C` pass.
  - `cprimegen_substitution.inc` permits incomplete array elements while
    probing a type-id, fixing `g++.dg/rtti/typeid5.C`.
  - `cprimegen.c` pretty-function replay now renders member-template parameter
    names and dependent free-function parameter bindings, plus compact pointer
    return spellings; `g++.old-deja/g++.pt/memtemp77.C` and
    `g++.dg/diagnostic/bindings1.C` pass.
  - `cprimegen_lifecycle.inc` uses the class tag keyword (`union` versus
    `struct`) in lifecycle and aggregate replay; `g++.old-deja/g++.mike/net37.C`,
    `g++.dg/tree-ssa/pr22615.C` and `g++.old-deja/g++.brendan/union3.C` pass.
  - `cprimegen.c` accepts an elaborated global tag after `struct`/`class`,
    fixing `g++.dg/lookup/scoped10.C`, `g++.dg/tc1/dr68.C` and
    `g++.old-deja/g++.brendan/scope4.C`.
  - Anonymous typedef bases receive a stable class name
    (`g++.old-deja/g++.jason/anon.C`), and out-of-class class definitions enter
    their namespace/class scope while parsing the base list
    (`g++.old-deja/g++.martin/lookup1.C`).
  - A parenthesized member declarator with attributes followed by `(` is no
    longer mistaken for a constructor parameter list
    (`g++.dg/ext/attrib61.C`).
  - Nested anonymous aggregate members are injected recursively with their
    offsets, fixing `g++.old-deja/g++.other/anon2.C`.
  - Zero-length array members no longer consume aggregate initializers,
    fixing `g++.dg/init/array31.C`.
  - Local regressions were added under `Tests/features/`: RTTI names and
    incomplete array type-ids, member/dependent pretty functions, union
    lifecycle, elaborated global lookup, anonymous typedef bases, out-of-class
    base scope, constructor-paren attributes, and nested anonymous unions.
  - The remaining 324 rows are still led by the whole-feature clusters
    (coroutines, vector types, `_Complex`) and deep template lookup; the
    regenerated triage is `build/compiler-bug-triage.{txt,json}`.

- Two repairs landed this cycle, both verified with root `cpc.exe` (SHA256
  `ddeda2f593a9701e3372812c47d5115159f46066d91d9dd5f3d74766f3001548`) and
  promoted (retained fast tier 346 -> 340 unresolved of 344 selected; pedantic
  GCC tier 352/352 -> 356/356 in `build/scratch-mp/ped2`). The fast-tier re-run
  (`build/scratch-mp/fast3`) reported zero regressions against
  `build/pedantic-gcc-nsfix-fast`; the only status changes were the four
  promoted rows (below) and the promotion itself, which removed two rows from
  the fast selection.
  - `src/compiler/frontend/cprimegen_templates.inc`,
    `deduce_direct_template_type` and function-call deduction: a member
    *function* pointer parameter written `R (T::*name)(U...)` was read through
    the `T C::*` member-type rule, so `R` bound the member function type
    instead of its result type, and `T` (and any `U`) fell through to the
    "unbound parameter takes the whole argument type" default. The target
    parameter type then nested the argument inside itself
    (`__cpc_member_pointer_<arg>_func_<arg>`), which surfaced as
    `cannot convert 'struct __cpc_member_pointer_A_func_double' to 'struct
    __cpc_member_pointer___cpc_member_pointer_A_func_double_func_func_double'`.
    When the parenthesized member-pointer group starts immediately after the
    deduced parameter, the result type is now taken from
    `member_type.ref->type`; the caller additionally binds the owner parameter
    from `member->owner_tok` when the parameter token is followed by `::*` and
    reuses the argument's function parameter list for further deduction.
    Fixed `g++.dg/template/ptrmem12.C` and `g++.dg/template/pr70466-2.C`.
    Probes `build/scratch-mp/{p1,p3,p4,p5,p7}.C` failed before and pass after;
    `p2.C`/`p8.C` (explicit template arguments) and `p6.C` (`T C::*` data
    member) guard the shapes that already worked. Regression
    `Tests/features/Classes/pass/test_member_function_pointer_deduction.cpp`.
  - `src/compiler/frontend/cprimegen_member_pointers.inc` and
    `src/compiler/frontend/cprimegen.c`: two member-pointer conversion gaps.
    `compare_cpp_member_pointers` chose the more-derived operand's type as the
    composite but kept only *its* member cv-qualifiers, so `ap != bp` with
    `const int A::*` against `int B::*` (B derived from A) asked for a
    qualification-dropping conversion and reported `cannot convert`; the
    composite now takes the union of both members' cv-qualifiers through
    `mk_cpp_member_pointer`. Separately, `convert_cpp_member_pointer` required
    a class-subobject relation between the two owners once the member types
    were compatible, so the explicit `reinterpret_cast` of a member function
    pointer between *unrelated* classes was rejected; a new mode value 2 (only
    passed for `reinterpret_cast` in the named-cast path) carries the words
    over when neither owner relation exists. Fixed
    `g++.old-deja/g++.other/cast7.C` and `g++.old-deja/g++.mike/p9206.C`
    (both FAIL_COMPILE -> PASS_RUN). Probes `build/scratch-mp/{R1,R2,R3,S2}.C`
    failed before and pass after; `S1.C` (`static_cast` between unrelated
    member pointers) still reports `cannot convert`, `R4.C` guards same-type
    copy-initialization. Regression
    `Tests/features/Classes/pass/test_member_pointer_qualifier_and_reinterpret.cpp`.
  - Still open: `g++.dg/conversion/cast1.C` (line 17, `(int Base::*)&Derived::n`
    with classes local to `main`) and `g++.old-deja/g++.mike/pmf1.C` (line 30,
    `int (MD::*)(int) = &MD::f` for a base member, whose address is forming the
    plain function pointer `int (*)(struct X *, int)` instead of a member
    pointer). For cast1 the locality is the trigger: probes
    `build/scratch-mp/L1.C` (local `Base`/`Derived`, explicit cast, fails) and
    `L4.C` (local classes, base-to-derived cast, fails) reproduce while the
    namespace-scope equivalents `L2.C`/`c2.C` pass, so the remaining block is
    local-class owner identity in the conversion path rather than the rules
    repaired here. Note `L3.C` (`int Base::* p = &Derived::n;` for a local
    `Derived`) rejects the same way but its rejection is correct C++ (the
    implicit member-pointer conversion runs base -> derived, never the
    reverse).

- One repair landed this cycle, verified with root `cpc.exe` (SHA256
  `5d60f1beaa9d447915e1887a8c11b36e04fe30c07462da685bef2eca13d6fb25`) and
  promoted (retained fast tier 346 -> 344 unresolved of 346 selected; pedantic
  GCC tier 350/350 -> 352/352 in `build/pedantic-gcc-nsfix-ped`). The
  fast-tier re-run (`build/pedantic-gcc-nsfix-fast`) and the both-tier run
  `build/pedantic-gcc-nsfix-root-all` reported zero regressions against
  `.../0c3196cc...` and `.../4e0d5a85-ped`, and `Tests/run-all.ps1 -Tier fast`
  reported every language suite passing with the same retained GCC rows.
  - `src/compiler/frontend/cprimegen_lifecycle.inc`,
    `add_pending_global_dynamic_init`: a namespace-scope object with a
    non-constant initializer is initialized by a synthesized
    `__cpc_global_dynamic_init_N` function that is queued and replayed at end of
    translation unit. The replay entered the namespace of `active_member_class_tok`
    (`symbol_namespace_scope_tok(pm->struct_tok)`), which is 0 for anything that
    is not an out-of-class static member, so the saved initializer was replayed
    in the *global* namespace and an unqualified name declared earlier in the
    same namespace was not found. `PendingMemberFunc` now carries the
    declaration's `namespace_tok` (set only when there is no member class
    owner) and the replay enters it, keeping class members on the existing
    derived-scope path. Fixed `g++.old-deja/g++.mike/ns6.C` and
    `g++.dg/template/koenig3.C` (both FAIL_COMPILE -> PASS_COMPILE/ASSEMBLE).
  - Reproducers: `build/scratch-task/ns3.C` (`namespace A { int i = 1; int j = i; }`),
    `ns6.C`, `ns7.C` (nested), `ns8.C` (typedef member), `ns11.C`
    (`int j = i + 1;`), `ns12.C` (`int j = f();` calling a same-namespace
    function) all failed before and pass after; `ns1.C`/`ns15.C` are the same
    shape without a reader, `ns4.C` (global `i`) and `ns14.C` (qualified
    `A::i`) guard the shapes that already worked, and `ns9.C`/`ns10.C`/`ns13.C`
    guard global-scope, constant-initialized and out-of-class static-member
    initialization.
  - Regression
    `Tests/features/Namespaces/pass/test_namespace_scope_dynamic_initializer_lookup.cpp`
    initializes objects from same-namespace scalars, a same-namespace function
    call, a class-construction member and a reopened namespace, then checks the
    computed values at runtime.
  - Not fixed by this change: the sibling "expression replayed without its
    lexical scope" shapes are separate mechanisms. `g++.dg/parse/defarg22.C`
    (`void fn1 (int i = sizeof (i))`) fails in the default-argument parameter
    scope, `g++.dg/parse/functor1.C` (`int (j)(1);`) in the parenthesized
    direct-initializer disambiguation, and `g++.dg/overload/defarg4.C`
    (`bool (*pFunc)(const U&) = func`) in an unqualified same-class member
    template reference from a default argument; all three still fail.
- A third repair landed in the same cycle, verified with root `cpc.exe`
  (SHA256 `4e0d5a85f54118249e8c3d06f9e0d5224c35211b163a2429e8cea42f42a7103f`)
  and promoted (retained fast tier 347 -> 346 unresolved of 348 selected;
  pedantic GCC tier 349/349 -> 350/350 in `build/pedantic-gcc-4e0d5a85-ped`).
  The fast-tier re-run (`build/pedantic-gcc-cond-root`, baseline
  `build/pedantic-gcc-3b7d0266...`) reported exactly one status change
  (`g++.dg/expr/cond4.C` FAIL_COMPILE -> PASS_COMPILE) and zero regressions,
  and `Tests/run-all.ps1 -Tier fast` reported every language suite passing with
  the same retained GCC rows.
  - `src/compiler/frontend/cprimegen.c`, conditional-expression type
    selection: the two arms that accept a class operand against a *non-class*
    one (`find_cpp_conversion_operator` for `struct` against scalar/pointer)
    had no class-versus-class counterpart, so `psi ? QChar::null : s[psi]`
    with `QCharRef::operator QChar()` reported `type mismatch in conditional
    expression`. Two arms now also select the operand whose class type the
    conversion function produces, and the join's per-operand
    `try_call_cpp_conversion_operator` call is no longer restricted to
    non-struct targets, so the converted temporary is what the branch yields.
    Fixed `g++.dg/expr/cond4.C` (FAIL_COMPILE -> PASS_COMPILE). Runtime probe
    `build/scratch-cond/r1.C` returns 7 for the `QCharRef` arm and 0 for the
    `QChar` arm. Known cosmetic issue: the class-value join reports
    `assignment from incompatible pointer type` from `gen_assign_cast` on the
    two branch addresses for this new shape; the joined value is correct.
  - Not fixed by this repair: `g++.dg/expr/cond6.C` (`true ? f() : b` with
    `D : B`) and the reduced prvalue shapes `build/scratch-cond/p5.C` line 6
    and `p6.C` still report the original `type mismatch` diagnostic. The
    prvalue path is `cpp_try_conditional_prvalue`
    (`cprimegen_conditionals.inc`), which only resolves differing class arms
    through `class_has_single_arg_constructor_for`; a derived-to-base attempt
    there must construct the base from the derived operand, and that works only
    when the base *declares* a copy constructor (cond6 does, p6 does not, and
    the functional-cast path has no implicit-copy-constructor arm) - an
    attempt guarded that way was made and reverted this cycle because it left
    `cpp_conditional_declared_copy_from_derived` matching nothing for cond6
    and turning p6's diagnostic into `no matching constructor`.
- Two repairs landed this cycle, both verified with root `cpc.exe` (SHA256
  `3b7d026615d44535427a4b335294f09269e1a16df5863a36d6489ae50e962f58`) and
  promoted (retained fast tier 352 -> 347 unresolved of 349 selected; pedantic
  GCC tier 346/346 -> 349/349 in `build/pedantic-gcc-3b7d0266-ped`; the first
  of the two repairs was published at SHA256
  `cb01cd35febd59c3634fe7b790028993bd17a5acb0da087e8abf1b44a6987e0f`, where
  the pedantic corpus scored 346/346 in `build/pedantic-gcc-cb01cd35-ped`).
  Each fast-tier re-run with a results-file baseline reported zero regressions
  (352-row runs `build/pedantic-gcc-cb01cd35...` and `build/pedantic-gcc-3b7d0266...`
  against `build/pedantic-gcc-09ad0d03279148e99f77100d2385f3f4` and
  `.../cb01cd35...` respectively, then the 349-row
  `build/pedantic-gcc-16b25ead9ddc43eaa898608ed2b4eaf6` after promotion), and
  `Tests/run-all.ps1 -Tier fast` reported every language suite passing with the
  same retained GCC rows.
  - `src/compiler/frontend/cprimegen_lifecycle.inc`,
    `resolve_member_func_by_param_signature`: a conversion function of a class
    template that is declared in class and defined out of class was registered
    twice for the instantiation. The class-pattern replay spells the conversion
    target through a bound-type alias (`operator __cpc_bound_type_1486 *`),
    while the out-of-class definition replay spells the substituted type
    (`operator X *`), so the method tokens differed and the definition could not
    bind to its declaration. Both entries then ranked equally in
    `find_cpp_conversion_operator`, which reported `ambiguous` internally and
    surfaced as `cannot convert 'struct S__X' to 'struct X *'`; the definition
    also emitted its body under a separate mangled name. `allow_explicit == 2`
    (explicit-cast) and the implicit paths both failed. When the definition is a
    conversion function the candidate loop now iterates the conversion bucket
    for the class and matches the target type with `is_compatible_types` instead
    of comparing method tokens, so the definition adopts the declaration's
    method token and mangled name and only one overload entry survives.
  - Fixed `g++.old-deja/g++.brendan/crash47.C` (FAIL_COMPILE -> PASS_ASSEMBLE,
    `(REP *) a` on a `const Ref<REP>&`) and `g++.dg/parse/conv_op1.C`
    (FAIL_COMPILE -> PASS_COMPILE, `static_cast<A<int>::B *>(p)`), the
    template conversion-operator PR 8572 ICE test. Reduced probes:
    `build/scratch-c47/c7.C` (out-of-class definition, dependent target,
    failed before) versus `c9.C` (declaration only) and `c6.C` (in-class
    definition), which show the defect needs the out-of-class definition.
    `c10.C` runs both conversions (`operator T *` and `operator T`) on an
    instantiated `holder<element>` and checks the returned values.
  - Regression
    `Tests/features/Templates/pass/test_out_of_class_conversion_operator_identity.cpp`
    checks `element *p = h;`, `element copy = h;` and `(element *) h` for a
    class-template conversion operator declared in class and defined out of
    class.
  - `src/compiler/frontend/cprimegen_lifecycle.inc`,
    `try_materialize_constructor_conversion_into`: an explicit cast to a *base*
    class type from a derived object only searched for a converting
    constructor, so `(A) B()` reported `no matching constructor for cast to`.
    A new arm mirrors the same-type arm: after `try_adjust_derived_pointer_to_base`
    retargets the address to the base subobject, the base is copied memberwise
    (with the rvalue/move flag preserved), which is the slicing copy the
    standard requires.
  - `src/compiler/frontend/cprimegen.c`, explicit cast to a class type: after
    the member-pointer and converting-constructor attempts,
    `try_call_cpp_conversion_operator(&type, 2)` (explicit mode, requiring the
    converted type to already match the target) now runs, so a cast can use a
    conversion function of the source class. Constructors are still tried
    first, so existing constructor-based casts keep their behaviour.
  - Fixed `g++.dg/eh/cast1.C` (FAIL_COMPILE -> PASS_COMPILE, `(A) B()`),
    `g++.old-deja/g++.mike/p3524b.C` (FAIL_COMPILE -> PASS_ASSEMBLE,
    `(ccPair<float>) r` with `ccO<float> : ccPair<float>`) and
    `g++.old-deja/g++.pt/memtemp67.C` (FAIL_COMPILE -> PASS_RUN, `(A<double>) a1`
    through `template <class T2> operator A<T2>() const`). Reduced probes
    `build/scratch-c47/c11.C` (plain `operator element()` cast, failed before and
    now runs), `c12.C`/`c13.C` (`(A) B()`, compile and run), plus the unchanged
    `g++.dg/eh/cast1.C` shape.
  - Regression
    `Tests/features/Classes/pass/test_explicit_cast_to_class_type_uses_conversions.cpp`
    runs both shapes: `(element) p` through `plain::operator element()`, the
    explicit pointer cast, and `(base) derived()` slicing a base subobject.
  - Still open in the same area (not fixed, no corpus row isolates them): an
    explicit cast to a class type never considers an *explicit* constructor or
    the source's conversion function when the target is reached through a
    reference, and the cast path still resolves constructors before conversion
    functions rather than ranking both, so a case where the conversion function
    is the better match would pick the constructor.
- One more repair landed this cycle, verified with root `cpc.exe` (SHA256
  `fd9595e2b5ea68b1e6e288b540e78c07f8465581e1caf88968d4d0ffc3912dcb`) and
  promoted (retained fast tier 353 -> 352 unresolved of 354 selected; pedantic
  GCC tier 343/343 -> 344/344 in `build/pedantic-gcc-sizeof-ped2`). The
  fast-tier re-run (`build/pedantic-gcc-sizeof-root`, baseline
  `build/pedantic-gcc-ca7d48133aa94a229efc9d4a9260ed95`) reported exactly one
  status change (`g++.dg/template/sizeof19.C` FAIL_COMPILE -> PASS_COMPILE)
  and zero regressions; `Tests/run-all.ps1 -Tier fast` reported every language
  suite passing.
  - `src/compiler/frontend/cprimegen.c`: the unqualified identifier fallback
    in `unary()` resolved a non-static data member only through
    `find_cpp_this_symbol()`, so `static const int c = sizeof (b);` inside the
    class declaring `b` reported `'b' undeclared`; the minimal non-template
    shape `struct A { int b; static const int c = sizeof(b); };` fails the
    same way (`build/scratch-sizeof/s1.C`). Class member initializers are now
    parsed with `cpp_static_member_initializer_owner` set, and when there is no
    receiver in an unevaluated operand
    (`cpp_unevaluated_expression_depth > 0`) the field is resolved through
    that class; the value is materialized as an lvalue of the field's type
    that is never evaluated. The flag is scoped to in-class static data member
    initializers, so the pre-existing behaviour of `sizeof (member)` in static
    member function bodies is unchanged and an evaluated use such as
    `static const int c = b;` still reports `'b' undeclared`
    (`build/scratch-sizeof/v3.C`).
  - Regression
    `Tests/features/Classes/pass/test_static_member_initializer_member_sizeof.cpp`
    checks the computed constants (`sizeof(int[4])/sizeof(int) == 4`, pointer
    member size) for an ordinary class and for a nested class inside a class
    template. Reduced probes `build/scratch-sizeof/s1.C`-`s6.C` and `v1.C`
    failed before and pass after; `v2.C`/`v3.C` guard the unchanged
    static-member-function and evaluated-use behaviour.
- One repair landed this cycle, verified with root `cpc.exe` (SHA256
  `4797f4b7056d73797aa4f2b5edc660e62b5c547100e601ad362ac57c5a72b694`) and
  promoted (retained fast tier 354 -> 353 unresolved of 355 selected; pedantic
  GCC tier 342/342 -> 343/343 in `build/pedantic-gcc-optattr-ped2`). The
  fast-tier re-run with root `cpc.exe` (`build/pedantic-gcc-optattr-root`,
  baseline `build/pedantic-gcc-05563379760a4b0eb1b21dc205c8300a`) reported
  exactly one status change (`g++.dg/other/canon-33194.C` FAIL_COMPILE ->
  PASS_COMPILE) and zero regressions; `Tests/run-all.ps1 -Tier fast` reported
  every language suite passing with the same retained GCC rows.
  - `src/compiler/frontend/cprimegen.c`,
    `local_paren_starts_direct_initializer`: the probe that decides whether a
    declarator's `(` begins a direct initializer
    (`T x(1);`) or a parameter list (`void f(int x);`) scanned the whole
    parenthesized token run without skipping GNU attribute groups. Attribute
    arguments such as `aligned(8)`, `section("data")` and
    `format(printf, 1, 2)` put a value token (or a string) at parameter-list
    depth, so `void g(int x __attribute__((aligned(8))));` was classified as a
    direct initialization; `type_decl` then took the
    `goto done` path before `post_type` consumed the parameter list, leaving
    the declared type as `void` and reporting `declaration of void object` at
    `cprimegen.c` 21462. The scan now consumes a balanced `__attribute__((...))`
    group (tokens still appended to the replay buffer) and continues, so
    attribute arguments never influence the disambiguation. Attributes with no
    argument list (`unused`, `deprecated`, `noreturn`), `cleanup(f)`, and
    `mode(DI)` already parsed, which is why the trigger looked argument-list
    specific; the real trigger is any attribute argument that looks like an
    initializer operand.
  - Regression
    `Tests/features/Functions/pass/test_parameter_attribute_arguments.cpp`
    covers `aligned(8)`, `section("cpc_param_attr")`,
    `format(printf, 1, 2)` and the corpus's `noreturn, format(printf, 1, 2)`
    function-pointer parameter, and runs the function to check the value
    survives. Reduced probes: `build/scratch-att/a3.C`, `b2.C`, `b3.C`,
    `b5.C`, `b6.C`, `h5.C`-`h7.C`, `i3.C`-`i8.C`, `j1`-`j5.C` failed before and
    pass after; `i1.C`/`i2.C`/`i10.C`/`k1.C`-`k8.C` guard the
    no-argument-attribute, cleanup, post-parameter-list-attribute, local
    direct-initializer and local-function-declaration shapes that already
    worked.
  - Not fixed by this change: `Tests/pedantic/gcc/corpus/g++.dg/ext/attrib61.C`
    still reports `parameter type expected before 'corge'` at its constructor
    parameter `U (__attribute__((unused)) corge) (int);`, so the earlier guess
    that it shared the canon-33194 root is wrong; that is a separate
    attribute-in-decl-specifier shape.
- One repair landed this cycle, verified with root `cpc.exe` (SHA256
  `23fe28b83a67ec2dc7e53296f96e44a18c3e55df7554da3f71fa326085f19a2a`) and
  promoted (retained fast tier 358 -> 354 unresolved of 356 selected; pedantic
  GCC tier 338/338 -> 342/342 in
  `build/pedantic-gcc-d183e35c1316422cb4fad81395068bf9`). The fast-tier re-run
  (`build/pedantic-gcc-05563379760a4b0eb1b21dc205c8300a`, baseline
  `build/npc-ttp41-full`) reported 4 status changes (all
  FAIL_COMPILE -> PASS) and 0 regressions, and every language suite passed.
  - `src/compiler/frontend/cprimegen_lifecycle.inc`, new
    `cpp_member_class_name_tok`: an unnamed struct/union that declares member
    functions (`static struct { void f(); } x;`,
    `typedef struct { void f(); } S;`) had no class name token, so all four
    member-function entry points (`add_pending_member_func`,
    `declare_member_func`, `declare_static_member_func`,
    `add_pending_static_member_func`) reported `member functions require a
    named struct/class` (or the `inline` variant). The first member function
    declaration now gives the anonymous type a unique synthesized name
    (`__cpc_anonymous_class_N`), replaces the struct symbol's tag and links it
    into the identifier table exactly as a named tag is linked, so later
    members, overload bookkeeping and mangled symbols share one stable name.
    Named classes keep their own spelling unchanged.
  - `src/compiler/frontend/cprimegen_lifecycle.inc`, `add_pending_member_func`:
    the lowered `this` parameter was always spelled `struct <class> *`, so
    re-parsing it for a *union* member function body made `struct_decl` see
    `VT_STRUCT` for a union type symbol and report `redeclaration of 'U'`
    (`union U { void f() { } };` failed, as did the unnamed form). The tag is
    now `union` when the class type is a union.
  - Fixed `g++.dg/other/anon7.C`, `g++.dg/ext/vla7.C`,
    `g++.old-deja/g++.oliva/linkage1-main.cc` (all FAIL_COMPILE -> PASS) and
    `g++.old-deja/g++.other/parse1.C` (FAIL_COMPILE -> PASS_ASSEMBLE). Reduced
    probes `build/scratch-anon2/t{1,3,6,7,8}.C` separate the two roots: `t3`
    (named body in a struct) already passed, `t6` (named union, inline body)
    and `t1` (unnamed union, inline body) failed with the tag and
    class-naming defects respectively. `t7`/`t8` (declarations only, and
    `class`) passed with and without the change.
  - Still open in the same area, unchanged by this repair: `union`/`struct`
    member bodies whose lowering re-spells the class tag in other token paths
    (`cprimegen_lifecycle.inc` 3730, 4345, 4416 and
    `add_ctype_tokens` for a union type) still emit `struct <union>`, and
    `struct U *p;` for a union `U` is still rejected as a redeclaration; no
    corpus row currently isolates those.
- One repair landed this cycle, verified with root `cpc.exe` (SHA256
  `399dc4c0e687014a801f8df6c9eb88016fe0f0f67f677ceca658e780a6d6a067`) and
  promoted (retained fast tier 359 -> 358 unresolved of 361 selected; pedantic
  GCC tier 337/337 -> 338/338 in `build/pedantic-gcc-ttp41`). The retained
  corpus re-run over all 361 fast-tier rows (`build/npc-ttp41-full`, baseline
  `build/pedantic-gcc-29e88c018c52479eb7d9772aaaac02c8`) reported `+1 passes,
  0 regressions`, and the promoted pedantic corpus scored 338/338 with
  `+0 passes, 0 regressions` against the previous promoted set. The language
  suites were not re-run before the cycle budget ended.
  - `src/compiler/frontend/cprimegen_templates.inc`,
    `matching_partial_class_template`: a partial specialization whose pattern
    argument names a member class template of the enclosing class
    (`template<class U> struct B<C, const U*>;` inside `template<class T> struct
    A`) never matched `A<int>::B<A<int>::C, const int*>`, so the declared-only
    primary stayed incomplete and `A<int>::B<A<int>::C, const int*> b;` was
    reported as `initialization of incomplete type`. Instrumentation on the
    reduced probe `build/scratch-c2/p3.C` showed the loop binds
    `pattern=C actual=A__int_C t=1580(C) class=A__int_B`:
    `template_substituted_pattern_argument` resolves the unqualified member
    name to a type token spelled `C` (a type-token space distinct from the
    argument's owner-qualified spelling), so the token comparison failed even
    though both name the same member. `lookup_cpp_record_decl` gives the
    argument owner `A__int` and no record at all for the pattern token, and
    `find_class_template_def(make_static_member_tok(A, C))` is null (member
    class templates are not indexed under an owner-qualified `TemplateDef`
    token), so the fix is scope-based rather than lookup-based: when the
    substituted pattern token does not match the argument, and the argument's
    record owner is the *same* owner token as the partial specialization's own
    name (`A__int_B`), and the argument's final `_`-separated component equals
    the pattern token's spelling, the argument is accepted. Namespace-scope
    specializations have no owner token on either side, so they keep their
    existing behaviour exactly.
  - Reduced probes: `build/scratch-c2/p3.C` and corpus
    `g++.dg/template/ttp41.C` now compile; `p13.C` (namespace scope), `p15.C`
    (non-template enclosing class) and `p16.C` (primary defined) still pass, and
    `p2.C` (same shape as `p3.C`) passes. Not fixed by this change:
    `build/scratch-c2/p4.C` and `p10`/`p11`/`p12`/`p23` are the distinct
    TT-shaped case (pattern argument is the primary's template-template
    parameter), and `g++.dg/template/typename27.C` is a separate dependent
    elaborated-type lookup defect.
- One repair landed this cycle, verified with root `cpc.exe` (SHA256
  `3aaa13eed0469afed31b845b83c62681e7795c0c7ab23fb8203b9cda1ebc9750`) and
  promoted (retained fast tier 360 -> 359 unresolved of 361 selected; pedantic
  GCC tier 336/336 -> 337/337). `Tests/run-all.ps1 -Tier fast` reported every
  language suite passing with `+0 passes, 0 regressions` against the previous
  fast results file, and `Tests/run-all.ps1 -Tier pedantic` scored the retained
  GCC corpus 337/337 (`build/pedantic-gcc-a6aeec7a6b9c401da43a545501d616a4`)
  with the same four latent language-suite failures (Classes, Expressions,
  Includes, Templates); the three failing `features/Classes` rows
  (`test_member_default_arg_after_overloaded_constructors`,
  `test_member_pointer_calls_and_virtual_bases`,
  `test_out_of_class_member_nested_range_for`) are unchanged from before this
  repair.
  - `src/compiler/frontend/cprimegen.c`,
    `parse_explicit_constructor_member_initializers`:
    `g++.old-deja/g++.other/init14.C` builds `mbstate_t _M_st;` where
    `mbstate_t` is a typedef of an
    *anonymous* struct, and value-initializes it with `fpos(int pos) :
    _M_st()`. The class-member branch above the general replays through
    placement construction, but it requires `get_struct_type_name_tok` on the
    member type to be non-zero, which is false for an anonymous member class
    type, so
    the initializer fell through to the general form `this->_M_st <args>` with
    an empty argument list and was reported as `expression expected before ';'`
    (`_M_st(7)` in the same shape reported `cannot convert 'int' to 'struct
    <anonymous>'`). A member of unnamed class type initialized with an empty
    argument list now replays the equivalent brace form
    (`this->_M_st = {}`), which is the same value-initialization semantics the
    named-type path already provided. The new arm is guarded by
    `!field_struct_tok`, so every named class-type member keeps its existing
    lowering exactly.
  - Reduced probes `build/scratch-cyc/mi{1,4,8}.C` fail with the previous
    compiler and pass with the new one; `mi{5,6}.C` (typedef of a *named* struct,
    and a directly named struct member) pass with both and show the defect is
    the anonymous spelling, not the `typedef` or the empty argument list.
    Regression `Tests/features/Classes/pass/
    test_constructor_value_init_anonymous_member.cpp` value-initializes the
    member over a dirtied buffer through placement new and checks the member is
    zeroed, so it covers the runtime semantics as well as the compile error.
  - Trap repeated this cycle: `Tests/triage_retained_failures.ps1` with no
    `-ResultsPath` picks the newest `build/pedantic-gcc-*/results.jsonl`, which
    after a pedantic-tier run is the promoted file and trivially reports zero
    unresolved rows. Pass the fast-tier file explicitly
    (`-ResultsPath build/pedantic-gcc-05563379760a4b0eb1b21dc205c8300a/
    results.jsonl`).
- One repair landed this cycle, verified with root `cpc.exe` (SHA256
  `63e99048b210edf0ae5af0205b663a37e2b8efb49b431e1f7d8608752805c7e69`) and
  promoted (retained fast tier 361 -> 360 unresolved; pedantic GCC tier
  335/335 -> 336/336). `build/pedantic-gcc-npc-fast14` compared against
  `fast12` showed no status regressions; `Tests/run-all.ps1 -Tier fast`
  reported every language suite passing, and `Tests/run-all.ps1 -Tier
  pedantic` scored the retained GCC corpus 336/336 with the same latent
  language-suite failures (Classes, Expressions, Includes, Templates).
  - `src/compiler/frontend/cprimegen.c`, the PE `__builtin_va_start` path:
    it required the second operand to retain `VT_LOCAL`, so
    `g++.dg/other/vararg-2.C`'s address-valued
    `((const char *)(&(szFmt)))` was rejected with `__builtin_va_start
    expects a local variable`. A non-local operand now normalizes to
    `char_pointer_type`, adds the eight-byte home-slot offset with
    `gen_op('+')`, and stores the result; the direct-local path is unchanged.
    Regression
    `Tests/features/Functions/pass/test_va_start_address_expression.cpp`
    compiles and runs the address-valued form.
  - Failed experiment, fully reverted: skipping zero-length array members in
    `decl_designator` made `g++.dg/init/array31.C` pass but produced a
    compiler that crashed on `g++.dg/init/array10.C` and
    `g++.old-deja/g++.martin/sts_partial.C` and could not self-host. No
    initializer changes are retained.
  - Failed experiment, fully reverted: the `g++.dg/parse/redef1.C` lead
    (allowing the built-in `wchar_t` typedef to be replaced from a
    system-header `#line ... 3` marker) did not work because the frontend did
    not observe the parsed system-header flag on that path. No preprocessor
    or typedef changes are retained.
- One repair landed this cycle, verified with root `cpc.exe` (SHA256
  `34808fc63559e541ec5430331f05ce862f1dfe9ce52b9dec15ef89be76e46fde`) and
  promoted (retained fast tier 364 -> 363 selected, 362 -> 361 unresolved;
  pedantic GCC tier 334/334 -> 335/335). `build/pedantic-gcc-npc-fast12`
  compared against `fast10` showed exactly one status change
  (`g++.old-deja/g++.brendan/overload6.C` FAIL_COMPILE -> PASS_ASSEMBLE) and
  zero regressions; `Tests/run-all.ps1 -Tier fast` reported every language
  suite passing with the same retained GCC rows.
  - `src/compiler/frontend/cprimegen_cpp_names_overload.inc`,
    `call_arg_matches_param_type`: overload resolution already accepted an
    argument-to-parameter conversion performed by a *conversion operator*
    (`call_arg_match_rank` -> `find_cpp_conversion_operator`), but the
    viability predicate that confirms the ranked candidate
    (`member_func_matches_arg_types`) did not, so the candidate was ranked and
    then discarded. `overload6.C` (`String a; a = lbuf;` with
    `S::operator temp_string&() const` and `String::operator=(temp_string)`)
    therefore reported `no matching user-declared copy assignment operator`
    from `try_call_cpp_assignment_operator`. The standard predicate is now
    `call_arg_matches_param_type_standard` and `call_arg_matches_param_type`
    adds the same class-argument conversion-operator fallback the rank
    predicate uses (guarded by `cpp_standard_conversion_only`).
    `gfunc_param_typed_with_conversions` gained the matching materialization
    for a class parameter, and `try_call_cpp_assignment_operator` now passes
    the value argument through it instead of the plain
    `gfunc_param_typed`, so the parameter receives the converted temporary
    rather than the source object's storage. Regression
    `Tests/features/Classes/pass/test_assignment_from_conversion_operator.cpp`
    (assignment plus an ordinary member-function parameter) passes.
    Reduced probe `build/scratch-assign/a1.C` is the minimal shape and failed
    with the previous compiler.
  - Not fixed by this change: `g++.old-deja/g++.brendan/overload6.C`'s
    sibling row `g++.dg/other/pr24623.C` still reports the same diagnostic
    (`m_item = __null;` needs the converting constructor
    `RefCountPointer(T* p = 0)`, i.e. a null-pointer-constant argument reaching
    a pointer parameter of the constructor, which the constructor search still
    rejects), so the two rows of that cluster have distinct roots.
  - Note for the next compare: runs with `build/compiler/cpc.exe` and no
    `-RuntimeRoot` report bogus `include file 'string.h' not found` for
    `g++.dg/rtti/cv1.C` and `g++.dg/rtti/typeid9.C` (that binary's implicit
    `-B` root is `build/compiler`); they are FAIL_RUN with root `cpc.exe` and
    were not touched by this repair.
- No repair landed this cycle (the budget went to triage and to reducing one
  language-semantics defect). New lead for `g++.dg/template/sfinae26.C`
  (`'>' expected after template argument '_Bool' opened near line 30 (got
  '<eof>')`, 1 row, FAIL_COMPILE): an explicit member-template-id call with a
  pointer-to-member template argument in a *declaration* context at namespace
  scope is reported as an out-of-class static data member definition.
  Reduced probes `build/scratch-c3/p{1,2,4,5}.C`, `d1.C`, `e2.C` all fail, and
  `d1.C` is the minimal shape, measured with root `cpc.exe`
  (SHA256 `7e3cc1cbdf718976321051e5bb9c045442a917f21ac9ee4edfff0a77d8ad06a3`):
  `struct C { bool b; }; template<class T> struct M { typedef T U; template
  <class V, V U::*P> static bool f(); }; bool y = M<C>::f<bool, &C::b>();`
  gives `static data member definition requires class member declaration
  'C_b'` at line 6, while the identical expression inside a function body
  (`d2.C`: `bool g() { return M<C>::f<bool, &C::b>(); }`) compiles. `p2.C`
  (three arguments, `bool y` replaced by a functional-cast initializer) is the
  corpus diagnostic verbatim, so both messages come from one defect.
  Controls that pass already: `q{1..5}.C` show the member-pointer template
  *parameter* declaration itself (`V U::*P`, `V T::*P`, `bool T::*` as a
  parameter) parses fine; `e3.C`/`e4.C` show a free function template with the
  same parameter shape in the same namespace-scope initializer is accepted; so
  the trigger is the qualified call `ClassTemplate<...>::memberTemplate<...>`,
  not the pointer-to-member argument.
  The two diagnostics are the probes at
  `src/compiler/frontend/cprimegen.c:9336` and `:9480` inside
  `try_parse_cpp_scoped_member_def` (entry at `:9232`), whose caller guard
  `if (tok != '(' || (decl_sym && (decl_sym->type.t & VT_BTYPE) != VT_FUNC))`
  treats a qualified member name that is followed by `<` (an explicit
  member-template-id call) as a static data member definition. Next step:
  instrument that probe's entry to identify which replay path reaches it with
  `class_tok = C`/`member_tok = b` (the outer call cannot be the one, since
  the declarator token there is `y` and the probe returns early), then decline
  the static-data path for a member that resolves to a function template while
  keeping the corpus row's own base call
  `set_member_constant<V,K>(opt, t, mem)` (member body, line 14) working.
- No repair landed this cycle (the budget went to localizing one
  language-semantics defect). Lead for `g++.dg/inherit/virtual5.C`
  (FAIL_RUN, exit 123): `A& ar = b1; ar = b2;` where `A` declares
  `virtual B& operator=(const B&)` and `B : A` has only the implicit copy
  assignment. Reduced probes `build/scratch-v5/p{1,2,3}.C`, measured with root
  `cpc.exe` (SHA256 `7e3cc1cbdf718976321051e5bb9c045442a917f21ac9ee4edfff0a77d8ad06a3`):
  - `p3.C` shows the explicit spelling `r.operator=(c2)` *does* dispatch
    virtually to `C2::operator=` (result 0), while `r = c2` calls
    `C0::operator=` directly (result 123). So the defect is the `=`
    expression path, `try_call_cpp_assignment_operator` in
    `src/compiler/frontend/cprimegen_cpp_names_overload.inc`, which always
    emits a direct call through `vpushsym`; it never consults
    `find_virtual_method_for_symbol` (unlike the conversion-operator path,
    which uses `push_virtual_call_target`).
  - `p1.C`/`p2.C` add a second, necessary half: a class that only inherits a
    virtual `operator=` needs its *implicit* copy assignment noted as a
    virtual override. `note_virtual_method` is only reached from the class
    body parser, so `B`'s implicit assignment has no virtual-table entry, and
    `find_virtual_method_for_symbol(B, B::operator=)` is null. Declaring it at
    class completion in `cprimegen.c` (after `struct_layout`, before
    `emit_virtual_tables_for_class`) via `note_defaulted_member_func` +
    `note_virtual_method` does register it, and the implicit assignment still
    lowers correctly at the use site (`b1 = b2` -> 0 with the retained
    compiler).
  - Failed experiment, reverted: substituting `push_virtual_call_target(owner,
    virtual_method, func_sym)` for the `vpushsym` push in
    `try_call_cpp_assignment_operator`. It compiles and dispatches to the
    right override, but the call then passes the receiver as the source
    argument (self-assignment: `p1.C` prints `direct 123`, `p3.C` prints
    `after = 123` instead of 0). The virtual target's duplicated receiver does
    not end up in place of the already-converted `this` argument, so the next
    attempt must model the two argument slots explicitly (or route the
    assignment through the ordinary member-call path that already handles
    `ar.operator=(b2)`), not patch the target push in place.
  - Trap for the next cycle: `-RuntimeRoot` is a runner option, not a compiler
    flag (`build/compiler/cpc.exe -RuntimeRoot ...` reports
    `invalid option -- '-RuntimeRoot'`); compile probes with the bare compiler.

- Two repairs landed this cycle, verified with root `cpc.exe` (SHA256
  `7e3cc1cbdf718976321051e5bb9c045442a917f21ac9ee4edfff0a77d8ad06a3`) and
  promoted (retained fast tier 366 -> 364 selected, 364 -> 362 unresolved;
  pedantic GCC tier 332/332 -> 334/334). `fast10` compared against `fast8`
  showed exactly the two promotions and zero other status changes, and `ped7`
  scored 334/334. `Tests/run-all.ps1 -Tier fast` reported every language suite
  passing with the same retained GCC rows. `Tests/run-all.ps1 -Tier pedantic`
  still reports the same four latent language-suite failures (seven rows,
  including the two Templates rows not listed in the earlier note), none
  related to these repairs.
  - `src/compiler/frontend/cprimegen.c` and `cprimegen_initializers.inc`: the
    already-committed class-type member-subobject table publication (HEAD
    `cf21bbd`) was verified and promoted. It fixed
    `g++.old-deja/g++.mike/p12306a.C` (FAIL_RUN_CRASH -> PASS_RUN); the new
    `Tests/features/Classes/pass/
    test_member_subobject_virtual_base_tables.cpp` passes. `p16146.C` remained
    a FAIL_RUN_CRASH.
  - `src/compiler/frontend/cprimegen_rtti.inc`, `cpp_dynamic_cast`: the source
    vptr was selected by scanning `virtual_table_infos` for the first entry with
    `class_tok == source_tok`. When the source class declares its own virtual
    functions and also has a virtual base, that list can begin with the virtual
    base's table, so `p16146.C` passed vptr offset 8 (the contained virtual
    base's vptr) instead of the source subobject's primary vptr at 0. The
    helper then read the wrong descriptor: the downcast returned null or a
    bogus subobject and `cip->addRef()` or the following virtual-base
    conversion faulted. Selection now uses
    `class_primary_virtual_root(source_tok)` and
    `virtual_vptr_offset(source_tok, root)`, the same primary-root anchor as
    `cpp_virtual_base_cast`; direct helper calls from
    `build/scratch-vbase/s7.C`/`s8.C` already returned the complete object
    before the compiler change, which localized the defect to vptr selection.
    Fixed `g++.old-deja/g++.mike/p16146.C` (FAIL_RUN_CRASH -> PASS_RUN).
    Regression `Tests/features/Classes/pass/
    test_dynamic_cast_virtual_base_primary_vptr.cpp` fails with the old root
    compiler and passes with the published one; probes are
    `build/scratch-vbase/s{1..8}.C`.
- No repair landed this cycle (the budget went to triage and to reducing the
  `g++.old-deja/g++.mike` runtime-crash cluster). New lead, measured with root
  `cpc.exe`: a virtual-base data member accessed through a *member subobject*
  expression faults with 0xC0000005, while the same access through a local
  object or through a pointer is correct. Reduced probes
  `build/scratch-cycle/v{1..13}.C`: `v12` (`struct a{int i;}; struct b :
  virtual a {int j;}; struct f {b _b;}; f D; D._b.i = 42;`) crashes on its own;
  `v13` (`b bb; b* bp = &bb; bp->i = 42;`) passes; `v11` (stderr markers) shows
  the fault is exactly the `D._b.i` store, while `D._b.j`, `bp->j` and
  `bp->i` around it are fine. The store appears to use the raw `cumofs` (0)
  returned by `find_field`, clobbering `_b`'s vptr, because
  `adjust_virtual_field_access` (`src/compiler/frontend/cprimegen.c:13865`,
  called from the `.` field path at :17967 and the `->` path at :17292)
  early-returns unless `class_field_owner(&vtop->type, field)` finds the
  declaring class, and that walk does not reach the virtual-base subobject of
  the receiver. Next step: resolve the field's owner so `D._b.i` takes the
  `cpp_virtual_base_cast` path, then retain a local regression covering the
  member-subobject write and read (`p12306a.C`/`p16146.C` both exercise the
  `b*` conversion of a virtual-base subobject and fail at runtime).
- Follow-up on that lead, measured with root `cpc.exe` (SHA256 `7e3cc1cb`):
  the member-subobject table publication landed for automatic storage only.
  Static-storage objects and the `new` pointer path still fault with
  0xC0000005 on the same shape (`build/scratch-vf/w2.C`, `f g;` at namespace
  scope, and `w7.C`, `f* p = new f;`), because the nonlocal path calls only
  `initialize_virtual_tables_for_object` and
  `initialize_virtual_tables_for_pointer` still has no member walker. Reuse
  `initialize_virtual_tables_for_member_objects` there (the pointer form must
  push the field address instead of using `r`/`addr`).
- One repair landed this cycle, verified with root `cpc.exe` (SHA256
  `a3e2c536529e22b9f9a37ef8cc5a1f2e6316835affeebf39762f8703d8111b1d`) and
  promoted (retained fast tier 367 -> 366 selected, 365 -> 364 unresolved;
  pedantic tier 331/331 -> 332/332). The fast-tier re-run against
  `build/pedantic-gcc-npc-fast6` showed zero status changes on the common rows
  (`build/pedantic-gcc-npc-fast7`), and the regenerated triage is
  `build/compiler-bug-triage.{txt,json}`.
  - `src/compiler/frontend/cprimegen.c`, class member loop: an in-class friend
    declaration whose specifier list starts with `inline`/`constexpr`
    (`inline friend float EvalNextArg() { return 1.0; }`) was parsed as an
    ordinary member. `try_skip_in_class_lifecycle_specifiers` and
    `try_cpp_constructor_specifiers` deliberately restore those specifiers when
    the declarator is not the class name, so the ordinary member parser read
    `inline` as the member type, took `friend` as the declarator name and
    reported `invalid type for 'friend'`. Added
    `try_skip_in_class_friend_specifiers`, which consumes the leading
    inline/constexpr specifiers only when a `friend` declaration follows and
    replays them into the saved declaration (`TOK_INLINE1` / `tok_constexpr`
    after the `friend` token, alongside the existing has-body inline marker).
    Fixed `g++.old-deja/g++.brendan/crash6.C` (FAIL_COMPILE -> PASS_ASSEMBLE).
  - Regression `Tests/features/Classes/pass/
    test_inline_friend_function_definition.cpp` covers the explicit-
    specialization class body (`inline friend` with a body) and a plain class
    with both `constexpr friend` and `inline friend` definitions. Reduced
    probes `build/scratch-fr/a{1..4}.C`: `a1` (template specialization,
    `inline friend`) and `a2` (plain class, `inline friend`) failed before and
    compile after, while `a3` (`friend` declaration) and `a4` (plain `inline`
    member) always passed. `build/scratch-fr/c1.C` confirms `constexpr friend`.
  - Not part of this repair: an in-class friend function definition is still
    not found by ADL from an unqualified call (`build/scratch-fr/run1.C`,
    `run2.C` both report `no matching overloaded function 'Eval'`, and `run2.C`
    is the plain non-inline `friend` shape), so the friend-body lookup path is
    a separate open lead.
  - Budget note: this cycle's 15-minute budget was already spent verifying and
    publishing this repair, so no second cluster was started.
- One repair landed this cycle, verified with root `cpc.exe` (SHA256
  `d699677ae6c7fba5dd0a791ca56505a638fa942596b99c4e84234b2ac66793fc`) and
  promoted (retained fast tier 370 -> 367 selected, 368 -> 365 unresolved;
  pedantic tier 328/328 -> 331/331). The fast-tier re-run against
  `build/pedantic-gcc-npc-fast5` showed exactly the three promoted status
  changes and zero regressions, and the regenerated triage is
  `build/compiler-bug-triage.{txt,json}`.
  - `src/compiler/frontend/cprimegen_templates.inc`,
    `parse_one_template_type_arg`: two argument shapes stopped at an
    unconsumed `<`. A function template-id argument
    (`Container<int, default_initialize<int> >`,
    `x.template set_default<random_positive<T> >()`) was read as the bare name,
    so `parse_template_type_args` reported `'>' expected after template
    argument 'f' opened near line N (got '<')`; the identifier path now
    consumes the argument list and instantiates through
    `instantiate_template_if_needed` when the name resolves to a non-class,
    non-alias function template. An unqualified nested class template of the
    enclosing class (`Probe<Inner<int> >` inside `Outer<T>`) left the nested
    shortcut with the same unconsumed `<`; it now instantiates through
    `instantiate_template_argument_type`, the path the qualified class-template
    argument already used. Fixed `g++.dg/template/subst1.C`,
    `g++.dg/template/memtmpl5.C` and `g++.dg/template/ttp9.C`
    (FAIL_COMPILE -> PASS_COMPILE).
  - Regressions: `Tests/features/Templates/pass/
    test_function_template_id_template_argument.cpp` (function-type non-type
    parameter plus explicit member-template call) and
    `Tests/features/Templates/pass/
    test_unqualified_nested_class_template_argument.cpp`. Reduced probes
    `build/scratch-t/p2.C` (`S<f<int> >`) and `p3.C` fail before and compile
    after; `p1.C` (`S<f>`, bare function name) always passed.
  - `Tests/run-all.ps1 -Tier fast` reported every language suite passing with
    the same retained GCC set, and the promoted pedantic tier scored 331/331
    (`build/pedantic-gcc-npc-ped4`).
- One repair landed this cycle, verified with root `cpc.exe` (SHA256
  `fb2a361dfc93ccf170b26e216ed3e1c289b72787d80fd2a1bf1be54f92ffcd30`) and
  promoted (retained fast tier 372 -> 370 selected, 369 -> 368 unresolved;
  pedantic tier 327/327 -> 328/328). The fast-tier re-run against
  `build/pedantic-gcc-npc-fast4` showed exactly the promoted status change and
  zero regressions (`build/pedantic-gcc-npc-fast5`), and the regenerated triage
  is `build/compiler-bug-triage.{txt,json}`.
  - `src/compiler/frontend/cprimegen_templates.inc`, `deduce_partial_type_argument`:
    class partial-specialization patterns had no member-pointer case, so the
    pattern tokens after the deduced parameter (`K :: *`) were never consumed,
    the candidate was dropped (`matched = 0`) and a declared-only primary stayed
    incomplete (`unknown type size`). The matcher now reads the pattern
    textually and fills the bindings from the argument's member-pointer record:
    `T K::*` deduces T as the member type and K as the owner, and
    `R (K::*)(T1) [cv]` recognises the `(K::*)` declarator, reuses the existing
    parameter-list walk on the member function type, and requires the pattern's
    trailing cv to equal the member function type's cv. Owner and member
    bindings go through `template_type_tok_from_ctype` /
    `template_exact_ctype_typedef_tok`, the shapes `deduce_direct_template_type`
    already uses for `T C::*`.
  - `src/compiler/frontend/cprimegen_template_ordering.inc`,
    `class_order_member_pointer_form`: structural class ordering cannot model
    member pointers, so `order_member_ptrs<T K::*>` and
    `order_member_ptrs<R (K::*)(T1)>` both matched a member function pointer
    argument and reported `ambiguous partial specialization`. The function form
    is now ranked above the object form when both match (an arbitrary member
    type cannot be deduced from a synthesized function member type).
  - `src/compiler/frontend/cprimegen_templates.inc`,
    `deduce_direct_template_type`: deduction of `T` through `T C::*` in a
    function parameter decayed a function member type to a function pointer and
    stripped its top-level cv, so `fun_takes_member_ptr(&X::bar_c)` reported
    `cannot convert '__cpc_member_pointer_X_const_const_func_void_float' to
    '__cpc_member_pointer_X_func_void_float'`; the member type now keeps its own
    type and cv. Fixed `g++.dg/template/ptrmem21.C`
    (FAIL_COMPILE -> PASS_COMPILE).
  - `g++.dg/template/mem_func_ptr.C` is *not* fixed; it now compiles through
    line 49 and fails in the deferred function-template interface probe
    (`invalid function template interface 'fun_takes_member_ptr'`). Reduced:
    `build/scratch-mp/r1.C` (`sizeof(O<void (X::*)(float)>); f(&X::bar);`)
    fails while `r2.C` (same statements reversed) and `r3.C` (call only) pass,
    and the interface text is *identical* in the passing and failing runs
    (`void f____cpc_template_type_func_void_float__X__a1_h6...(
    __cpc_template_type_func_void_float __cpc_bound_type_1486::*)`), so the
    failure is ambient state at probe time (the `__cpc_bound_type_*` alias from
    `template_bound_type_spelling` or the scope the pending spec is compiled in),
    not the interface spelling. Start there, not in the matcher.
  - Unrelated row in the same triage cluster: `g++.dg/template/dr408.C` is an
    out-of-class static data member array bound (`build/scratch-mp/d1.C`),
    independent of member pointers.
- One repair landed this cycle, verified with root `cpc.exe` and promoted
  (retained fast tier 374 selected / 372 -> 370 unresolved; pedantic tier grew
  by `g++.old-deja/g++.robertl/eb48.C` and `eb75.C` to 326/326). Fast-tier and
  pedantic-tier re-runs after publishing showed zero regressions against
  `build/pedantic-gcc-npc-fast2` (exactly the two expected status changes) and
  `build/pedantic-gcc-npc-ped`.
  - `src/compiler/frontend/cprimegen.c`, reference named-cast binding: after
    `cpp_validate_named_cast` accepted a cast, the shared reference-cast tail
    rejected any target whose referenced type was not
    `is_compatible_unqualified_types` with the source, exempting only
    `reinterpret_cast`. `const_cast<char *&>(s)` where `s` is `const char *&`
    was therefore rejected as `cannot convert 'const char *' to 'char *&'`
    even though the const_cast rules were satisfied; the check now also exempts
    `const_cast`, so the result is an lvalue of the new reference type referring
    to the same object (the fall-through `vtop->type = cast_type` after
    `gaddrof`). Fixed `g++.old-deja/g++.robertl/eb48.C` and `eb75.C`
    (FAIL_COMPILE -> PASS_ASSEMBLE).
  - Regression `Tests/features/Expressions/pass/
    test_const_cast_reference_qualification.cpp` covers both reduced shapes
    (`const char *&` -> `char *&` and `int const *&` -> `int *&`) and checks the
    alias is preserved at runtime. Reduced probes `build/scratch-pf/c{1..5}.C`
    isolate it: `c1`/`c4` (pointer const_casts), `c3` (same-type reference
    const_cast) and `c2`/`c5` (qualification-changing reference const_cast,
    failed before) - only the last shape was broken, and `CPC_DBG_CAST`
    instrumentation showed `cpp_validate_named_cast` passed with `similar=1`
    and the error came from the later reference-cast check.
- Second repair landed this cycle, verified with root `cpc.exe` and promoted
  (retained fast tier 374 -> 372 selected after the two promotions; 370 -> 369
  unresolved; pedantic tier 326/326 -> 327/327). Fast-tier and pedantic-tier
  re-runs showed zero regressions against `build/pedantic-gcc-npc-fast3`
  (exactly one status change) and `build/pedantic-gcc-npc-ped2`.
  - `src/compiler/frontend/cprimegen_cpp_names_overload.inc`,
    `try_call_cpp_binary_operator`: member-operator candidate ranking saw only
    the right operand's type, so the integer constant zero was `int` and a
    member operator taking `const char*` was never viable for
    `(tempHost = ddeDefaults->GetHost()) == 0` (`invalid operand types for
    binary operation`). The live operand is now marked with `VT_NULLPTR_TYPE`
    (`is_null_pointer(vtop)` and not already a pointer/null pointer) before
    `resolve_member_func_by_arg_types` / `call_arg_match_rank`, and the marker is
    dropped before the call is lowered. Fixed `g++.old-deja/g++.robertl/eb51.C`
    (FAIL_COMPILE -> PASS_ASSEMBLE).
  - Regression `Tests/features/Expressions/pass/
    test_member_operator_null_pointer_constant.cpp`. Negative probes
    `build/scratch-pf/n{1,2,3}.C` pin the boundary: `d == 1` still reports
    `invalid operand types for binary operation`, `d == 0` with a pointer
    parameter compiles, and `d == 0` against a class-taking operator without a
    usable conversion still fails.
- Next cluster candidates still open near this area: `g++.dg/template/ptrmem12.C`
  and `g++.dg/template/pr70466-2.C` both report `cannot convert 'struct
  __cpc_member_pointer_...' to 'struct __cpc_member_pointer_<same>_...'`, i.e.
  deduction for `mem_fun_ref(&A::f)` / `foo(&B::bar)` produced a member pointer
  whose member type is itself a member pointer. Recheck them first now that the
  pointer-to-member and converting-argument paths have changed.
- Pedantic language-suite latent failures, verified to reproduce with a
  compiler built from the pre-repair source (self-host build of the stashed
  `cprimegen.c`, hash `3aae08dce07b1d655ccf995d9af5211ff4ae62ca684cba9c0a64ec862693596c`),
  so they are not regressions from this cycle's repair:
  `features/Expressions/pass/test_functional_conversion_operator.cpp` (runtime
  exit 1), `features/Classes/pass/test_member_default_arg_after_overloaded_constructors.cpp`
  (`duplicate static member template definition`),
  `features/Classes/pass/test_member_pointer_calls_and_virtual_bases.cpp`
  (`static data member name`), `features/Classes/pass/test_out_of_class_member_nested_range_for.cpp`
  (`cannot convert 'const struct Owner_Item *' to 'struct Owner_Item *'`) and
  `features/Includes/pass/test_cstdio_function_identity.cpp`
  (`undefined symbol '__cpc_ns_std_fclose'`). `Tests/run-all.ps1 -Tier pedantic`
  currently reports 21 passed / 4 failed suites from these five rows.
- Three repairs landed this cycle, all verified with root `cpc.exe` and promoted
  (retained fast tier 382 -> 374 selected and 380 -> 372 unresolved; pedantic
  tier 316/316 -> 324/324). The fast and pedantic re-runs after publishing
  showed zero regressions against their pre-repair result files, and
  `Tests/run-all.ps1` reported every other suite passing with the same 372
  retained failures.
  - Null pointer constant argument conversion: an argument that is an integer
    constant zero could not select a converting constructor taking a pointer,
    because overload resolution saw only `int`. The deferred-argument replay
    already marks that case with `VT_NULLPTR_TYPE`; live call arguments,
    default arguments and returned class values now set the same marker while
    the constructor is selected and constructed
    (`gfunc_param_typed_with_conversions` and the class-result arm of
    `TOK_RETURN` in `src/compiler/frontend/cprimegen_cpp_names_overload.inc`
    and `src/compiler/frontend/cprimegen_statements.inc`). The constructor
    arms of `call_arg_match_rank_standard` / `call_arg_matches_param_type`
    keep a `constructor_arg_type` copy so the marker survives the non-pointer
    normalization. Fixed `g++.dg/overload/defarg7.C`,
    `g++.dg/ipa/pr61160-3.C`, `g++.dg/ipa/pr69649.C` and
    `g++.old-deja/g++.jason/thunk1.C` (FAIL_COMPILE -> PASS_RUN).
  - `src/compiler/frontend/cprimegen_lifecycle.inc`,
    `try_materialize_constructor_conversion_into`: copy-initialization
    resolved constructors with the explicit-including
    `resolve_member_func_by_arg_types`, so `void f(A a = 0)` with
    `A(const char*)` and `explicit A(const int*)` reported `ambiguous
    overloaded member function 'constructor'` once both were viable. The
    implicit mode now uses `resolve_implicit_constructor_func`, which excludes
    explicit constructors before ranking, so `defarg7.C` selects
    `A(const char*)`.
  - Variadic member argument counts: a variadic member was never viable for
    more arguments than the parameter list spelled out, so `A(...)` could not
    convert any argument and `VectorNd(size_t, size_t, ...)` could not take
    five. `member_overload_accepts_argument_count` now mirrors
    `free_func_accepts_argument_count`, and `member_func_arg_match_rank` /
    `member_func_matches_arg_types` pass the remaining arguments through
    `FUNC_ELLIPSIS` at the worst rank
    (`cprimegen_cpp_names_overload.inc`, `cprimegen_lifecycle.inc`,
    `cprimegen_initializers.inc`). Fixed
    `g++.dg/cpp2a/concepts-nondep6.C`, `g++.dg/ext/attr-format1.C`,
    `g++.dg/opt/inline3.C` and `g++.old-deja/g++.brendan/crash63.C`; reduced
    probes `build/scratch-x/{p1,p2,p3,p4,r3,r5,s3}.C` isolate the call,
    return and functional-cast shapes.
  - Environment trap that cost this cycle most of its budget: `cpc.exe` on
    `PATH` resolves to `C:\Luke\Src\Archive\coder\cpc.exe`, an unrelated
    compiler that reports phantom parse failures on valid source. Invoke the
    repository compiler by explicit path (`.\cpc.exe` or
    `build\compiler\cpc.exe`), or read diagnostics from the runner's
    `results.jsonl`. The self-host compiler also needs `-RuntimeRoot` (the
    runner's `-B`) before it can find the installed runtime headers.
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

## Next leads (updated 2026-09-10)

- `g++.old-deja/g++.mike/pmf1.C` (`&MD::f` for a member function inherited from a
  base): the address expression forms the plain function pointer
  `int (*)(struct X *, int)` instead of `int (X::*)`/`int (MD::*)`, so no
  member-pointer conversion applies. Reduced probes `build/scratch-mp/M1.C`
  (`int (MD::*p)(int) = &MD::f;`) and `M2.C` (`int (X::*p)(int) = &MD::f;`)
  fail, while `M3.C` (inherited *data* member `&MD::a`) and `M4.C` (own member
  `&MD::g`) pass - so start from the address-of-member path that decides whether
  `try_form_cpp_member_pointer(class_tok, member_tok)` succeeded before falling
  back to the ordinary function-designator lowering
  (`cprimegen_member_pointers.inc` ~203-343 already requires
  `candidate->struct_tok == class_tok`, which is false for an inherited member;
  `resolve_member_func` then supplies a base-qualified symbol whose
  `int (X*, int)` shape is what leaks out).
- `g++.dg/conversion/cast1.C` (`(int Base::*)&Derived::n` with `Base`/`Derived`
  declared inside `main`): member-pointer conversions work for namespace-scope
  classes (`build/scratch-mp/L2.C`, `c2.C`) but not for local classes
  (`build/scratch-mp/L1.C`, `L4.C`), including the base-to-derived direction
  that is even implicitly valid. Investigate local-class owner identity in
  `class_subobject_offset`/`make_class_type_from_tok` before the conversion
  rules (`convert_cpp_member_pointer`). Note `L3.C` is not a reproducer: the
  implicit base-from-derived member-pointer conversion it uses is ill-formed.
- `g++.dg/ext/attrib61.C` (`parameter type expected before 'corge'`): distinct
  from the fixed canon-33194 root. The failing shape is an attribute with an
  argument list in a parameter's decl-specifier position before a *function
  declarator* name, `U (__attribute__((unused)) corge) (int);` inside a class;
  start from the parameter `parse_btype`/`type_decl` path in `post_type`
  (`cprimegen.c` ~13339/13369), not from
  `local_paren_starts_direct_initializer`.
- `g++.dg/opt/complex4.C`/`complex5.C` share `';' expected (got 'double')` and
  `g++.old-deja/g++.brendan/complex1.C` the same diagnostic shape; these are
  `_Complex` feature work, like the other complex clusters.
- Remaining clusters are still the whole-feature ones (coroutines 24,
  `vector_size` 7 + 3, `_Complex` 5 + more, `__builtin_object_size` 6,
  `__builtin_va_arg_pack` 5) plus deep template rows.
- `g++.old-deja/g++.mike/p8018.C` (`cannot convert 'struct AccRefA' to
  'struct A *'`): `RefA a3 = aa1;` with `AccRefA::operator RefA&()` and a
  `RefA(A*)` constructor reaches the constructor's argument conversion first
  and reports the failure instead of using the conversion function. This is a
  constructor-versus-conversion-function selection gap in copy-initialization
  (`call_arg_matches_param_type` / `gfunc_param_typed_with_conversions`), not
  the conversion-operator identity defect repaired this cycle.
- `g++.dg/expr/cond6.C` (`true ? f() : b`, `D : B`, copy constructor must run):
  continue from `cpp_try_conditional_prvalue` in `cprimegen_conditionals.inc`.
  The blocked shape is the *prvalue* derived arm joining a base arm; the
  glvalue pair already joins through `cpp_conditional_class_lvalue_type`, and
  the conversion-function pair now joins through the ordinary path. The needed
  piece is a base-from-derived construction that works with an
  implicitly-declared copy constructor, which today fails in the functional
  cast/constructor-call path (`no matching constructor for initialization of
  'B' with 1 arguments` in `cprimegen_lifecycle.inc` around the
  `resolve_member_func_by_arg_types` fallback). Probes: `build/scratch-cond/p5.C`
  line 6, `p6.C` (implicit copy constructor, still failing) versus `cond6.C`
  and `build/scratch-cond/p2.C` (lvalue pair, already passing).
- The class-versus-class conditional arm added this cycle makes
  `gen_assign_cast` report `assignment from incompatible pointer type` for the
  joined branch addresses of a converted class temporary; the value is right,
  but a follow-up should give that join the result type (cv-qualified per
  operand) so the warning disappears.
- Six rows share the `cannot convert 'struct __cpc_member_pointer_*' to ...`
  first diagnostic and are a good next cluster, since the previous cycles
  already localized this area: `g++.old-deja/g++.other/cast7.C`,
  `g++.old-deja/g++.mike/p9206.C`, `g++.old-deja/g++.mike/pmf1.C`,
  `g++.dg/conversion/cast1.C`, `g++.dg/template/pr70466-2.C` and
  `g++.dg/template/ptrmem12.C`. They were not diagnosed this cycle; start from
  `convert_cpp_member_pointer` and the owner/qualifier spelling of the source
  and target member-pointer types (for example `A::i` const/volatile against
  `B::i`, and a member-pointer-to-member-pointer target that nests
  `__cpc_member_pointer_..._func_func_void`).
