# GCC C++ compatibility work, 2026-09-08

The GCC suites are downloaded and reproducibly pinned. **The full suite does
not pass yet.** This change establishes an assessment runner and fixes the
first group of concrete compiler failures without modifying upstream tests.

The checkout contains 23,035 C++ source files under `g++.dg` and
`g++.old-deja`, plus 3,416 shared C/C++ sources. The initial count of 22,832
covered only files ending in `.C`; the complete inventory also includes other
source extensions. Both complete surveys ran serially, with a two-second
per-process limit and no compiled-result reuse.

## Comparable C++ results

These numbers exclude the shared C/C++ trees. They describe the adapter's
standalone checks in CPC's default language mode, not full DejaGnu conformance.

| Result | Before | After |
| --- | ---: | ---: |
| Checked compile, object, preprocess, link, or runtime passes | 2,725 | 2,748 |
| Checked compile failures | 1,324 | 1,313 |
| Checked runtime failures | 106 | 106 |
| Compiler crashes, including unverified probes | 156 | 144 |
| Compiler timeouts, including unverified probes | 5 | 5 |
| Unverified probe accepted | 7,477 | 7,538 |
| Unverified probe rejected | 11,242 | 11,181 |

Twenty-three previously failing cases now pass. No previously passing case
regressed in the complete comparison. Across all 26,451 surveyed sources,
including shared sources, crashes decreased from 160 to 148.

The checked C++ subset contains 4,176 sources; 18,859 others have expectations
the adapter does not implement. Probe acceptance/rejection is an observation,
not a pass/fail verdict. The remaining checked subset includes six crashes
and three timeouts. Moving a crash to a diagnostic is reported separately
from fixing the underlying language feature.

The aggregate counts and all 23 newly passing filenames are preserved in
[results-2026-09-08.json](results-2026-09-08.json). Full commands, compiler hashes,
diagnostics and execution results are in `build/gcc-all-baseline/` and
`build/gcc-all-after/`. Those historical raw surveys included all shared
directories; the adapter now explicitly marks specialized shared directories
as requiring their own GCC driver. The C++ comparison above is unaffected.

## Compiler fixes

- Global `new` expressions leave constant-expression probing and execute in
  the existing generated startup initializer. This fixes scalar/class array
  allocation crashes, including initialization before `main`.
- Empty C++ enums are accepted, including class-local enums used in bitfields.
- Scalar `static_cast` to a const reference materializes a converted temporary
  when required and retains aliasing when the types already match.
- Class `static_cast` considers explicit constructors while retaining existing
  member-pointer conversion and base-class conversion paths.
- Overload matching accepts a standard pointer conversion followed by binding
  a const reference to the resulting pointer temporary.
- Long `else if` chains use an iterative parser path when no condition-scope
  declarations must remain alive. The 11,000-branch upstream stack-overflow
  reproducer and a corresponding runtime branch-selection test pass.
- Built-in increment/decrement can use a class conversion to a modifiable
  scalar reference. Ambiguous conversions and value-only conversions are
  rejected; inherited conversions and single evaluation have runtime coverage.
- Template attributes no longer confuse declaration-name discovery. Signature
  analysis hashes literal payloads as data instead of interpreting them as
  identifier tokens. Saved string-token padding is zeroed so those hashes do
  not depend on previously allocated memory; a string-bound template
  declaration/definition is covered by the regression test.
- Incomplete-template layout probing inspects the class tag's own layout
  instead of dereferencing its empty embedded type reference. A minimal
  invalid incomplete-object declaration now gets a diagnostic without crashing.
  Several valid partial-specialization cases still require further fixes.

The old `test_converted_pointer_const_reference.cpp` failure is fixed.
The old all-in-one list test was missing `<stdlib.h>`; Clang independently
rejected its undeclared `malloc`, `free`, and `realloc` calls. Its support
source now includes that header. This was a fixture defect, not the previously
reported compiler overload bug.

## Validation and remaining work

CPC's 18 surrounding suites pass 1,164 tests. The test runner now rejects
abnormal compiler exit statuses even for expected-error tests, with a fake
crash fixture validating that rule. The GCC adapter has seven classification
tests covering nested selectors, unsupported options, diagnostic expectations,
assembly checks, and malformed directives.

The next substantial compiler areas are recursive template completion/ADL,
template-template partial specialization, covariant virtual returns, and
initialization/lifetime cases. Remaining examples include `lookup/koenig12.C`,
`template/deduce2.C`, `template/partial13.C`, and `inherit/covariant1.C`.
Large constructor-expansion cases also hit the survey's short time limit.

The harness still needs real standard-version selection, target predicates,
diagnostic matching, multi-source driver support, and GCC-specific checks.
Those cases remain visible and unverified; they are not hidden behind expected
failures or counted as compatibility successes.

## Final compiler and OTServ checks

Both the installed Clang `-O3` host and CPC `-O2` self-host pass all
1,164 surrounding CPC tests and all 23 repaired GCC cases after the
final string-token padding fix. Successive optimized bootstrap stages
have identical executable code. Binary hashes are recorded in the JSON
report; the earlier full-survey hashes remain separate historical evidence.

Fresh serial OTServ builds used the existing 215-source, 19-unit unity
configuration without compiled-result reuse. Compiler time includes
compilation, archive creation, and linking; outer wall time also includes
the root build helper and resource/driver overhead.

| Compiler host | Compiler work | Outer wall time |
| --- | ---: | ---: |
| clang | 12.799s | 14.665s |
| selfhost | 20.363s | 22.167s |

Both hosts generated identical executable sections for all 19 objects
and produced `C:/Luke/Src/OT/cl/builds/OTServ_prime.exe`.
These are compile/link checks, not OTServ gameplay validation.

An earlier alternating full-build comparison measured 25.995s for the
updated self-host versus 23.982s for the retained previous self-host.
A final adjacent comparison measured 22.009s previous versus 22.224s
updated (compiler work: 20.234s versus 20.381s). The final pair is within
1%, so the earlier apparent slowdown was not consistently reproduced.
Isolated parser variants did not identify a consistent single cause;
the suspected hot-function stack-frame increase was disproved.
These measurements do not establish a speed improvement, and the
self-host remains above the earlier under-15-second full-build target.
