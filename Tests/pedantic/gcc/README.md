# Retained GCC compatibility work

## Scope and evidence

45 unresolved rows remain at upstream revision
`5f6257c26b814de1a14c71b2d3a49291765b6577`. The 2026-09-13 root-CPC audit
reported compile/link failure for every row, with no timeout. Compiler identity
and exact commands are in `build/gcc-preparation-audit/metadata.json` and
`results.jsonl`; task.md records the compiler hash. The manifest hashes every
retained source and its required support header. Passing behavior must move to
minimal first-party coverage before deleting a repaired external row.

C++17 is the cutoff. `g++.dg/coroutines/pr113457.C` is excluded for concepts,
ranges and coroutine requirements; do not resume its old repair queue.
`g++.dg/cpp26/aggr-init1.C` remains because the line-92 failure is in an older
constexpr aggregate expression. Reduce that in-scope behavior without adding
post-C++17 support. Old GNU syntax and optimizer link sentinels are labelled
separately below; acceptance is not a claim of standard conformance.

## Package mechanisms

| ID | Mechanism and likely implementation | Package validation |
| --- | --- | --- |
| T1 | Declaration ownership, specialization identity and linkage: cprimegen_templates.inc, function_specializations.inc, microsoft_mangle.inc. Register the owner and declaration before replay; canonicalize once. | Member templates under non-template owners, friend template-ids, enum arguments, declarations/definitions and separate-TU symbol identity. |
| T2 | Dependent scope and completion: templates, substitution, alias_templates, cpp_member_info. Keep lexical lookup separate from receiver lookup; materialize layout only when semantically required. | Alias/class chains, explicit-instantiation parameter scope, free names inside members, deferred nested layout; include access rejection controls. |
| T3 | Deduction, defaults and ordering: template_ordering, substitution, cpp_names_overload. Substitute explicit arguments before deduction and preserve candidate order/ownership. | Runtime selected-overload values, all argument spellings, ambiguity controls, SFINAE versus hard errors. |
| T4 | Member-pointer identity/calls: member_pointers, microsoft_mangle, cpp_member_info and backend receiver adjustment. Use a consistent owner/type representation through substitution and emission. | Non-primary base, virtual dispatch, member-template addresses, NTTPs and callable results; compile-only success is insufficient for runnable originals. |
| I1 | Initialization and lifecycle: initializers, constexpr, lifecycle, temporaries. Separate parsing, viability, constant materialization and runtime cleanup. | Static assertions plus runtime values/counts, new/class conversions, mutable assignment, compound-literal lifetime and the pending reductions in task.md. |
| E1 | Function-try-block parsing and cleanup: statements, exceptions, lifecycle. Save the function scope and constructed-subobject state for handlers. | Constructor base destruction before handler, ordinary member handler scope and propagation. |
| X1 | GNU/C compatibility: cprimegen.c, initializers, cprimeasm.c, cprimepp.c and linker symbol naming. Keep extensions explicit; do not weaken standard diagnostics globally. | Generic atomic size, asm tied operands/names, VLA scope, alignment and system-header-only permissive behavior. |
| L1 | Library and object model: first-party include/ library headers, class base storage, RTTI/lifecycle. Replace fixed-capacity assumptions with bounded owned storage. | bitset proxy/value checks, dynamic complete-object identity, capacity boundary and pedantic hierarchy stress. |
| O1 | Required optimization/linkage: frontend reachability, exception attributes and x86_64-fastopt.inc. Optimize proven unreachable calls while preserving side effects and inline linkage. | Keep undefined sentinel functions undefined; both the original link check and a local semantic opposite must behave correctly. |

These are investigation plans based on source and first diagnostics, not claims
that every row has a proven common root cause. Process T1 before dependent T2/T3
and T4 fixes; initialization and exception work then feed O1. Reassess all rows
in the affected package after a coherent repair, rather than starting another
one-row cycle. The general order and non-GCC issues are in [task.md](../../../task.md).

## Complete retained inventory

All rows currently have status FAIL_COMPILE (the adapter uses this label for
link failures too). Paths are relative to corpus/. The error column is the
first observed blocker; the planned check includes behavior beyond that blocker.

