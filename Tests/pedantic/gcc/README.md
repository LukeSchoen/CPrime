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

One open lead, for the last coroutine row `g++.dg/coroutines/pr113457.C`. The
row writes `ranges::elements_of(ranges)` after `using namespace std;` inside a
variadic function template and in a promise member body, so its body replay
needs class template argument deduction from the explicit guide
`elements_of(_Range &&) -> elements_of<_Range &&>`, and
`template <range _Range> struct elements_of` needs the `template <Concept T>`
type-constraint spelling, which the template parser currently records as a
value parameter of type `range`. Cycle 58 landed the two lookup repairs in
front of those sites (below); the deduction and the type-constraint spelling
are the remaining work.

The earlier leads were each reduced to one site and are closed: cycle 56 closed
the last of them, the indirect virtual base mem-initializer.

## Retirement queue

Cycle 58 landed two lookup repairs the last coroutine row exposed without
retiring it. A function parameter pack's name is no longer substituted when it
is followed by `::` or preceded by `.`, `->` or `::`, because a
nested-name-specifier ignores variables and a member name after `.` belongs to
the object, so `ranges::elements_of` and `holder.ranges` resolve inside a body
whose pack is also spelled `ranges`. A name a using-directive makes visible is
also recognized when it names a nested namespace: `find_current_namespace_tok`
probed plain symbols only, so `ranges::Wrap<int>` after `using namespace std;`
built the token `__cpc_ns_ranges_Wrap` and reported it undeclared instead of
reaching `std::ranges`. Coverage is
`Tests/features/Templates/pass/test_parameter_pack_name_shadowed_namespace.cpp`
and
`Tests/features/Namespaces/pass/test_nested_namespace_via_using_directive.cpp`;
the previous published compiler rejects both. Measured over all 1554
first-party pass sources, one process per source, with the previous published
compiler against the new one: 1552 accept with both and produce byte-identical
objects, and the two new tests are the only status difference. The row stays,
with its lead recorded above.

Cycle 57 retired the `_Complex` cluster, all three Wave B items at once: the
seventeen `complex*`/`conj*` rows plus `g++.dg/expr/stdarg2.C`,
`g++.dg/opt/pr83608.C`, `g++.dg/tree-ssa/pr50622.C` and
`g++.old-deja/g++.other/debug9.C`, leaving 51 retained rows. `_Complex`,
`__complex__` and `__complex` are type specifiers that combine with any
arithmetic element type, and the type is one canonical two-part aggregate per
element type under a stable tag, so copies, `sizeof`, parameters, results,
arrays, pointers and variadic arguments reuse the ordinary struct paths.
Imaginary constants lex as complex values with a zero real part; arithmetic
lowers element-wise with the usual arithmetic conversions over the real
types; `__real__` / `__imag__` and the `__builtin_creal` / `__builtin_cimag`
/ `__builtin_conj` family select the parts; and overload resolution ranks a
real-to-complex conversion worse than a scalar arithmetic conversion.
Coverage is
`Tests/features/GnuExtensions/pass/test_complex_type_declarations_and_literals.cpp`,
`.../test_complex_arithmetic_and_parts.cpp` and
`.../test_complex_in_classes_and_templates.cpp`; the previous published
compiler rejects all three. The narrowed-lead list is still empty, so the
remaining work is `g++.dg/coroutines/pr113457.C` and the general language long
tail below.

Cycle 56 closed the indirect virtual base mem-initializer lead, the last
narrowed Wave A item 3 lead, which had no retained row of its own.  A
constructor that names a virtual base its class inherits only through another
base found neither a member nor a direct base field for the name, so
`skip_initializer_emit` dropped the whole initializer and the base was
default-initialized instead of taking the written arguments.  The name is now
matched against the class's own virtual-base set, and its initializer is
replayed by the virtual-base section of the most-derived constructor -- the
same section a direct virtual base uses -- so the intermediate base's own
initializer for that base is still ignored at run time.  Zero-initializing a
base subobject no longer covers its virtual base subobjects either, which is
what `[dcl.init]` requires and what the intermediate base's implicit
construction used to clobber.  Coverage is
`Tests/features/Constructors/pass/test_indirect_virtual_base_initializer.cpp`,
which pins the written argument, the single construction of the subobject, the
intermediate base's ignored initializer, a deeper override, and a second
virtual base reached directly.  The previous published compiler runs the test
to exit 1.  The narrowed-lead list is empty now, so the remaining work is the
three wave clusters below: Wave B `_Complex`, Wave C coroutines, and the rest
of the general language long tail.

