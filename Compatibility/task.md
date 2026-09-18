# Compatibility: endlessly correct C++17

Make root `cpc.exe` accept every valid C++17 program and reject invalid ones
with the right diagnostic, so real C++17 projects compile unmodified. Coverage
never ends: when the current gaps close, find the next real project or language
area that breaks and close that too. Never create `done.x`.

Work in this tree only. Delete nothing: `Compatibility\worker.cmd` and this file
are the user's control surface, and other worker folders may be in use on other
days.

This clone is one arm of a shared branch: up to three machines run the
Compatibility, Capability and Cost workers against the same origin, and
`Compatibility\worker.cmd` commits, fetches, rebases and pushes at every cycle
boundary. Expect the tree to hold the other agents' work when a cycle starts,
and expect the work left behind to be published to them. Leave git to the
worker: do not add, commit, fetch, rebase or push yourself, and never drop or
rewrite another agent's work to make a merge easier. When the worker opens a
cycle by naming an unfinished rebase, that merge is the cycle's task: keep both
sides' intent, run the fast tier and `-Regression`, then finish it with
`git -c core.editor=true rebase --continue`.

## Top priorities

Both priorities of the 18 September cycle are closed and published; they are
the newest closed entries in `Compatibility\KNOWN-ISSUES.md`, the evidence is
`Compatibility\build\top2\evidence-after.log`, and the retained cases are listed
under the first two leads below.  A consumer that worked around the header shape
can revert the workaround and refresh its `cpc.exe` copy from this tree's root.
The consumer's server build then exposed a third frontend defect, also closed
this cycle: a nested same-type temporary claimed the enclosing initializer's
construction destination and lost its prvalue category
(`Compatibility\KNOWN-ISSUES.md`, retained as
`features/Constructors/pass/test_nested_temporary_argument_keeps_destination.cpp`),
plus one consumer-side source fix (a setter parameter now takes `const v2I &`).
With those, the consumer's release build completes.

