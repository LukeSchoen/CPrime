# Remaining work

Target: 100% of the retained GCC language rows and the first-party pedantic
corpus, with no missing coverage, weakened expectations, compiler internal
errors or timeout retries.

State (2026-09-13, cycle 58): the retained corpus is 51 rows -- 0
PASS_COMPILE, 51 FAIL_COMPILE -- with 0 fast-tier regressions, and the
pedantic first-party list is empty: every first-party case passes at both
tiers (`Tests/run-all.ps1 -Tier fast`, `Tests/run-all.ps1 -Tier pedantic`).
Cycle 58 landed the two lookup repairs the last coroutine row exposes, without
retiring the row. A function parameter pack's name is no longer substituted
when it is followed by `::` or preceded by `.`, `->` or `::`: a
nested-name-specifier ignores variables and a name after `.` belongs to the
object, so `ranges::elements_of` and `holder.ranges` resolve inside a body
whose pack is also spelled `ranges`. A name a using-directive makes visible is
also resolved when it names a nested namespace, so `ranges::Wrap<int>` after
`using namespace std;` reaches `std::ranges` instead of the unqualified token
`__cpc_ns_ranges_Wrap`. Coverage is
`Tests/features/Templates/pass/test_parameter_pack_name_shadowed_namespace.cpp`
and
`Tests/features/Namespaces/pass/test_nested_namespace_via_using_directive.cpp`;
the previous published compiler rejects both, and over all 1554 first-party
pass sources the 1552 that both accept produce byte-identical objects.
`g++.dg/coroutines/pr113457.C` therefore stays as the one open Wave C item 2
row with a recorded lead: its `ranges::elements_of(ranges)` calls need class
template argument deduction from the explicit guide, and its
`template <range _Range> struct elements_of` needs the `template <Concept T>`
type-constraint spelling, which the template parser records as a value
parameter of type `range`.
Cycle 57 retired the whole `_Complex` cluster, all three Wave B items at once,
as 21 rows: the 17 `complex*`/`conj*` rows plus `g++.dg/expr/stdarg2.C`,
`g++.dg/opt/pr83608.C`, `g++.dg/tree-ssa/pr50622.C` and
`g++.old-deja/g++.other/debug9.C`. `_Complex`, `__complex__` and `__complex`
are now type specifiers that combine with any arithmetic element type, and the
type itself is one canonical two-part aggregate per element type under a
stable tag the native mangler can use, so aggregate copies, `sizeof`,
parameters, results, arrays, pointers and variadic arguments all reuse the
ordinary struct paths. Imaginary constants (`0i`, `90i`, `2.0i`, `2.0fi`,
`3.0Li`) lex as complex values with a zero real part. Arithmetic lowers
element-wise on the two parts with the usual arithmetic conversions over the
real types, so `+`, `-`, `*`, `/`, unary `-`, the compound assignments,
equality and the zero tests all work on mixed complex/real operands, and a
real value converts to a complex one while a complex value converts to
another complex element type or to `bool`. `__real__` and `__imag__` select
the corresponding part as an lvalue, `__real__` of a non-complex class
applies its conversion to int, and `__builtin_creal`, `__builtin_cimag` and
`__builtin_conj` (with the `f`/`l` forms) lower onto the same parts.
Overload resolution treats a real-to-complex conversion as viable but worse
than any scalar arithmetic conversion, so `f(1)` prefers `f(double)` while
`g(1)` still binds to `g(_Complex int)`. Coverage is
`Tests/features/GnuExtensions/pass/`:
`test_complex_type_declarations_and_literals.cpp` (spellings, element types,
layout, `new`, the literal forms, element conversions, the two overloads),
`test_complex_arithmetic_and_parts.cpp` (all operators, mixed operands,
compound assignment, comparison, the part lvalues, the builtins, aggregate
members, a class with complex members and operators, a nested try/catch and a
const complex pointer) and `test_complex_in_classes_and_templates.cpp` (a
class-template specialization whose constructor writes the parts,
`__real__` of a class with a conversion operator, a template writing through
a `T*` alias, and a struct with a complex member through `va_arg`); the
previous published compiler rejects all three. Measured over the 182 fast-tier
pass sources with the previous published compiler against the new one: 179
compile with both, the three new tests are the only status difference, and of
those 179 objects five differ only in generated `__cpc_*` local-class and
coroutine names whose numbering follows the token table (all other symbols and
bytes are identical). The narrowed-lead list held nothing before cycle 58, so
the next open cluster was Wave C item 2, the remaining
`g++.dg/coroutines/pr113457.C` row, which now carries the lead cycle 58
recorded, followed by the general language long
tail. Cycle 56 closed the last narrowed Wave A item 3 lead, the indirect
virtual base mem-initializer, which had no retained row of its own. A
constructor that names a virtual base its class inherits only through another
base had the initializer dropped (`skip_initializer_emit`) because the name is
neither a member nor a direct base field, so the base was default-initialized
instead of taking the written arguments; a virtual base is now resolved
through the class's own virtual-base set, and its initializer is replayed in
the complete-object section the constructor already keeps for the virtual
bases it does have a base field for, so only the most-derived constructor
establishes the subobject. The same class's implicit construction now also
leaves that subobject alone: zero-initializing a base subobject zeroes its
non-virtual regions only, as `[dcl.init]` requires. Coverage is
`Tests/features/Constructors/pass/test_indirect_virtual_base_initializer.cpp`,
which pins the written argument, the single construction, the intermediate
base's ignored initializer, and the deeper override; the previous published
compiler runs it to exit 1. Cycle 55 retired the `spec7.C` narrowed Wave A
item 3 lead, which
the consolidation commit had already removed from the corpus while leaving its
linked-member defect in place: an out-of-class
`template<> template<> template<class V> void A<int>::B<char>::g(V) { }`
definition was registered as a namespace-scope function template because
signature canonicalization stopped at `A<int>`; the canonicalizer now resolves
the following member class template under that instantiation and rewrites the
whole qualifier prefix to the member specialization's class token. Coverage is
`Tests/features/Templates/pass/test_member_class_template_of_explicit_specialization.cpp`;
the previous published compiler fails it at link with the undefined `combine`
symbol. The dependent template template lead was already covered by
`test_nested_template_name_template_argument.cpp` in cycle 32, and cycle 56
closed the lead that followed it. Cycle 54 closed the
array-decay narrowed Wave A item 3 lead: a
qualified id that reaches a static data member through a member typedef of a
class-template instantiation (`B<T>::C::p`) now continues at the class the
typedef names instead of letting the typedef's scoped alias token stand in for
the whole name, which is what made a comparison that opened a statement or a
`?:` condition report `invalid operand types for binary operation`; cycle 52
closed the narrowed lead before it -- the
constant probe's new declarations, which had no retained row of its own
because it was narrowed out of the retired `anon3.C` -- by letting the
emitting replay of a saved initializer reuse the tag or enumerator the probe
of the same declaration already defined; cycle 51 retired
`g++.dg/ext/desig11.C` by keeping a GNU array designator out of the saved-body
lambda rewrite. Pre-step 2 is closed (see
below); cycle 37 retired the first nine
coroutine rows into first-party coverage and cycle 38 retired the remaining 30
retained rows that `-fcoroutines` already compiled. Cycle 19 finished pre-step 1
by repairing the last four first-party failures. A range-for over a member read
from a const object takes the const element type; the
coroutine syntax case is a runnable case whose `co_yield`/`co_return` promise
lowering is observed; a by-value parameter is read alive inside its own full
expression, matching the pinned temporary-lifetime case; and a named local
returned by a trailing `return <local>;` is now constructed in the caller's
result object instead of being copied out of the frame.