Cycle 55 closed the `spec7.C` lead, which the consolidation commit had already
removed from the corpus while leaving its linked-member defect in place.  A
concrete qualifier chain ending in a member class template specialization
(`A<int>::B<char>::g`) was only canonicalized for its outer class, so the
following member class template was not resolved under that instantiation and
the out-of-class definition became a namespace-scope function template; a call
on `A<int>::B<char>` then reported the member symbol undefined.  The signature
canonicalizer now carries the just-emitted concrete class token across `::`,
instantiates the member class template under it, and replaces the whole
qualifier prefix with the member specialization's class token, so the ordinary
concrete-member path attaches and replays the definition.  Coverage is
`Tests/features/Templates/pass/test_member_class_template_of_explicit_specialization.cpp`.
The dependent template template lead was already retired by cycle 32; it is
covered by `test_nested_template_name_template_argument.cpp`, which passes in
the pedantic language group, so it is no longer listed as open.

Cycle 54 closed the array-decay lead that followed `qualified-id2.C` and had no
retained row of its own. A qualified id that reaches a static data member
through a member typedef of a class-template instantiation (`B<T>::C::p`) took
the namespace fallback for the segment after the member, and the alias
substitution that maps that member to its scoped alias token then replaced the
whole name, so a comparison that opened a statement or a `?:` condition
compared the class value (`return X::p == X::c ? 0 : 1;` reported `invalid
operand types for binary operation`), while the same comparison behind a `!` or
inside parentheses worked. The `::`-chain loop now resolves that member through
the class it names and continues the chain at the aliased class, so the static
member the alias reaches is looked up under its own joined name. Coverage is
`Tests/features/Templates/pass/test_template_member_typedef_qualified_static_member.cpp`,
which pins the true and false comparisons of two instantiations, the
comparison opening a `?:` condition, an initializer and an `if` condition, the
array element reached through the stored pointer, and the discarded statement
form. The previous published compiler rejects that test with the lead's
diagnostic.

Cycle 52 closed the constant probe's new declarations, the lead that followed
`anon3.C`, which had no retained row of its own. A saved initializer is probed
with the real parser before the emitting replay runs, and the probe kept every
tag and enumerator it met for the first time, so the replay of the same tokens
reported `struct/union/enum 'S' already defined` for `const int ns = sizeof
(struct S { int x; });` (and for the out-of-class static-member form
`const int D::s = ...`) and `redeclaration of 'a'` for the anonymous
`enum { a, b }` form. The probe now records what it defines and the replay
reuses it: the tag body is skipped with the probe's definition standing, and an
enumerator the probe registered at the same scope is adopted instead of pushed
again, while a probe that fails leaves its definitions to the dynamic
initialization replay as before. Coverage is
`Tests/features/Declarations/pass/test_constant_initializer_defines_named_type.cpp`.

Cycle 51 retired `g++.dg/ext/desig11.C`. A body saved for template replay is
rewritten with a lambda introducer marker for every `[` that follows `{`, `,`,
`(` or `=`, so the row's GNU array designators (`const int x[] = { [e] = 0 };
const int y[] = { [I] = 0 };` inside a function template) were replayed as
capture lists and reported `lambda capture 'e' must name an automatic
variable`. The bracket is now left alone when its designator list continues
through further `[index]`/`.field` designators and then `=`, a shape no lambda
introducer can take, so real lambdas keep their marker. Coverage is
`Tests/features/GnuExtensions/pass/test_array_designator_in_template_body.cpp`,
which checks the designated values and untouched slots of a template function
and a class-template member function, a two-dimensional designator chain, and a
capture in the same template body. The previous published compiler rejects that
test with the row's diagnostic.

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