| Row | Package | Current blocker | Planned repair and proof |
| --- | --- | --- | --- |
| `c-c++-common/pr60689.c` | X1 | integral or integer-sized pointer target type expected | Implement generic __atomic_exchange for non-scalar objects; check 9-byte exchange and sequential consistency, not integer casts. |
| `c-c++-common/pr71654.c` | O1 | undefined symbol 'foo' | Fold the proven unsigned-byte condition before emitting a reference to foo; retain the undefined sentinel. |
| `g++.dg/cpp26/aggr-init1.C` | I1 | constant expression expected | Reduce line 92 to C++17 constexpr aggregate default-member initialization reading a string; preserve constexpr and runtime checks, exclude only later-feature branches. |
| `g++.dg/eh/comdat1.C` | O1 | undefined symbol '?undefined@@YAXXZ' | Prove the throw() path cannot reach undefined; check exception cleanup and emitted unresolved references together. |
| `g++.dg/eh/dtor1.C` | E1 | function definition expected | Parse constructor function-try-blocks and destroy fully constructed bases before entering the handler; observe destructor count/order. |
| `g++.dg/ext/attrib6.C` | O1 | undefined symbol '?link_error@@YAXXZ' | Carry nothrow into reachability so link_error stays unreferenced; keep the sentinel undefined. |
| `g++.dg/ext/complit12.C` | I1 | '{' expected (got ';') | Parse GNU array compound literals with class elements; check constructor/destructor counts and lifetime. |
| `g++.dg/ext/pr99508.C` | X1 | undefined symbol 'bar_assembler' | Unify block extern asm names with file-scope declarations for functions and data; link under the written assembler names. |
| `g++.dg/ext/tmplattr2.C` | X1 | initialization of incomplete type | Substitute dependent aligned attributes without losing the typedef array type; assert size/alignment and instantiate both dimensions. |
| `g++.dg/ext/vla9.C` | X1 | 'x2d' undeclared | Keep runtime array-bound typedefs visible through pointer declarators; verify dimensions, allocation and indexed writes. |
| `g++.dg/inherit/ptrmem2.C` | T4 | incompatible redefinition of 'D_f' | Separate derived member-pointer declarations from inherited virtual-function identity; verify owner adjustment and indirect call. |
| `g++.dg/init/new33.C` | I1 | no matching constructor for placement new of 'A' with 1 arguments | Allow construction of A from a user conversion to const A& during new; cover trivial and nontrivial temporary cleanup. |
| `g++.dg/init/pr25811-3.C` | I1 | static assertion failed in '' | Make new-expression substitution test default-initialization viability of const/reference members; preserve positive and negative static assertions. |
| `g++.dg/ipa/pr60640-3.C` | T4 | no matching member function '__cpc_local_class_1659_1::foo' | Audit covariant returns, multiple-base receiver adjustment and virtual lookup; preserve the runnable dispatch assertions. |
| `g++.dg/ipa/pr98075.C` | T2 | no matching member function 'xg__int::operator new' | Let unqualified operator new in a template member resolve the global allocation function when no class declaration owns it. |
| `g++.dg/opt/inline11.C` | O1 | undefined symbol '?baz@@YAHH@Z' | Implement gnu_inline linkage and required call elimination without inventing baz; pin external definition versus inline body semantics. |
| `g++.dg/opt/pr48967.C` | T2 | nested template type member 'S__F__C_O::J' must be a typedef | Resolve nested member templates/types across dependent alias chains; do not require a nested class to be a typedef. |
| `g++.dg/opt/pr79267.C` | E1 | ';' expected (got 'catch') | Parse ordinary member function-try-blocks and lower the handlers with the member scope intact. |
| `g++.dg/other/copy1.C` | I1 | assignment of read-only location | Respect mutable fields during implicit copy assignment through const contexts; verify copy/assignment counters and subobjects. |
| `g++.dg/other/pr24623.C` | I1 | no matching user-declared copy assignment operator | Resolve inherited/user-declared assignment and conversion candidates before implicit fallback; test the original wrapper assignment. |
| `g++.dg/other/vararg-5.C` | T2 | nested template type member 'b__void::c' must be a typedef | Substitute dependent base nested class types and varargs declarations; distinguish class members from typedef aliases. |
| `g++.dg/overload/defarg4.C` | T3 | 'func' undeclared | Resolve a member-template default argument in its declaration scope and deduce the function-pointer target. |
| `g++.dg/overload/member2.C` | T4 | unsupported non-type template argument 'int' in native linkage for 'bar' | Encode distinct member-function template and member-pointer argument types in native linkage without collapsing overloads. |
| `g++.dg/parse/using3.C` | T1 | base class type expected | Select the explicit nested-class specialization before instantiating the invalid primary base T=int; retain using lookup for T=b. |
| `g++.dg/pr61033.C` | X1 | ';' expected (got 'unicode') | Audit system-header permissive missing-return-type declarations as a GNU compatibility behavior, not standard C++17; isolate acceptance from backend checks. |
| `g++.dg/pr96818.C` | T2 | no matching member function 'l::operatorY' | Keep namespace/free calls in saved member bodies from becoming member calls; resolve operatorY under lexical scope. |
| `g++.dg/template/access37.C` | T2 | field 'range_' has incomplete type | Defer nested-class layout until its enclosing instantiation is complete; then audit the friend's access and DECLARE_FRIEND variants against upstream intent. |
| `g++.dg/template/access6.C` | T2 | parameter type expected before 'Type' | Resolve the trailing parameter type in explicit member-template instantiation under the qualified class scope; preserve protected typedef access. |
| `g++.dg/template/anonunion1.C` | T4 | 'F_bar' undeclared | Instantiate an addressed member template with a member-pointer argument; preserve the anonymous union body and callable symbol. |
| `g++.dg/template/arg6.C` | T3 | ')' expected (got '1') | Parse functional/cast bool constant expressions as non-type arguments with balanced angle/parenthesis handling; check all five spellings in one test. |
| `g++.dg/template/array21.C` | T3 | base class 'dynamic_dispatch____cpc_template_type_struct___cpc_member_pointer_file_reader_func_void_int_ref' is incomplete | Deduce array partial specializations and member-pointer function parameter packs before requiring a complete base specialization. |
| `g++.dg/template/asm1.C` | X1 | invalid operand reference after % | Handle GCC's implicit tied input for a +r output when numbering %0/%1; check both template instantiations and emitted assembly. |
| `g++.old-deja/g++.law/operators34.C` | I1 | new requires a complete element type | Resolve new class A to the existing complete class rather than introducing a fresh incomplete tag; check allocation and construction. |
| `g++.old-deja/g++.martin/bitset1.C` | L1 | include file 'bitset' not found | Supply first-party bitset support needed by this case; test proxy assignment, indexed read and value preservation, without pretending this covers the entire header. |
| `g++.old-deja/g++.mike/dyncast5.C` | L1 | no matching member function 'Foo::isObjectAllocation' | Audit inherited static lookup before RTTI/allocation tracking; preserve complete-object address and dynamic-cast assertions. |
| `g++.old-deja/g++.mike/hog1.C` | L1 | too many base classes for 'super' | Replace the hard base-class capacity limit with owned growable storage; preserve virtual-base uniqueness and add small boundary plus pedantic stress coverage. |
| `g++.old-deja/g++.other/crash5.C` | I1 | function parameter type expected (got 'D') | Disambiguate a typedef-named functional expression in an initializer from a parameter declaration; retain declaration-versus-expression controls. |
| `g++.old-deja/g++.other/overload12.C` | T3 | ambiguous overloaded function 'f' | Rank derived/base pointer conversions by the correct inheritance distance; retain genuinely ambiguous negative controls. |
| `g++.old-deja/g++.other/pmf4.C` | T4 | incompatible types for redefinition of 'B_f': 'void (struct B *)' versus 'struct __cpc_member_pointer_C_func_void' | Prevent data member PMFs from redeclaring inherited function symbols; verify invocation through a non-primary vtable. |
| `g++.old-deja/g++.pt/explicit81.C` | T3 | too many arguments to function | Substitute explicit template arguments before deduction; preserve overload arity and invoke the selected specialization. |
| `g++.old-deja/g++.pt/instantiate11.C` | T1 | ')' expected (got '<') | Parse friend operator template-ids with explicit argument lists and bind the existing declaration, not a new overload. |
| `g++.old-deja/g++.pt/ptrmem2.C` | T4 | invalid type for 'h' | Represent and substitute member-function pointers as non-type arguments through typedefs; verify indirect calls. |
| `g++.old-deja/g++.pt/spec18.C` | T3 | '>' expected (got '*') | Parse and order function-template specializations with pointer arguments; retain both primary overloads as selection controls. |
| `g++.old-deja/g++.pt/ttp23.C` | T3 | qualified member requires an object of its class | Preserve ownership through template-template argument substitution; check the qualified call on its actual object. |
| `g++.old-deja/g++.pt/ttp53.C` | T1 | 'H____cpc_template_type_const_int_template' undeclared | Bind an explicitly qualified friend function-template specialization under the instantiated class without rewriting its template keyword as a name. |

