# Compatibility: endlessly correct C++17

Make root `cpc.exe` accept every valid C++17 program and reject invalid ones
with the right diagnostic, so real C++17 projects compile unmodified. Coverage
never ends: when the current gaps close, find the next real project or language
area that breaks and close that too. Never create `done.x`.

Work in this tree only. Delete nothing: `Compatibility\worker.cmd` and this file
are the user's control surface, and other worker folders may be in use on other
days.

## Where compatibility evidence comes from

1. Retained internal cases, which are the record of what works:
   `Compatibility\tests\features\**`, `Compatibility\tests\integration\**`,
   `Compatibility\tests\abi\**`, `Compatibility\tests\runtime\**`,
   `Compatibility\tests\payload\**`, `Compatibility\tests\c_compat\**`. The Microsoft
   x64 ABI facts live in `Compatibility\tests\features\Abi`.
2. Large C++ projects used as probes: the vendored competitive inputs under
   `Cost\tests\compile\competitive\` (xBRZ and the functions/pch cases),
   and the user's own consumers (for example `C:\Luke\Src\Kinect`, which is read
   only and whose findings are recorded in `Compatibility\KNOWN-ISSUES.md`). Compile a project
   with root `cpc.exe`, reduce each failure to a minimal local case, and never
   edit the consumer.
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

## Leads

- The two fast-tier gaps are closed, and the shape behind them is now covered
  twice over.  A standard attribute-specifier sequence is accepted anywhere in
  the decl-specifier-seq and at the end of a function's
  parameters-and-qualifiers, so `inline [[noreturn]] void f(int const &);` and
  `int f(int) [[noreturn]];` both parse; retained in
  `features/Cpp17Gaps/pass/test_attribute_before_function_template.cpp` and
  `features/Cpp17Gaps/pass/test_attribute_after_parameter_list.cpp`.  Decision:
  a compiler-side merge of a using-declaration import with a same-signature
  declaration in the importing namespace must stay rejected, because
  `features/Namespaces/fail/test_using_merged_overloads_ambiguous.cpp` requires
  that diagnostic, so the duplicated `<cstdlib>`/`<cmath>` overload set was
  repaired in the runtime headers, where the reference headers import one set
  per name instead of redeclaring it in `namespace std`.
- The `features/Atomics/pass/test_atomic_typedefs.cpp` floor is closed: the
  standard typedefs are published in `src/include/runtime/atomic`.  The crash
  was a double free in conversion-operator parsing:
  `make_type_from_saved_type_tokens` consumes its token string, but three
  callers freed it again.  The token-pool corruption became fatal once the
  typedef set enlarged the stream.  The fast list is now empty.  The known
  closed floors and retained cases are recorded in
  `Compatibility\KNOWN-ISSUES.md`.
- Work the defects recorded in `Compatibility\KNOWN-ISSUES.md`: the `::`-spelled
  member function template replay, the `InterlockedIncrement` return value,
  inline SSE asm corrupting surrounding float code, the missing `psapi.h` in the
  vendored Windows SDK, and the `__m256`/`immintrin.h` stub that blocks SSE/AVX
  types.
- The Explorer++ consumer build (`C:\Luke\Src\Archive\explorerplusplus`) is the
  large C++17 probe. Its entry point is `build_cpc.cmd` (it exports the
  manifest, then drives root `cpc.exe` through `src\scripts\project.exe`); its
  reduced cases and their probe are the consumer's own
  `Scripts\cpc\gaps\*.cpp` and `Scripts\cpc\Test-CpcGaps.ps1`. All five reduced
  cases compile with the published compiler.  With the atomic typedefs
  published and the three floors since closed -- `ordered_range_t()` as a
  constant class initializer, the replayed `std::basic_string` constructor
  under a static initializer fold, and the missing `<cfloat>` runtime header --
  the build now stops in `boost/mpl/vector/aux_/vector0.hpp:45`; the reduced
  case and the work it needs are in `Compatibility\KNOWN-ISSUES.md`.
- `Compatibility\tests\test.exe -Checks` (the CPC-only publication/development gate) is
  red: the runner self-check `runner rejects false expectation:
  test_valid_without_main.cpp` reports exit 1 and `Summary: 0 passed, 1 failed`
  but the retained diagnostic string does not match. Find out whether the
  runner or the retained expectation is wrong; the check must stay strict
  either way.
- Grow coverage where nothing is retained yet, one area per cycle: class
  template argument deduction and deduction guides, `constexpr` and lambdas,
  structured bindings, `if constexpr`, fold expressions, inline variables,
  `noexcept`, the standard library (`optional`, `variant`, `any`,
  `string_view`, `chrono`, `filesystem`, `charconv`, PMR, allocators), aligned
  and sized `new`, atomics and threading, exceptions and RTTI across
  translation units, and MSVC x64 ABI shapes.
- Runtime headers and the packaged runtime are part of the contract: a program
  that compiles but misbehaves at run time is a compatibility failure, and its
  reduced case belongs in the suite that owns the behaviour.

## Next action

Retain the reduced `boost::mpl` `vector0<>` floor as one minimal case under
`features/Templates/pass`, add it to the fast list, reproduce it with root
`cpc.exe`, repair the nested template-id parse, then run the suite, fast tier,
regression gate, and publish:

```
Compatibility\tests\test.exe -Suite features/Templates -Select <retained-case>.cpp
Compatibility\tests\test.exe -Suite features/Templates
Compatibility\tests\test.exe -All -Tier fast
Compatibility\tests\test.exe -Regression
src\scripts\build.exe
```

The reduced case is
`Compatibility\build\mpl-vector-probes\m1_self_empty_argument.cpp`: inside
`template<> struct vector0<na>`, the member typedef
`v_iter<vector0<>, 0> begin;` is rejected with
`'>' expected after template argument '...vector0...na' opened near line 16
(got '<')`, while the same shape outside the specialization
(`m2_outside.cpp`) parses.  The consumer probe log is
`Compatibility\build\explorer-probe11.log`.  After publishing, re-run
`C:\Luke\Src\Archive\explorerplusplus\build_cpc.cmd Release x64` (it copies
root `cpc.exe` in first) and reduce the next floor it reports.  Evidence for
the floors closed this cycle is in `Compatibility\build\`: the `const-class-init-*`,
`replayed-ctor-*`, `include-cfloat-*`, `container-fwd.*`, `explorer-probe10.log`
and `explorer-probe11.log` logs and the `pending-flush-probes` scratch cases,
plus the published suite/gate runs.