Cycle 39 retired `g++.dg/template/conv1.C`, the first of the narrowed Wave A
item 3 leads. A qualified conversion-operator name keeps its member template
argument list inside the conversion-type-id, so `&First<D>::operator First<B>`
reached the `Class<T>::` path with `operator` itself as the member name (the
diagnostic named the synthetic `First__D_operator`) and the non-template path
looked for an uninstantiated `D_operator Second < int >`. Both qualified paths
now parse the whole operator name, deduce the conversion member template
against the written target type, and publish the selected specialization under
the spelled target -- the name a non-template conversion function already
uses -- before the member-address lookup runs. Coverage is
`Tests/features/Templates/pass/test_conversion_template_member_address.cpp`
(typed addresses plus direct-call values) and
`test_conversion_template_member_address_declarations.cpp`, which keeps the
upstream shape: incomplete `Second`, undefined conversions and a `First<T>::Foo`
method beside them.

Cycle 41 retired `g++.old-deja/g++.jason/synth7.C`. An implicit copy assignment
was usable inline (`a = b`) but had no function symbol, so `&A::operator=`
resolved the synthetic `A_operator=` name and reported it undeclared. When a
member address names `operator=`, the member-pointer path now materializes the
implicit copy assignment -- guarded to classes with no user-declared copy
assignment, no user-declared move operation, and memberwise-assignable data and
bases -- through the same `declare_member_func` and
`queue_defaulted_assignment_body` path as `= default`; the existing lookup then
forms the pointer. Coverage is
`Tests/features/OperatorOverloads/pass/test_implicit_copy_assignment_address.cpp`,
which takes `&Owner::operator=` and checks the memberwise copy through the
pointer.

Cycle 43 retired `g++.old-deja/g++.jason/template25.C`. A class-template member
definition may put its declarator-id on the line after `Class<T>::`; the
line-number record made both the member-class recognizer and
`template_member_def_method_tok()` read past the qualifier as a name, so the
definitions became namespace-scope function templates and calls to the
instantiated members were left undefined (or replay reported
`Foo::<no name>`). Both scans now advance with the line-aware next-token
helper, including after `::` for operators and destructors. Coverage is
`Tests/features/Templates/pass/test_member_definition_line_break.cpp`, which
selects both dependent overloads and checks the selected body. The same
repair retired `g++.dg/opt/pr6713.C`, whose class-template constructor and
static member definition both put their declarator-id after that line break;
the regression test covers both lifecycle and static forms.

Cycle 45 retired `g++.dg/template/access28.C`. `&Class::member<Arg>` was
reported as `no matching static member-template call` whenever the member was
declared inside its class and defined out of line. Those are two
`TemplateMemberDef` records -- one for the in-class declaration and one for the
definition -- and both instantiate the same specialization, so the address
selection, which has no call to rank, saw two candidates for one function and
its `candidate_count == 1` test rejected the address. The address now resolves
each candidate to its function symbol and accepts them when they all name the
same symbol; two distinct functions still fail, which is an address no target
type can choose. The row's other half is unchanged behavior: the class-template
member-initializer forms the address from a dependent argument, and the
`has_R<T>` SFINAE overload selected by `setR(&msg)` still wins over the
ellipsis fallback. Coverage is
`Tests/features/Templates/pass/test_static_member_template_address.cpp`, which
takes the address in a class-template member-initializer, calls through the
stored pointer and checks the selected `setR` overload.

Cycle 46 retired `g++.dg/template/canon-type-3.C`. A class-template member
typedef may name a function type through a parenthesized declarator
(`typedef Y (FP) ();`), and the class-body generator that builds the
instantiated definition read the identifier in front of the group as the
declared name. It published `Y` -- here a template parameter -- under the
instantiation's joined name and rewrote the member typedef with it, so the
replayed definition named an undeclared `E__int__Y` where its type belongs,
the declaration fell back to the implicit-int path, and instantiating the
enclosing template reported `function parameter type expected (got 'FP')`.
Both class-body scans (`materialize_template_class_typedef()` and the
specialization body builder)
now take the group's own identifier as the name whenever the identifier before
the `(` resolves to a type -- a template's own type parameter, a
specialization alias, a typedef, a class/enum or a class template -- and keep
the depth-zero name for a parameter list (`typedef int name(Arg);`). Coverage
is `Tests/features/Templates/pass/test_parenthesized_function_typedef_member.cpp`,
which instantiates the row's `E<Y>` and a member-typedef-typed variant, then
names `E<int>::FP` and `D<int, char>::FP` from outside and calls through both
pointers.

