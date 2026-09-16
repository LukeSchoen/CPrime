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
- The fast-tier gap is `features/Atomics/pass/test_atomic_typedefs.cpp`: the
  standard `std::atomic_*` typedefs of [atomics.types.generic] are missing from
  `src/include/runtime/atomic`, and adding them makes the compiler die with
  `0xC0000005` on any translation unit that reaches `<atomic>` through
  `<string>`/`<memory>`.  `<atomic>` alone and the same typedef list in the
  translation unit proper are both accepted, so the crash is the compiler's,
  not the header's.  Reproduce it with the staged tree and probe in
  `Compatibility\build\atomic-alias-crash` (`notes.md` has the command and the
  bisection table).  Fix the crash first: publishing the typedefs is what
  unblocks `boost/smart_ptr/detail/sp_counted_base_std_atomic.hpp` and the
  consumer floors behind it.  This closed cycle's floors and their retained
  cases are recorded in `Compatibility\KNOWN-ISSUES.md`; the next floor behind
  this one is `boost/container/container_fwd.hpp`'s
  `constexpr increment requires an evaluation-owned object`.
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
  cases compile with the published compiler.  The build now stops in
  `boost/smart_ptr/detail/sp_counted_base_std_atomic.hpp`, which needs the
  `std::atomic_*` typedefs (see the fast-tier gap above); compile it with root
  `cpc.exe`, reduce each failure to a minimal local case, close the shared
  mechanism, and continue from the next floor.
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

Reproduce the crash with the exact case, repair the shared mechanism, publish
the typedefs and republish:

```
Compatibility\tests\test.exe -Suite features/Atomics -Select test_atomic_typedefs.cpp
cpc.exe -B Compatibility\build\atomic-alias-crash\stage -std=c++17 Compatibility\build\atomic-alias-crash\probe.cpp -o Compatibility\build\atomic-alias-crash\probe.exe
Compatibility\tests\test.exe -Suite features/Atomics
Compatibility\tests\test.exe -All -Tier fast
Compatibility\tests\test.exe -Regression
src\scripts\build.exe
```

The case leaves the fast list by passing, never by removal. Then re-run the
consumer probe with the published compiler --
`C:\Luke\Src\Archive\explorerplusplus\build_cpc.cmd Release x64`, which copies
root `cpc.exe` in first -- and reduce the next floor it reports. Reproduction
evidence for this cycle is in `Compatibility\build`: the consumer floors in
`explorer-probe2.log` through `explorer-probe8.log`, the alias crash in
`atomic-alias-crash`, and the suite/fast/gate runs in `classes-suite-final.log`,
`atomics-suite.log`, `fast-final.log` and `regression-final.log`.  The
`gregdur-probe*.cpp` and `small_*.cpp` reductions, `crash_probe1.cpp`,
`roundstyle_probe.cpp` and `string-pair-*.log` are the reductions behind the
closed floors.