## Commands and retirement

Run an exact row with
`powershell -NoProfile -ExecutionPolicy Bypass -File Tests/pedantic/gcc/run.ps1 -Select <path> -Out build/gcc-package`.
Prefix selections may select a package directory; unknown selections fail.
Use root CPC only and one compiler process at a time. Each compile/program has
a five-second ceiling; crashes/timeouts fail without retries. The adapter uses
CPC's default language mode, so content inspection/reduction is required for
the C++17 cutoff; the runner does not enforce a standard-version matrix.

The retained suite deliberately exits nonzero while positive cases fail; it is
not an expected-failure green gate. Fast selects all retained rows, pedantic
selects none, and `-Tier all` selects the complete manifest. Before final
retirement, fix empty-manifest handling in corpus.ps1 and run.ps1; both currently
reject an empty corpus instead of reporting a valid zero-case assessment.
Implement new/replacement tooling in native C under src/, built by root CPC;
avoid expanding the PowerShell runner implementation.

For each repaired row, preserve its distinct observable behavior in a combined
first-party test where flags and scope permit, verify original and reduction,
then delete the source and manifest entry. Remove unused support entries only
after checking remaining include dependencies. Keep auxiliary multi-TU cases
open until their complete linkage scenario is represented locally.

Corpus/tier edits require `test_corpus.ps1` and discovery checks. Harness
changes also need runner, assessment, provenance and progress fixture checks.
Raw logs remain in build/; this file contains only current decisions and work.