Cycle 48 retired `g++.old-deja/g++.ext/syshdr1.C` and `g++.dg/parse/redef1.C`.
GCC linemarker flags were discarded after the file name, so the `3` bit that
marks system-header text never reached the parser and `typedef int bool;` was
parsed as two basic type specifiers. Buffered files now retain the
system-header bit, and typedefs of `bool` and `wchar_t` in such headers are
accepted without replacing the builtin types. Coverage is
`Tests/features/GnuExtensions/pass/test_system_header_builtin_typedefs.cpp`,
which checks the `int` and `long wchar_t` forms, returns to the source file, and
verifies both builtin sizes and runtime behavior.

Cycle 49 retired `g++.old-deja/g++.ext/anon3.C`. An out-of-class static data
member definition is copied into the member's class scope before it is parsed,
and that copy ended at the first `;` at any nesting depth. An initializer that
creates a new type puts a `;` inside its own body (`sizeof (struct { int x; })`),
so the copied declaration was truncated and the replay reported `unexpected end
of file`; the row's union compound literal with an obsolete designated
initializer hit the same `;`. The copy now tracks `(`/`[`/`{` groups and stops
at the definition's own `;`, so a lambda body, a brace initializer and a new
type in a static member initializer all survive the copy. Coverage is
`Tests/features/Classes/pass/test_static_member_initializer_defines_type.cpp`,
which checks the row's `(union { ... }){ __c: { ... } }` infinity constant, a
`sizeof (struct { ... })` member, and a lambda-bodied member at run time, and
pins that the initializer's new type is not injected as a class member.
Measured over all 1585 first-party pass sources with the previous published
compiler against the new one, one process per source: the 1582 that both
accepted produced byte-identical objects, two failed for both with the same
diagnostics (the coroutine header test needs its flag and the heap-list
performance test lacks `GetTickCount64`), and the only status difference is the
new regression test, which the previous compiler rejects with the row's
diagnostic.

Cycle 51 retired `g++.dg/ext/desig11.C`. A function or member body that is
saved for template replay is rewritten so every `[` that follows `{`, `,`,
`(` or `=` becomes a lambda introducer marker, because the replay has no way
to re-read the original spelling. A GNU array designator in a braced
initializer has exactly that shape, so `const int x[] = { [e] = 0 };` inside a
template body was replayed as a capture list and reported `lambda capture 'e'
must name an automatic variable` (`__cpc_template_const_0` for a non-type
template parameter); the same statement outside a template parsed fine. The
rewrite now leaves the bracket alone when the designator list it opens
continues through further `[index]`/`.field` designators and then `=`, which
is the one shape a lambda introducer cannot take, so real lambdas keep their
marker. Coverage is
`Tests/features/GnuExtensions/pass/test_array_designator_in_template_body.cpp`,
which checks a template function's designated values and untouched slots, a
class-template member function's designated slot and two-dimensional
`[e][1] = 5` chain, and that a capture in the same body still works.

Cycle 52 closed the constant probe's new declarations, the last narrowed Wave A
item 3 lead, which had no retained row of its own: it was narrowed out of the
retired `anon3.C`. A saved initializer is probed with the real parser before
the emitting replay runs, and the probe kept every tag and enumerator it met
for the first time, so the replay of the same tokens reported
`struct/union/enum 'ProbeS' already defined` for
`const int struct_size = sizeof (struct ProbeS { int a; double b; });` and
`redeclaration of 'a'` for the anonymous `enum { a, b }` form; the same
statements inside a function body, whose initializers are not probed, were
already fine. The probe now records what it defines (a `probe_defined` flag on
the tag or enumerator binding, tracked per declaration in
`cprimegen.c`) and the emitting replay reuses it -- the tag body is skipped
with the probe's own definition standing, and an enumerator the probe
registered at the same scope is adopted instead of pushed again -- while a
probe that fails leaves its definitions to the dynamic initialization replay,
exactly as before. Genuine redefinitions outside an initializer still report
`already defined`. Coverage is
`Tests/features/Declarations/pass/test_constant_initializer_defines_named_type.cpp`,
which pins the namespace-scope named struct and enum counts and enumerator
values, the anonymous-enum form, both out-of-class static-member shapes, and an
aggregate initializer's nested type, each of them visible and usable after the
definition. The previous published compiler rejects the test with the lead's
diagnostic. Measured over all 1630 first-party pass sources with the previous
published compiler against the new one, one process per source: the 1628 that
both accepted produced byte-identical objects, the other 2 failed for both,
and no status or diagnostic differs. A known remaining hole in the same
recovery path: a *genuine* redefinition inside an initializer (`struct S { int
x; }; const int n = sizeof (struct S { int y; });`) is still swallowed by the
probe's substitution recovery and replayed as a function-local definition
instead of being reported, and the same statement with the tag defined by the
same initializer now reaches that path as well.

