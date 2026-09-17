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
- The `boost/math` parenthesized-template-declarator floor is closed: the
  template declaration scan now normalizes `T (f)(args)` to `T f(args)` before
  registration, so the declared function name and its signature helpers agree
  with the rest of template processing.  Retained as
  `features/Templates/pass/test_parenthesized_template_definition_then_plain.cpp`;
  evidence is in `Compatibility\build\parenthesized-template-*.log` and
  `Compatibility\build\explorer-probe13.log`.
- Work the defects recorded in `Compatibility\KNOWN-ISSUES.md`.  Closed this
  cycle: the `::`-spelled parameter list (a free or member function template
  definition, and `new ::T(args)`), the `__m256`/`immintrin.h` stub (the
  runtime header now carries the SSE/AVX/AVX-512 types and kernels the CNN
  kernels use, and `faceDetectCNN.cpp` compiles), and the inline-SSE GEMM
  report, which does not reproduce with the published compiler and is retained
  as a passing case.  Still open: the missing `psapi.h` in the vendored
  Windows SDK, and the `is_base_and_derived_select` dependent typedef floor
  that stops the Explorer++ probe.
- The Explorer++ consumer build (`C:\Luke\Src\Archive\explorerplusplus`) is the
  large C++17 probe. Its entry point is `build_cpc.cmd` (it exports the
  manifest, then drives root `cpc.exe` through `src\scripts\project.exe`); its
  reduced cases and their probe are the consumer's own
  `Scripts\cpc\gaps\*.cpp` and `Scripts\cpc\Test-CpcGaps.ps1`. All five reduced
  cases compile with the published compiler.  With the atomic typedefs
  published and the three floors since closed -- `ordered_range_t()` as a
  constant class initializer, the replayed `std::basic_string` constructor
  under a static initializer fold, and the missing `<cfloat>` runtime header --
  and the `boost::mpl` `vector0<>`, parenthesized template-declarator and
  `std::ostreambuf_iterator` floors now closed, the build stops in
  `boost/date_time/gregorian/gregorian_io.hpp:49` with
  `nested template type member '__cpc_ns_std_locale::facet' must be a typedef`
  (`Compatibility\build\explorer-probe14.log`).  The reduced cases and the work
  they need are in `Compatibility\KNOWN-ISSUES.md`.
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

`std::locale::facet` is closed and published together with the facet registry,
`basic_ios::getloc`/`imbue`, and the depth-independent lookup that lets a
registered facet be found from another translation unit.  The retained case is
`features/Includes/pass/test_locale_facet.cpp`; the reduced probes
`Compatibility\build\locale-probes\p1_facet_base.cpp` and
`p2_facet_on_stream.cpp` compile and run, and the fast tier (28) and regression
gate (58) passed before the compiler was republished.

`InterlockedIncrement`/`InterlockedDecrement` are also closed: the runtime now
owns the two intrinsics (`src/runtime/windows/winintrin.S`, declared through
`cprimedefs.h`) because the vendored header's inline-asm version inferred the
new value from flags the register allocator could clobber.  Retained as
`features/Abi/pass/test_msvc_interlocked_counter.cpp`.

This cycle closed the remaining fronts of `Compatibility\KNOWN-ISSUES.md` that
had a reproducer.  The `::`-spelled parameter list is one classifier defect:
`(::type name)` was read as a direct initializer, so an out-of-class member
template lost its declarator and `new ::T(args)` lost its constructor
arguments.  Both are retained
(`features/Templates/pass/test_leading_global_scope_parameter_in_member_template.cpp`,
`features/Expressions/pass/test_new_global_scope_qualified_type.cpp`).  The
`immintrin.h` stub is replaced by the real SSE/AVX/AVX-512 types and kernels
(`features/Intrinsics/pass/test_sse_avx_intrinsics.cpp`) and the consumer's
`faceDetectCNN.cpp` now compiles.  The inline-SSE GEMM report does not
reproduce: the reduced shape is retained as
`features/GnuExtensions/pass/test_inline_sse_asm_with_scalar_tail.cpp`, and the
consumer's own kernels verify with `--smoke` 9/9 and `--verify` at
`max abs error 2.289e-05` under `KPOSE_GEMM=sse`.  The four new cases joined
the regression gate, which is now 62 cases; the fast tier (29) and the gate
(62) pass, and the compiler and runtime package were republished.

The next action is to re-run the consumer probe and reduce its next floor:

```
C:\Luke\Src\Archive\explorerplusplus\build_cpc.cmd Release x64
```

It copies root `cpc.exe` in first and exports its own manifest.  That has been
done for this cycle: `Compatibility\build\explorer-probe15.log` shows the probe
clearing `gregorian_io.hpp`/`std::locale::facet` and stopping in
`boost/date_time/gregorian/greg_weekday.hpp:215`, on the dependent
`typedef typename ...::type` chain through
`boost::detail::is_base_and_derived_select`.  The reduction so far is
`Compatibility\build\date-time-probes\svp2.cpp` with its `svp2.log`
(`svp_first_order.cpp` is the same idea with the other include order, which
lands on a different error first).

The exact next action is to reduce that `is_base_and_derived_select` chain to a
CPrime-local case, fix the replayed class-template member typedef lookup, retain
the case under `features/Templates`, and re-run the probe again.
