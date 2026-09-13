# Retained GCC failure corpus

This directory holds only the unresolved rows from upstream GCC tests at
revision `5f6257c26b814de1a14c71b2d3a49291765b6577`
(https://github.com/gcc-mirror/gcc). Every retained case is expected to fail
until it is repaired; established passes were deleted once their behavior moved
into first-party coverage, and git history keeps the removed sources.

`corpus.json` lists and hashes the retained cases; supporting headers live
beside them and are hashed separately so they are never discovered as tests.

## Commands

```powershell
./Tests/pedantic/gcc/run.ps1 -List
./Tests/pedantic/gcc/run.ps1 -Select g++.dg/template/access27.C -Out build/gcc-exact
./Tests/pedantic/gcc/run.ps1 -ContinueAfterTimeout -Out build/gcc-retained
```

The default fast tier selects every retained case. The tier partition in
`Tests/tiers.json` is still consulted so a repaired row can be removed from the
corpus without disturbing the rest; there is no pedantic GCC partition while
every retained row is unresolved, and `-Tier pedantic` is a no-op.

The adapter passes `-fcoroutines` for `g++.dg/coroutines`, matching GCC's
`coroutines.exp` default options.

`-CompilerPath` and `-RuntimeRoot` select the compiler and runtime. Each
compiler and program invocation has a five-second ceiling; crashes and timeouts
fail and are never retried with a larger budget. An unknown `-Select` fails.
`compare.ps1 -Baseline <old> -Current <new>` reports regressions and lost
coverage within the same corpus.

`summary.json`, `progress.json`, `metadata.json`, `inputs.json` and
`results.jsonl` under the output directory record results and provenance. The
score is verified passes over the selected retained cases; it is not GCC or
C++ conformance.

Focused adapter checks: `test_runner.ps1`, `test_compare.ps1`,
`test_progress.ps1`, `test_provenance.ps1`, `test_corpus.ps1`.

The repair order and row buckets live in [task.md](../../../task.md) under the
wave plan; repaired rows are deleted rather than promoted. The narrowed leads
for the remaining clusters are below.

## Narrowed leads

Each lead is already reduced to one site and is the shortest path for the
matching cluster:

- Dependent template template argument: `T::template AA<U>::template B` names
  the nested template but loses the enclosing argument, so
  `chain<outer<char>, char>` still builds `middle<int>::inner` (`sizeof` stays 4
  where the `typename` spelling gives 1).
- Member function template of a member class template specialization:
  `template<> template<> template<class V> void A<int>::B<char>::g(V) { }` is
  still a namespace-scope template, so a linked call to `g` is undefined
  (`spec7.C` is compile-only; probe a call on `A<int>::B<char>`).
- Indirect virtual base mem-initializer is dropped (`skip_initializer_emit`) and
  default-initialized instead; cycle 17 only split base-object/complete
  (`*__base`) variants.
- Array decay: `return X::p == X::c ? 0 : 1;` reports `invalid operand types for
  binary operation` where the comparison opens a statement or `?:`;
  `if (!(...))` works.
- New type in a static member initializer: the definition copy is complete now,
  but the constant probe replays the initializer in the member's class scope,
  so a name the initializer declares is registered twice.
  `const int D::s = sizeof (struct S { int x; });` reports
  `struct/union/enum 'S' already defined` and
  `const int D::s = sizeof (enum { a, b });` reports `redeclaration of 'a'`,
  while an anonymous union or struct body with named members is unaffected
  because its second definition is not name-visible (the retired `anon3.C`
  compiled for that reason).

## Retirement queue

Cycle 50 retired `g++.old-deja/g++.martin/sts_iarr.C`. The terminal name of a
qualified id written inside a class-template member body belongs to the
qualifier in front of it, but both body replays redirected every identifier
that named a member of the instantiation being replayed, so
`typename Outer<N-1>::Inner` reached the lookup of `Outer<1>` spelled as
`Outer<2>`'s joined member name and reported `nested template type member
'Outer____cpc_template_const_1::Outer____cpc_template_const_2_Inner' must be a
typedef`. The class-body member-body rewrite and the member-template body
substitution now keep the written spelling after a `::` -- the same rule the
class-body-level replay already applied -- while still substituting the
template's own parameters, and the joined-name probe keeps running so token
numbering and object bytes are unchanged. Coverage is
`Tests/features/Templates/pass/test_qualified_nested_class_in_member_body.cpp`,
which runs the row's nested `operator[]` chain down to `Outer<1>::Inner`,
checks the injected class name qualified through the current instantiation,
pins the member-typedef alias shape and a member function template of the
class template. The previous published compiler rejects that test with the
row's diagnostic.

Cycle 49 retired `g++.old-deja/g++.ext/anon3.C`. An out-of-class static data
member definition is copied into the member's class scope before it is parsed,
and the copy ended at the first `;` at any depth. A new type in the initializer
carries a `;` inside its own body (`sizeof (struct { int x; })`), so the copied
declaration was truncated and the replay failed with `unexpected end of file`;
the row's `(union { ... }){ __c: { ... } }` initializer hit the same `;`.  The
copy now tracks `(`/`[`/`{` groups and ends at the definition's own `;`.
Coverage is
`Tests/features/Classes/pass/test_static_member_initializer_defines_type.cpp`,
which checks the row's infinity constant plus a `sizeof (struct { ... })`
member and a lambda-bodied member at run time, and pins that the initializer's
new type is not injected into the class.  The previous published compiler
rejects that test with the row's diagnostic.

Cycle 48 retired `g++.old-deja/g++.ext/syshdr1.C` and `g++.dg/parse/redef1.C`.
A GCC linemarker's trailing numeric flags were discarded, so the `3` bit that
marks system-header text was not visible to the parser and `typedef int bool;`
was read as two basic type specifiers.  Buffered files now retain that flag,
and system-header typedefs of `bool` and `wchar_t` are accepted without
replacing the builtin types.  Coverage is
`Tests/features/GnuExtensions/pass/test_system_header_builtin_typedefs.cpp`,
which checks the `int` and `long wchar_t` forms through flagged linemarkers,
returns to the source file, and verifies the builtin sizes and runtime behavior.

Cycle 46 retired `g++.dg/template/canon-type-3.C`.  A class-template member
typedef may name a function type through a parenthesized declarator
(`typedef Y (FP) ();`), and the class-body generator read the identifier in
front of the group -- a template parameter here -- as the declared name, so
the replayed definition named an undeclared type and the instantiation failed
with `function parameter type expected (got 'FP')`.  Both class-body scans now
take the group's own identifier as the name when the identifier before the `(`
resolves to a type, and keep the depth-zero name for a parameter list.
Coverage is
`Tests/features/Templates/pass/test_parenthesized_function_typedef_member.cpp`,
which instantiates the row's `E<Y>` and a member-typedef-typed variant and
calls through `E<int>::FP` and `D<int, char>::FP`.

Cycle 45 retired `g++.dg/template/access28.C`. A static member function template
declared inside its class and defined out of line is two member records that
instantiate the same specialization, so `&grac::once<Derived>` looked
ambiguous to the address path, which has no call to rank and required exactly
one candidate. The address path now resolves each candidate to its function
symbol and accepts them while they agree, still failing when two distinct
functions match. Coverage is
`Tests/features/Templates/pass/test_static_member_template_address.cpp`, which
takes the address in a class-template member-initializer, calls through the
stored pointer and keeps the row's `has_R<T>` SFINAE overload selection over
the ellipsis fallback.

Cycle 43 retired `g++.old-deja/g++.jason/template25.C` and
`g++.dg/opt/pr6713.C`.  A class-template member definition may put its
declarator-id on the line after `Class<T>::`.
The member-class recognizer and `template_member_def_method_tok()` both read
the line record as the member name, so the definitions became namespace-scope
function templates and calls left undefined symbols (or replay reported
`Foo::<no name>`).  Both scans now advance with the line-aware next-token
helper, including after `::` for operators and destructors.  Coverage is
`Tests/features/Templates/pass/test_member_definition_line_break.cpp`, which
also pins the constructor and static-member forms fixed in
`g++.dg/opt/pr6713.C`.

Cycle 41 retired `g++.old-deja/g++.jason/synth7.C`. An implicit copy assignment
had an inline memberwise path but no addressable symbol, so `&A::operator=`
failed on the synthetic `A_operator=` name. The member-pointer path now
materializes the implicit copy assignment when the class has no user-declared
copy assignment, no user-declared move operation, and memberwise-assignable
data and bases; it reuses the `= default` declaration and body path, and the
ordinary member-address lookup forms the pointer. Coverage lives in
`Tests/features/OperatorOverloads/pass/test_implicit_copy_assignment_address.cpp`.

Cycle 39 retired `g++.dg/template/conv1.C`.  A qualified conversion-operator
name hides its member template argument list inside the conversion-type-id, so
`&First<D>::operator First<B>` reached the class-template-argument path with
`operator` itself as the member name and never saw the `<B>`.  The path now
parses the whole operator name, deduces the conversion member template against
the written target type, and publishes the selected specialization under the
spelled target -- the same name a non-template conversion function uses -- so
the member-address lookup finds it.  Coverage lives in
`Tests/features/Templates/pass/test_conversion_template_member_address.cpp` and
..._declarations.cpp.

The coroutine rows `-fcoroutines` and the shipped `<coroutine>` already
unblocked were repaired rows waiting for coverage, not failures. Cycles 37-38
retired all 39 of them by reducing what each pins to a first-party test in
`Tests/features/Declarations/pass/` (with `EXPECT_COMPILE_ARGS: -fcoroutines`
and, for a snippet without `main`, `EXPECT_COMPILE_ONLY: 1`) and deleting the
corpus file and its `corpus.json` entry; the tests stay in the fast tier
because `Tests/tiers.json` lists only the pedantic partition. Cycle 37 retired
nine rows (`coro-pre-proc.C`, `coro-function-decl.C`, `pr95346.C`,
`pr95350.C`, `pr95822.C`, `pr95823.C`, `pr95824.C`, `pr102051.C`,
`pr109283.C`) and cycle 38 retired the remaining 30, including the
coroutine-only `coro.h` and `coro1-ret-int-yield-int.h` support headers. Both
mappings are recorded in `BuildProfile/README.txt`. The only retained coroutine
row left is `g++.dg/coroutines/pr113457.C`, which still fails and belongs to
Wave C.