Pre-step 2 has eleven landed changes, each measured on this machine against the
compiler it replaced and each codegen-neutral over the first-party sources that
compile standalone. Cycle 19 bucketed saved inline-expansion bodies by callee
name and gated `find_cpp_this_symbol()`'s receiver scan on any
`__cprime_this_` identifier having been interned: the self-compile workload went
from 0.469 s to 0.390 s (median of 7 runs, -16.8%) and `cpp.call.source.lookup`
from 18.640 ms to 15.055 ms. Cycle 20 cached the `CPRIME_TRACE_INCOMPLETE`,
`CPRIME_DUMP_AUTORET` and `CPC_TRACE_RETURN` gates: each gate sits in a
per-instantiation path and rescanned the whole environment block per call, which
the sampling profiler had charged 9.4% of the self-compile to. Against the
compiler it replaced the self-compile workload went from 0.411 s to 0.358 s
(-13.0%), `cpp.template.static.lookup` from 25.958 ms to 23.228 ms (-10.5%), the
`-E` control is unchanged, and the sampler shows no unmapped DLL bucket above
1.3%. Cycle 22 turned the cached `is_cpp_translation_unit()` check into a macro
with a one-time `_slow` filename probe, removing an out-of-line call from a
pervasive frontend path; the 12-compilation in-process self-compile workload
went from a 4118 ms median to 3953 ms (-4.0%), the single-compile median from
344 ms to 328 ms, and the compiled object is byte-identical. Numbers live in
`BuildProfile/README.txt`. Cycle 23 remembered the fixed spellings the C++
front end re-interns while resolving names (`this`, `operator=` and the other
operator spellings, `<no name>`, `__cpc_lexical_types`, `__cprime_vptr`): 217k
`tok_alloc_const()` calls per compilation, 92% of the calls the profiler
charged `tok_alloc`, became a compare. The 12-compilation in-process
self-compile workload went from a 4047 ms median to 3906 ms (-3.5%), the
single-compile `-bench` median from 336 ms to 313 ms (-6.8%), and the cache is
still filled at the first use of each spelling, so all 1534 first-party sources
that compile standalone and the self-compile object stay byte-identical.
Cycle 24 stopped the C front end from paying for C++ name resolution it cannot
use, with gates that read only the state the parser sets for the construct being
skipped: `find_current_namespace_tok_ex()` answers while no binding redirects a
name and no namespace state exists, `find_current_class_nested_type_tok()`
answers while no member class is active and no member declarator is being
parsed, `find_class_template_def()` and `find_function_template_def()` return
NULL while no template has been registered, and `gexpr()` skips the
comma-operator lookups until an `operator<op>` spelling has been parsed. The
12-compilation in-process self-compile workload went from a 3890 ms median to
3735 ms (-4.0%), the wall median from 3897 ms to 3726 ms (-4.4%), the
single-compile `-bench` median is unchanged inside its 15.6 ms tick, and the
1576 first-party tests that compile standalone plus the self object stay
byte-identical. Gates keyed on the source filename were tried first and
rejected: this parser also accepts `namespace`, `using`, `class` and operator
declarations in a `.c` file.
Cycle 25 removed repeated expression-spelling work from the C front end:
`unary()` derives an identifier's spelling once, directly from `table_ident`,
and gates the `typename`, named-cast and `delete` probes on the spelling's
first character; `cpp_type_trait_name_tok()` rejects names that do not begin
`__i`/`__h` before its ten string comparisons; and `get_tok_str()` no longer
resets its scratch buffer on the identifier fast path. The 12-compilation
in-process self-compile workload went from a 3756 ms wall median to 3690 ms
(-1.8%) and a 3735 ms inner-time median to 3672 ms (-1.7%), 6 interleaved
pairs, and the compiled self object is byte-identical.

Cycle 26 cut repeated identifier-table and skip-loop work. `find_cpp_this_symbol()`
answers immediately while no 'this' or lambda-'this' binding has been linked,
which removes two `sym_find()` probes (each a random `table_ident` load plus a
TokenSym deref) from every call expression that cannot have a receiver;
`sym_free()` no longer forwards its almost-always-NULL `field_index` through
the reallocator hook; `next()` takes the macro binding from the TokenSym
`next_nomacro()` just interned instead of probing `table_ident` again; and
`preprocess_skip()` advances over runs of bytes its switch does not handle
specially instead of dispatching once per skipped byte. The 12-compilation
in-process self-compile workload went from a 3715.2 ms to a 3638.0 ms wall
median (-2.08%, 8 interleaved pairs, every pair favouring the new compiler);
the unpacked pair measured 3605.7 to 3511.3 ms (-2.62%), and `preprocess_skip`
alone fell from 1.88% to 1.33% of the profile. All 1601 first-party pass-tier
sources compiled with both compilers: the 1599 that both accepted produced
byte-identical objects and diagnostic text, and the other 2 failed for both;
the added `Tests/c_compat/pass/test_skipped_block_text_scan.c` pins the bytes
that scan walks past.
Replacing the identifier comparison's `memcmp` with an inline byte loop was
measured and rejected (-0.5%): this codegen keeps loop variables in memory and
ucrtbase's `memcmp` is already cheap for short strings.