The conditional-expression miscompile this clone took on is closed and
published too: a conditional whose result is an lvalue kept each arm's address,
and the value fold used to replace a file-scope `const` arm with the constant
itself (`Compatibility\KNOWN-ISSUES.md`, evidence and the eight-case matrix in
`Compatibility\build\cond-ternary\`, retained as
`features/Expressions/pass/test_conditional_lvalue_constant_arm.cpp`).  The
consumer's pugixml workaround is reverted: its `cpc.exe` copy is refreshed to
this tree's published compiler (hashes match), the library is rebuilt, and the
rebuilt program loads its content and runs its steps
(`Compatibility\build\cond-ternary\verify-revert.log`).

1. The alias-replay limit a consumer is most likely to reach: a class
   template's own member typedef consumed by another member alias still reports
   `using alias type expected`
   (`Compatibility\build\top2\cases\k04_member_typedef.cpp`).
2. The stale diagnostic location for an error raised while replaying tokens
   saved from another file (`Compatibility\build\top2\cases\diag2.cpp`).
3. The member-pointer mangling limit for a member-pointer type nested in a
   function type used as a class template argument
   (`Compatibility\build\top2\cases\mp4.cpp`).
4. The `-S` listing drops every instruction whose operand carries a symbol
   relocation, so it is not the emitted program and it misleads any code
   reading (`Compatibility\KNOWN-ISSUES.md`, findings in
   `Compatibility\build\cond-ternary\findings.log`).

## Where compatibility evidence comes from

1. Retained internal cases, which are the record of what works:
   `Compatibility\tests\features\**`, `Compatibility\tests\integration\**`,
   `Compatibility\tests\abi\**`, `Compatibility\tests\runtime\**`,
   `Compatibility\tests\payload\**`, `Compatibility\tests\c_compat\**`. The Microsoft
   x64 ABI facts live in `Compatibility\tests\features\Abi`.
2. Large C++ projects used as probes: the vendored competitive inputs under
   `Cost\tests\compile\competitive\` (xBRZ and the functions/pch cases),
   and the user's own consumers, which are read only and whose findings are
   recorded in `Compatibility\KNOWN-ISSUES.md`.  Consumer checkouts live outside
   this repository, at whatever path the machine keeps them, and are not named
   here, so a probe this clone cannot see is not this cycle's work. Compile a
   project with root `cpc.exe`, reduce each failure to a minimal local case, and
   never edit the consumer.
3. Language unit tests and compiler test suites from clang, gcc and msvc used as
   a source of expectations. Running those toolchains needs the user's explicit
   authorization; without it, derive the required behaviour from the standard
   and record it as a local case. Never make an external compiler a CPC build
   dependency or a substitute for a fix.

## The loop

```
Compatibility\tests\test.exe -Suite features/X -Select test_y.cpp     reproduce, minimal case
Compatibility\tests\test.exe -Suite features/X                        the affected suite
Compatibility\tests\test.exe -All -Tier fast                          the open-work list
Compatibility\tests\test.exe -Regression                              publication gate
src\scripts\build.exe                                                  publish the compiler
```

- Reproduce first with the exact case; repair the shared mechanism, not the
  symptom; retain one minimal case in `pass/`. A gap that is still red is listed
  in `Compatibility\tests\tiers.json` and leaves the list by passing, never by removal.
- A case that crashes the compiler starts in a suite of its own so the crash
  cannot abort a shared batch.
- Do not run the pedantic tier. The affected suite plus `-Regression` is the
  broad check. Publish with `src\scripts\build.exe` once packaging and the gate
  pass.
- Do not relabel a failing case, weaken an expectation, or delete coverage to
  make the tree green. `Compatibility\tests\CPP17-REMAINING.md` and
  `Compatibility\KNOWN-ISSUES.md` are the open-work lists; keep them accurate as
  work completes.

## Standing decisions

- The two 18 September priorities from a consumer tree are closed and
  published.  A pack expansion inside a function type is a parameter list, so
  its first expanded entry may be any type:
  `template_paren_starts_parameter_list` now accepts every
  declaration-specifier start (the generated type name a reference, pointer,
  `const`-qualified or class-type element arrives as) and resolves
  `ns::I`/`::I`, while keeping `bool(1)`, `bool(flag)` and
  `bool(std::trait<T>::value)` expressions; the `using`-alias replay of a class
  template now inserts the declared name into the declarator's own name slot
  (`typedef bool Name(int);`), so `using F = bool(int);`, `using A = int[4];`
  and `using P = bool (*)(int, int);` declare what they say.  Retained as
  `features/Templates/pass/test_function_type_pack_expansion_parameter_list.cpp`
  (the `std::function<bool(Args...)>` consumer shape, the bare alias spelling,
  the parameter-type matrix and the partial-specialisation pattern) and
  `features/Templates/pass/test_class_template_function_type_alias_declarator.cpp`.
  `std::thread` with a bound pointer argument is closed in the runtime header:
  `__cpc_thread_arguments<>::invoke` takes its bound arguments by value and
  moves them into `std::invoke`, which repairs the `void *`/`int *` parameters
  and the `std::thread` pointer-to-member case
  (`features/StdConcurrency/pass/test_thread_deferred_member_address.cpp`)
  together; retained as
  `features/StdConcurrency/pass/test_thread_function_pointer_argument.cpp`.
  Evidence for both is `Compatibility\build\top2\evidence-after.log` with the
  probe cases and matrices in `Compatibility\build\top2\cases\`; the published
  checks were features/Templates 971 passed, features/StdConcurrency 15 passed,
  fast 29 passed and `-Regression` 66 passed, and the published hash is in
  `Compatibility\build\top2\publish-hash.txt`.  The consumer side can refresh
  its `cpc.exe` copy and revert the header function-typedef workaround.
- The conditional-expression miscompile is closed in this file's own cycle
  (`Compatibility\KNOWN-ISSUES.md`, first entry).  A conditional whose result
  is an lvalue joins its arms as addresses, and `expr_cond` converted each arm
  with the ordinary value conversion before taking the address; that
  conversion reads a named integral constant as its value
  (`constexpr_read_local_value`), so a file-scope `const`/`constexpr` arm
  stopped being an object and the shared indirection loaded through the
  constant itself.  The join now raises `cpp_conditional_lvalue_arm` around
  each arm conversion and the value fold leaves such an arm alone.  Retained as
  `features/Expressions/pass/test_conditional_lvalue_constant_arm.cpp`
  (the pugixml value shape, the reference-identity shape and an assignment
  through the conditional); evidence is
  `Compatibility\build\cond-ternary\evidence-before.log` and
  `evidence-after.log`, one eight-case matrix against the retained pre-fix
  compiler (`cpc-before.exe`) and the published one.  The same probe found that
  `cpc -S` prints the backend's instruction trace and drops every instruction
  whose operand carries a symbol relocation, which is why the first report's
  reading of the generated code was wrong; findings and reductions are in
  `Compatibility\build\cond-ternary\` (`findings.log`, `cases\`,
  `pe_code.py`), and the defect is recorded as open in
  `Compatibility\KNOWN-ISSUES.md`.  The consumer is on the fixed compiler: its
  `cpc.exe` matches this tree's published hash, the pugixml workaround is
  reverted, and the rebuilt program loads its content and runs its steps; the
  whole consumer-side record, including the pre-fix `invalid memory access`, is
  `Compatibility\build\cond-ternary\verify-revert.log`.  The published
  checks were features/Expressions 231 passed (230 before this cycle's retained
  case), features/Classes 248, features/Constructors 214, features/Templates
  971, features/StdConcurrency 15, features/Includes 76, fast 29 and
  `-Regression` 66, and `src\scripts\build.exe` republished the self-hosted
  compiler.
- A diagnostic raised while replaying tokens saved from another file still
  prints that file's line number with the current file's name (a three-line
  translation unit reported `t1.cpp:17`).  The two priorities above no longer
  reach it; the reduction `Compatibility\build\top2\cases\diag2.cpp` with
  `diag_h.h` shows the shape, and fixing it needs a per-token-string origin
  file rather than the current single `file` pointer.  Two coverage limits
  found by the same probe matrix are recorded at the top of
  `Compatibility\KNOWN-ISSUES.md`: member-pointer types nested in a function
  type used as a class template argument (`mp4.cpp`, `mp3.cpp`) and a class
  template's own member typedef or nested type used inside another member alias
  (`k04_member_typedef.cpp`, `w13_qualified_member.cpp`).
- The third frontend defect this cycle closed is the initializer-destination
  leak: `try_parse_cpp_functional_constructor` now hides the pending
  construction destination while it evaluates a call that cannot use it, and
  `template_probe_type_ex` hands the destination back with the availability it
  was given.  Without it, a functional-constructor call on a nested same-type
  temporary failed with `rvalue reference cannot bind to an lvalue`, which is
  what stopped the consumer's server build after the two priorities were
  published.  Reductions are
  `Compatibility\build\top2\cases\rva.cpp` and `rvl.cpp` (plus `cp1.cpp` and
  `cp3.cpp` against the consumer's headers), retained as
  `features/Constructors/pass/test_nested_temporary_argument_keeps_destination.cpp`;
  the consumer build also needed a setter parameter to take `const v2I &`.
- The two fast-tier gaps are closed, and the shape behind them is now covered
  twice over.  A standard attribute-specifier sequence is accepted anywhere in
  the decl-specifier-seq and at the end of a function's
  parameters-and-qualifiers, so `inline [[noreturn]] void f(int const &);` and
  `int f(int) [[noreturn]];` both parse; retained in
  `features/Cpp17Gaps/pass/test_attribute_before_function_template.cpp` and
  `features/Cpp17Gaps/pass/test_attribute_after_parameter_list.cpp`.  Decision:
  a compiler-side merge of a using-declaration import with a same-signature
  declaration in the importing namespace must stay rejected, because
- A compiler-side merge of a using-declaration import with a same-signature
  declaration in the importing namespace stays rejected:
  `features/Namespaces/fail/test_using_merged_overloads_ambiguous.cpp` requires
  that diagnostic. A duplicated overload set is therefore repaired in the
  runtime headers, where the reference headers import one set per name instead
  of redeclaring it in `namespace std`; that is how the `<cstdlib>`/`<cmath>`
  duplication was fixed.
- Attribute specifiers parse anywhere in the decl-specifier-seq and at the end
  of a function's parameters-and-qualifiers, and a declaration-specifier
  attribute may sit between a class-key and the name of a class template
  partial specialization. Retained in
  `features/Cpp17Gaps/pass/test_attribute_before_function_template.cpp`,
  `features/Cpp17Gaps/pass/test_attribute_after_parameter_list.cpp` and
  `features/Templates/pass/test_alignment_attribute_partial_specialization.cpp`.
- Template parameter lists are not capped at thirty-two:
  `CPC_MAX_TEMPLATE_PARAMETERS` is 128 and the parameter kind masks are word
  arrays behind `mask_test`/`mask_set`/`mask_clear`.
- Runtime headers and the packaged runtime are part of the contract: a program
  that compiles but misbehaves at run time is a compatibility failure, and its
  reduced case belongs in the suite that owns the behaviour.

## Open work

1. The current consumer floor: the repeated `Entry` class definition at
   `boost/date_time/gregorian/gregorian_io.hpp:220`, last seen in
   `Compatibility\build\explorer-probe46.log`. Repair it as described under
   Next action.
2. Then the queue recorded in `Compatibility\KNOWN-ISSUES.md`: the two local
   shapes found while reducing `foreign_ptr.hpp` (`W w(W(new int()));` and the
   assignment operator reached through a conversion from a same-class
   temporary), the pre-existing `std::thread` pointer-to-member case
   (`features/StdConcurrency/pass/test_thread_deferred_member_address.cpp`, red
   and outside the fast list and the gate), the `boost::mp11` member-alias
   floor, the `boost::date_time` `date` base-class-initializer floor, and the
   missing `psapi.h` in the vendored Windows SDK.
3. The Explorer++ consumer build is the large C++17 probe. Its entry point is
   `build_cpc.cmd` (it exports the manifest, then drives root `cpc.exe` through
   `src\scripts\build-project.exe`); its reduced cases and their probe are the
   consumer's own `Scripts\cpc\gaps\*.cpp` and `Scripts\cpc\Test-CpcGaps.ps1`.
   All five reduced cases compile with the published compiler.
4. Grow coverage where nothing is retained yet, one area per cycle: class
   template argument deduction and deduction guides, `constexpr` and lambdas,
   structured bindings, `if constexpr`, fold expressions, inline variables,
   `noexcept`, the standard library (`optional`, `variant`, `any`,
   `string_view`, `chrono`, `filesystem`, `charconv`, PMR, allocators), aligned
   and sized `new`, atomics and threading, exceptions and RTTI across
   translation units, and MSVC x64 ABI shapes.

## Next action

The consumer probe last advanced to
`Compatibility\build\explorer-probe46.log` and stopped at
`boost/date_time/gregorian/gregorian_io.hpp:220` with `struct/union/enum
'__cpc_ns_std_multimap__char____cpc_ns_boost_date_time_string_parse_tree__char____cpc_ns_std_less__char____cpc_ns_std_allocator____cpc_ns_std_pair____cpc_template_type_const_char____cpc_ns_boost_date_time_string_parse_tree__char_Entry'
already defined`.  Reduce that `Entry` redefinition to a standalone case under
`Compatibility\build`, reproduce it with root `cpc.exe`, repair the shared
template/member-class mechanism, retain one minimal case, and run the affected
suite, `-All -Tier fast`, `-Regression` and `src\scripts\build.exe`. The
published checks before this work were features/Templates 969 passed, fast 29
passed and regression 66 passed. After that floor, the queue is item 2 above,
whose head is the residual alias-replay limit a consumer is most
likely to reach: a class template's own member typedef consumed by another
member alias still reports `using alias type expected`
(`Compatibility\build\top2\cases\k04_member_typedef.cpp`).  Reduce it with root
`cpc.exe`, repair the alias replay, retain one minimal case in
`features/Templates/pass/`, and run the affected suite, `-All -Tier fast`,
`-Regression`, then `src\scripts\build.exe`; after that the queue is the two
local shapes recorded in `Compatibility\KNOWN-ISSUES.md`
(`W w(W(new int()));` and the conversion-fed assignment operator), the
member-pointer mangling limit (`Compatibility\build\top2\cases\mp4.cpp`), the
stale diagnostic location (`Compatibility\build\top2\cases\diag2.cpp`), the
`boost::mp11` member-alias floor, and then the consumer probe again for the next
floor.
