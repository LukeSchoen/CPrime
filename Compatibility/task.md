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

- `Compatibility\KNOWN-ISSUES.md` records reported defects with reduced shapes: the
  `InterlockedIncrement` return value, inline SSE asm corrupting surrounding
  float code, the missing `psapi.h` in the vendored Windows SDK, and the
  `__m256`/`immintrin.h` stub that blocks SSE/AVX types.
- The Explorer++ consumer build (`C:\Luke\Src\Archive\explorerplusplus`,
  reduced cases and a re-run command in `Scripts\cpc\gaps` and
  `Scripts\cpc\Test-CpcGaps.ps1`) now compiles every reduced floor case. The
  packaged SDK includes gdiplus, and the first translation units get through
  the Windows headers, the locale/classification headers, and into the
  `result_of` support headers. The current floor is the compiler's 16-template-
  parameter limit in `boost/utility/detail/result_of_iterate.hpp`. Shapes and
  the remaining work are in `Compatibility\KNOWN-ISSUES.md`.
- `Compatibility\tests\test.exe -Checks` (the CPC-only publication/development gate) is
  currently red: the runner self-check `runner rejects false expectation:
  test_valid_without_main.cpp` reports exit 1 and `Summary: 0 passed, 1 failed`
  but the retained diagnostic string no longer matches. Find out whether the
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