Cycle 27 shortened the identifier-hash dependency chain in `next_nomacro`.
Replacing `h + (h << 5) + (h >> 27) + c` with `h * 31 + c` compiles each
per-byte update to one multiply and one add instead of a chain of shifts and
adds, while keeping the full-spelling hash used by `tok_alloc()` and the
identifier table. Sixteen interleaved `cpc.exe @build/prof-self.rsp` pairs with
the order alternated measured a 3547 -> 3516 ms inner-time median (-0.87%) and
a 3542.2 -> 3523.4 ms mean (-0.53%); the wall median went from 3553.8 to
3530.0 ms (-0.67%), with 10 wins, 3 losses and 3 ties. The first-party
small-case workload is process-startup dominated and stayed inside its spread:
c.integer.compile.stress 10.433 -> 10.007 ms, c.empty.main 10.082 -> 10.169 ms,
cpp.call.source.lookup 14.505 -> 14.254 ms,
test_conversion_template_owner_lookup 18.434 -> 18.310 ms,
cpp.template.static.lookup 22.490 -> 22.415 ms and
cpp.template.member.list 12.560 -> 12.970 ms. A sampled O(1) hash was 1.75%
slower from extra bucket collisions; a shift-xor hash was 45% slower because
its low bits no longer depended on the prior hash; batching the byte emitters
in `o()`/`gen_le*()` was 1.8% slower; and inlining `get_tok_str()`'s identifier
fast path was neutral. All four were reverted. All 1551 standalone first-party
pass sources that both compilers accepted produced byte-identical objects, and
the remaining one failed for both.
Cycle 28 gave the identifier scan one classification byte per character.
`set_idnum()` now derives `ident_cont[256]`, whose entry c holds c while c can
continue an identifier and 0 otherwise, so the fast path in `next_nomacro()`
uses that byte as both the loop test and the hash addend instead of loading
`isidnum_table`, masking `IS_ID|IS_NUM` and reloading the character. Twelve
interleaved unpacked pairs of `cpc.exe @build/prof-self.rsp` measured a
3516 -> 3500 ms inner-time median (-0.46%) and a 3521.0 -> 3494.8 ms mean
(-0.74%); the wall median went from 3527 to 3505 ms (-0.62%), with 9 wins, 2
losses and 1 tie on inner time and 11 wins and 1 loss on wall. The
process-start-dominated small cases stayed inside this host's spread
(c.integer.compile.stress 10.450 -> 10.336 ms, c.empty.main 10.192 -> 9.964 ms,
cpp.call.source.lookup 14.056 -> 14.514 ms,
test_conversion_template_owner_lookup 18.444 -> 18.424 ms,
cpp.template.static.lookup 22.781 -> 22.792 ms and
cpp.template.member.list 13.098 -> 12.938 ms). All 1602 first-party pass-tier
sources were compiled with the old and the new compiler; the 1599 that both
accepted produced byte-identical objects and the other 3 failed for both with
byte-identical diagnostics.
Cycle 31 removed two repeated name-probe passes. Temporary counters put the
self-compilation at 945,882 `get_tok_str()` calls, of which 449,504 were the
two `std::is_`/`__cpc_ns_std_is_` qualifier tests (112,376 evaluations at each
of four sites) and about 310,000 were context-spelling tests against
`static_assert`, `using`, `typename`, `operator[]`, `template`, `friend` and
`wchar_t`. `next_nomacro()` now records the id of each of those spellings as
the lexer interns it, which makes the spelling test one integer compare while
keeping the identifier numbering the source itself produces -- interning the
literal at the first probe instead renumbers the identifiers in between, and
those numbers appear in synthetic `__cpc_bound_type_<n>` names. The two
qualifier tests now evaluate `tok == TOK_LT || tok == '<'` first, which drops
the 449,504 probes to 7,224. Twenty interleaved unpacked pairs of
`cpc.exe @build/prof-self.rsp` with the order alternated measured a
3531 -> 3500 ms inner-time median (-0.88%) and a 3526.6 -> 3492.1 ms mean
(-0.98%); the wall median went from 3525 to 3501 ms (-0.68%) and the mean from
3533.6 to 3498.2 ms (-1.00%), with 19 wins, 1 loss and 0 ties and a paired
delta median of -31 ms. Against that base the qualifier reorder alone measured
-0.90% median over 10 pairs and the spelling ids another -0.44% median over 14
pairs. All 1602 first-party pass-tier sources were compiled with both
compilers: the 1595 that both accepted produced byte-identical objects, the
other 7 failed for both with byte-identical diagnostics, and the self object is
byte-identical.
Cycle 32 finished the spelling-id work by recording the five spellings the
expression path still probed by name. Temporary counters put one
self-compilation at 199,192 `unary()` calls, of which 112,400 derived
`table_ident[tok - TOK_IDENT]->str` and compared it against `typename`,
`static_cast`, `reinterpret_cast`, `const_cast`, `dynamic_cast` and `delete` --
two dependent random loads plus up to three `strcmp()` probes per name token,
in a C translation unit where none of those spellings can appear. Those five
ids are now recorded as the lexer interns them, next to cycle 31's seven, and
`unary()` tests the id instead, so the same spellings still match when the
source really spells them (including in a `.c` file, where the old `strcmp()`
matched too). Eight interleaved unpacked pairs of `cpc.exe @build/prof-self.rsp`
measured a 3500 -> 3445 ms inner-time median (-1.57%) and a 3496.1 -> 3439.4 ms
mean (-1.62%), 8 wins and 0 losses; a further twelve pairs measured a
3508 -> 3484 ms median (-0.68%) and a 3510.4 -> 3480.3 ms mean (-0.86%), 10 wins
and 2 losses, 18 wins and 2 losses over the session. The same workload run
through the packed pair (previous published compiler -> new published compiler)
measured a 3532 -> 3508 ms median (-0.68%) and a 3521.4 -> 3494.3 ms mean
(-0.77%) over eight pairs. All 1602 first-party pass-tier sources were compiled
with the previous published and the new published compiler: the 1600 that both
accepted produced byte-identical objects and the other 2 failed for both with
byte-identical diagnostics.

Cycle 33 removed the per-byte classification mask from the tokenizer's
whitespace runs. `set_idnum()` already derives `ident_cont[256]` from
`isidnum_table` for identifier scanning; it now derives `ident_space[256]` in
the same place, with entry c set to 1 while c is `IS_SPC`, and
`next_nomacro()`'s horizontal-space loop tests that byte directly instead of
loading `isidnum_table`, masking `IS_SPC` and comparing. Twelve compilations
per invocation through `cpc.exe @build/prof-self.rsp` measured across 22
interleaved unpacked base/candidate pairs: inner-time median 3437.5 -> 3407 ms
(-0.89%), mean 3437.5 -> 3416.3 ms (-0.62%), with 16 wins, 3 losses and 3 ties
and a paired delta median of -23.5 ms. The first ten pairs measured a -0.36%
mean and the next twelve -0.83%, so the second round is not a single fast
sample. The packed previous-published -> new-published pair measured a
-0.89% median and -0.84% mean over 8 pairs (5 wins, 3 losses) using the no-`-B`
self-compile response file. All 1602 first-party pass-tier sources were
compiled with the previous published and the new published compiler, one
process per source; the 1600 that both accepted produced byte-identical
objects and the other 2 failed for both with byte-identical diagnostics. The
fast tier passed 57 first-party cases with 0 regressions and the retained
corpus stayed at 39/122; the pedantic language group passed 577/577. Rejected
this cycle (each reverted): growing the identifier table at 50% load measured
-0.47% (median/mean slower), a 1024-entry front lookup cache -0.48% median and
-0.72% mean despite 84% exact hits, shrinking the final table to a 2x load
-1.38% median and -1.55% mean, a final bucket-index mix neutral (5 wins, 5
losses, -0.06% mean), collapsing space/tab runs in `preprocess_skip()` -0.45%
median and -0.46% mean, and a collision-flag scheme that skipped the string
compare was unsound for a first colliding spelling and was not kept.

Cycle 34 kept no compiler change. Against the cycle-33 compiler, nine leads
were measured and reverted: a word-at-a-time `SValue` swap (+2.94% mean,
0/8 wins), dropping the tokenizer's per-byte `c` store (neutral),
`ST_INLN` on the byte emitter (+1.31% mean), recording the `__is_`/`__has_`
type-trait spellings as ids (+0.85% mean), unrolling the identifier scan four
ways (+0.85% mean), skipping the C++ expression-spelling probes when none was
interned (neutral), an unrolled short-spelling compare (+1.13% mean), folding
the first hash step to one add (neutral), and loading the text section once in
`g()` (+0.50% mean). Temporary counters put one self-compilation at 334,941
identifier lookups, 26.7% of them keyword tokens and 90.0% ending in a
full-spelling compare. The retained change is in `src/tools/profile_process.c`:
its `-hist` range arguments now use `strtoull(..., 0)` instead of `sscanf`
with `%llx`, which left the range empty under the CPC runtime. The rebuilt
profiler places the current `next_nomacro()` leaf at 11.6%, with its heaviest
32-byte buckets split between the identifier scan and the bucket probe.

Cycle 37 closed pre-step 2 and started retiring repaired rows. On the speed
side, the last named lead -- the `next_nomacro` identifier path -- was
re-profiled on this machine and is at its local optimum for this codegen: a
fresh sample of `cpc.exe @build/prof-self.rsp` against `build/compiler/cpc.map`
charges 11.4% of the run to `next_nomacro` (leaf), 5.5% to `unary`, 3.2% to
`parse_btype`, 2.8% to `next`, 2.3% to `g` and 1.9% to `vswap`, and a 32-byte
instruction histogram inside `next_nomacro` puts 7.8% of the whole run in the
288 bytes that hold the identifier scan/hash loop and the bucket probe, which
matches cycle 33's bisection of that block into the scan/hash loop (2.5%) and
the probe (2.5%). Every remaining candidate there either re-tries a rejected
experiment (word/prefix hashes, caches keyed on first byte, length or hash,
table load factors, comparison unrolling, `memcmp` replacement, dropping the
per-byte classification byte) or needs the codegen itself: the loop is bounded
by this backend keeping `p`, `h` and the byte in stack slots and by struct
copies going through `rep movsq` (`movl $13, %ecx` in a `-S` listing of
`vswap()` is 13 qwords of `sizeof(SValue)` = 104, and the array stride is 112;
`sizeof(Sym)` is 224), which a front-end change cannot reach. No compiler
change was kept, so the published `cpc.exe` is unchanged and the fast tier and
the retained corpus are byte-for-byte the same as at the end of cycle 34.

The row work in cycles 37-38 was the coroutine set that `-fcoroutines` and the
shipped `<coroutine>` already unblocked: 39 of the original 42 retained
coroutine rows compiled, so they were repaired rows that still needed their
behavior in first-party tests before deletion. Cycle 37 wrote six of those
tests in `Tests/features/Declarations/pass/` --
`test_coroutine_await_local_argument.cpp`
(`pr95822.C`, `pr95823.C`, `pr95824.C`: a `co_await` operand built from a live
local, a smart-pointer dereference and a virtual call, each with a held
descriptor), `test_coroutine_move_only_by_value_parameter.cpp` (`pr95350.C`),
`test_coroutine_promise_type_alias_wrapper.cpp` (`pr95346.C`),
`test_coroutine_body_local_function_declaration.cpp`
(`coro-function-decl.C`),
`test_coroutine_throwing_return_object_destructor.cpp` (`pr102051.C`) and
`test_coroutine_yield_conditional_temporary.cpp` (`pr109283.C`) -- and retired
those eight rows plus `coro-pre-proc.C`, whose assertions
`test_coroutine_headers.cpp` already makes. Cycle 38 reduced the remaining 30
rows to twelve more tests in the same directory
(`test_coroutine_return_move_only_operands.cpp`,
`test_coroutine_return_value_shapes.cpp`,
`test_coroutine_promise_constructor_preview.cpp`,
`test_coroutine_lambda_capture_shapes.cpp`,
`test_coroutine_await_expression_shapes.cpp`,
`test_coroutine_yield_await_control_flow.cpp`,
`test_coroutine_promise_static_functions.cpp`,
`test_coroutine_traits_function_object.cpp`,
`test_coroutine_traits_primary_inheritance.cpp`,
`test_coroutine_parameter_lifetime.cpp`,
`test_coroutine_optional_promise_value.cpp` and
`test_coroutine_symmetric_transfer_task.cpp`) and deleted those rows together
with the now-unused `coro.h` and `coro1-ret-int-yield-int.h` support headers.
The corpus is now 81 rows with no repaired passes, and
`g++.dg/coroutines/pr113457.C` is its only retained coroutine row.

The remaining work is the last coroutine row and the general language long
tail; re-open pre-step 2
only with a lead that removes a lookup, a pass or a struct copy from the
compiler's own code. The profile
leads for a future speed session: `next_nomacro` is still the largest reliable
leaf at 12.2-12.4%, now with its per-character hash chain reduced to one
multiply and add and its classification to one table load per byte. Counts from
temporary instrumentation are 334,727 identifier lookups over 2,585,116
identifier bytes (7.7 bytes each), 421,427 hash-chain probes and 302,975 probes
that match hash and length and therefore run `memcmp`; that residue is the
per-byte load/hash chain, the random `hash_ident` bucket load and the probe
walk, all bounded by this codegen's memory-resident locals, so the next win
there has to remove a lookup or a pass. The sampler's 8-byte-bucket histogram
places that fast path inside `next_nomacro` (the `CPC_LAYOUT_MARK()` bisection
below) and splits it evenly between the scan/hash loop and the bucket probe,
about 2.5% each of the self-compile; a 256-entry cache keyed on the first byte
would hit 44.8% of lookups, which is too little of the probe to be the lead on
its own. Cycle 33 also measured the first-byte-and-length reuse cache (71.6%
exact hits) and the hash-keyed caches (77-89% exact hits) directly: the
1024-entry hash-keyed version was still slower, and the 50%-load and 2x-load
table experiments measured slower in both directions, so the current
approximately full-load table is the local optimum for this codegen. The
horizontal-whitespace classification mask is now off the list as well. The
next lead is the scan/hash loop or a way to remove a different lookup or pass;
the `memcmp` itself cannot be removed without an exact spelling comparison for
a first colliding spelling.
`unary` is still about 5.9% of the self-compile; it no longer derives a
`table_ident` spelling there. `g` 2.4%, `parse_btype` 2.4%, `vswap` 2.1%, `next`
1.9% and `get_tok_str` 0.34%, the last now almost all of it the identifier fast
path its callers ask for by name; the rest is a long tail (`gen_cast`, `load`,
`macro_subst`, `tok_str_add2`,
`tal_realloc_impl`, the statement/decl path) over roughly 11% of unmapped leaf
samples in DLL routines, which the sampler places in ucrtbase's `memset`
(0x48810) and `memcmp` (0x48aa0) and in ntdll's heap-adjacent region
(0x9d180), reached from `expr_cond`, `next_nomacro`, `next`, `sym_push` and
`default_reallocator`. The repeated `get_tok_str()` calls and spelling probes
that fed those checks are off the list.
`find_expand_inline_function`, `find_cpp_this_symbol`, the environment gates,
the `is_cpp_translation_unit` call, the literal interning behind `tok_alloc` and
the C++ name-resolution helpers gated in cycle 24 are off the list, and so are
the second `define_find()` probe in `next()`, `preprocess_skip()`'s per-byte
dispatch, the free(NULL) storm from `sym_free()` and the per-character
identifier-hash shift/add chain and the identifier classification mask are off
the list as well; the `std::is_` qualifier probes and the context-spelling
probes went with cycle 31, and the six expression-path spelling probes went with
cycle 32.

Three measurement-protocol facts cost this cycle time and are worth keeping.
An unpacked compiler resolves its include set from its own directory, and the
SDK headers only exist inside the packaged payload, so the codegen-neutrality
sweep has to compare the previous published compiler (saved before `Build.cmd`
publishes) against the new one; the unpacked pair cannot compile the SDK-header
tests at all. On the packed compiler `-B<dir>` replaces the payload's include
search, so `build/prof-self.rsp`, whose lines start with `-B.`, only runs
against unpacked builds -- timing the published compiler needs the same lines
without `-B`. Finally, a bare `cpc.exe` handed to a helper resolves through
PATH to the unrelated archived compiler, so every measurement names the
compiler by absolute path.

Batch mode is not sweep- or measurement-safe and is a separate cluster. A batch
of the first 88 first-party pass sources deterministically makes job 88
(`Tests/features/Classes/pass/test_anonymous_enum_member_replay.cpp`) fail with
"member function definition requires declared class 'Controls'" although that
source compiles alone, and a 1602-job sweep stopped producing results after 228
jobs. State that survives between jobs in one process is the lead; per-source
processes were used for the codegen-neutrality sweep this cycle.

Three failures sit outside the target, for whoever owns those gates.
`Tests/run-checks.ps1` reports `test_MsvcRecordReturn` failing on
`native_make<Defaulted4>` (exit 8), which matches cycle 18's "a class whose
only constructor is defaulted stays an aggregate" change against MSVC's
trivial-record rule. `test_AsmOutput` fails in every recorded check run,
including runs before cycle 18. Neither is part of the retained corpus or the
pedantic first-party list, and cross-compiler ABI gates need explicit
authorization. `Tests/pedantic/performance/pass/test_heap_list_push_clear_perf.cpp`
does not compile in any compiler state: it calls `GetTickCount64`, which the
vendored `third-party/win32-sdk/include/winapi/winbase.h` never declares, so
the pre-step 2 runtime workload is unavailable until CPrime supplies that
prototype from first-party code.

Cycle 39 found one more defect outside the target while reducing its row:
calling through a pointer to member that names a member of a class-template
instantiation or of a member-function-template specialization returns garbage
(`int (D::*p)(int) = &D::add<int>; (d.*p)(3)`, and likewise
`&A<int>::plain`), while the same shape on a plain class is correct.  Both were
reproduced with the cycle-38 compiler saved before this cycle's build, so the
defect predates the row repair; the retained row is compile-only and did not
need it fixed.  It is a separate cluster for whoever takes pointer-to-member
calls.

Cycle 51 found one more defect outside the target while narrowing its row: in a
directly parsed braced initializer, a lambda as the first element is read as an
array designator, so `Holder h = { []() { return 4; } };` reports `array type
expected` in `decl_designator`.  It predates the cycle-51 compiler (the
unchanged direct parse never entered the saved-body rewrite the row repaired),
and no retained row needs it.  It is a separate cluster: the designator probe
has to recognize a bracket that opens a lambda introducer before requiring an
array type.

Cycle 57 left one known gap in the new `_Complex` support, outside the target
because no retained row needs it: a *static* (file-scope or function-local
`static`) complex object initialized from a value rather than a brace list
(`_Complex double z = 3.0;`, `_Complex double z = 1.0 + 2.0i;`) has no
load-time representation, so in C++ mode it takes the dynamic-initialization
path and runs correctly, while in C mode it reports `initializer element is not
constant` where GCC emits static data. A brace list over the two parts
(`{1.0, 2.0}`) and initialization from an anonymous compound literal still
become static data. Giving the conversion a synthesized static object for
constant operands, as the compound-literal path already does, would close it.

## Pre-steps

Do these first, in order. The target does not move; the order does. While a
pre-step is unfinished, do not open a new retained GCC row.

1. Green internal baseline. Every existing first-party test passes at both
   tiers -- `Tests/run-all.ps1 -Tier fast` and `Tests/run-all.ps1 -Tier
   pedantic` -- and `Tests/progress/first-party-failures.txt` is empty. Repair
   each listed failure as a normal cycle (reproduce, minimal first-party
   regression, fix, re-run the tier). The retained external GCC rows are the
   only cases allowed to fail by design: no skips, no `EXPECT_COMPILE_FAIL`
   rewrites, no relaxed timeouts and no missing coverage.
2. Compilation-speed wave. Make root `cpc.exe` compile the existing workloads
   measurably faster with no behavior change: no new passes, no new failures
   and no weakened expectations. Improve algorithms and data handling, keep one
   thread and one compiler process at a time, and measure the identical
   workload before and after on this machine with `cpc.exe` alone. Record both
   numbers; keep a change only when it is a repeatable win, and revert it
   otherwise. Use `Tests/benchmarks/compile/` and
   `Tests/pedantic/performance/`; add first-party benchmark inputs there rather
   than a second harness. The wave ends when the benchmark set stops yielding a
   repeatable win without regressing the fast or pedantic tiers. Closed in
   cycle 37: thirteen landed changes (cycles 19-33), then nine leads in cycle
   34 and the re-profiling in cycle 37 with no repeatable win left. A session
   that finds a new lead re-opens the wave.
3. Wider GCC retarget. Only after 1 and 2, continue the wave plan below
   (Wave A item 3 onward) and re-assess the retained corpus against the wider
   remaining GCC language rows, adding rows as the wave budget allows.

## Cycle contract

One session = one cluster; a fresh launch beats a long session that runs out of
context.

1. Pick the next unfinished pre-step above, or the next item from the wave plan
   below once the pre-steps are done. Reproduce one row:
   `Tests/pedantic/gcc/run.ps1 -Select <path> -Out build/<name>`.
2. Reduce it to a first-party regression in the nearest `Tests/features/...`
   suite, fix the compiler, and re-run the selection plus the regression until
   both pass.
3. Retire the row: delete the corpus file, remove its `corpus.json` case entry
   and drop any `Tests/tiers.json` entry for it. Repaired rows are deleted, not
   promoted into a permanent external corpus.
4. Re-run `Tests/run-all.ps1 -Tier fast` once (about 20 s) to confirm zero
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

Measurements must name the compiler explicitly: on this host a bare `cpc.exe`
resolves through the PATH to an unrelated archived compiler, which reports
different predefined macros and emits different objects.

## Wave plan

Waves A to C run after the pre-steps above are done.

139 rows = 40 coroutines + 17 `_Complex` + 82 general language long tail;
Wave B is closed (cycle 57 retired its 20 rows and the four further
`_Complex` rows counted in the long tail) and Wave C has one row left, so the
ceiling is now the last coroutine row plus the long tail.

### Wave A - general language long tail

Batch two or three independent repairs per session, in this order:

1. Overload and deduction singletons: the remaining `no matching function
   template` / `no matching overloaded function` rows. `mem_fun` and
   `defaultHandler`, `call7.C`, `ttp58.C` and `spec4.C` are retired, and cycle
   10 retired `template26.C` and `spec28.C`; cycle 13 retired `overload6.C` and
   `friend28.C`, so this item is empty apart from
   `g++.dg/coroutines/pr113457.C`, which belongs to Wave C.
2. Const and conversion cluster: empty; cycle 15 retired the last
   `cannot convert` external rows (`new5.C`, `cvt21.C`, `ptrmem5.C`).
3. The narrowed leads recorded under
   [retained GCC checks](Tests/pedantic/gcc/README.md#narrowed-leads), one site
   each; the list is empty now. Cycles
   39, 41, 43, 45, 46, 48, 49, 50 and 51 retired `conv1.C`, `synth7.C`,
   `template25.C`, `access28.C`, `canon-type-3.C`, `syshdr1.C`, `redef1.C`,
   `anon3.C`, `sts_iarr.C` and `desig11.C`; cycle 50 fixed the terminal name of
   a qualified id replayed inside a class-template member body (the enclosing
   instantiation's alias table used to redirect it to its own joined member
   name), the same rule the class-body-level replay already applied, and cycle
   52 closed the lead that followed them, the constant probe's new
   declarations, and cycle 54 closed the array-decay site. Cycle 55 closed
   `spec7.C` (the member function template of a member class template
   specialization) and confirmed the dependent template template chain was
   already covered by cycle 32; cycle 56 closed the last one, the indirect
   virtual base mem-initializer.

### Wave B - `_Complex` (closed in cycle 57)

All three items landed together: type/declarator parsing for `_Complex`,
`__complex__` and `__complex` with imaginary literals; the arithmetic,
conversions and the `__real__` / `__imag__` / builtin `conj` family; and the
layout and ABI questions the `g++.dg/opt` rows raise. Complex types reuse the
aggregate paths, so the runtime layout follows the two-part representation on
every row. Coverage lives in `Tests/features/GnuExtensions/pass/`
(`test_complex_type_declarations_and_literals.cpp`,
`test_complex_arithmetic_and_parts.cpp` and
`test_complex_in_classes_and_templates.cpp`).

### Wave C - coroutines (40 rows, 4 sessions)

1. Ship `<coroutine>` and `<experimental/coroutine>` and accept `-fcoroutines`;
   this single step unblocks 28 rows. Landed before cycle 37: the headers and
   `-fcoroutines` exist and 39 of the 42 retained coroutine rows compile;
   cycles 37-38 retired those 39 repaired rows into first-party coverage
   (nine, then 30), leaving `pr113457.C` for item 2.
2. Parse `co_await`, `co_yield`, `co_return` and awaitables. Cycle 58 landed
   the two lookup repairs `pr113457.C` exposed; the row itself still needs
   class template argument deduction from an explicit deduction guide and the
   `template <Concept T>` type-constraint spelling, and its lead is recorded
   under [retained GCC checks](Tests/pedantic/gcc/README.md#narrowed-leads).
3. Promise machinery and coroutine frame lowering.
4. ABI, symmetric transfer, exceptions and destruction.

## Known first-party pedantic failures

None. `Tests/progress/first-party-failures.txt` is the authoritative path list.
CPrime tests that reproduce failures in the sibling CL/OTServer project must
stay standalone; do not include `CommonLib` or other CL headers.

The named return value optimization is deliberately partial: it applies to a
body whose single, trailing `return <identifier>;` names an automatic object
of the caller-owned result type, declared directly in the function body's own
block, and whose declaration is constructor-shaped. Other named returns keep
the byte-copy elision that was already in place.

Resolved work, per-cycle history and yield analysis live in git history and
[the development loop](Tests/DEVELOPMENT.md).
