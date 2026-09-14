# Main task: lock in C++17 support

Make the remaining confirmed non-passing tests below pass with root `cpc.exe`, then close the remaining C++17 coverage gaps. Prioritize this work over unrelated development. Fix shared compiler/runtime behavior and retain deterministic regression coverage; do not weaken expectations or relabel valid tests as expected failures.

## Remaining confirmed failures

Complete the pedantic audit and reduce any new failures individually. Investigate compiler batch error recovery: an earlier Classes batch stalled after a constructor diagnostic, while the next test passed alone. The historical batch crash cascade is not an inventory of independent defects.

## Remaining coverage before claiming C++17 support

- Extend structured bindings to aggregate and tuple protocols, cv/reference identity, access checks, and meaningful invalid-count diagnostics.
- Finish selection initializer scope/lifetime validation. Complete optional constexpr behavior, conditional special-member availability/triviality, API overloads, and exception contracts.
- Complete fold coverage: operand grammar, nested folds, constexpr identities/dereference, multiple and non-type packs, binary member-pointer folds, overloaded operators, larger folds, and the feature macro.
- Audit `if constexpr`, constexpr lambdas, CTAD, inline variables across translation units, noexcept function types, over-aligned allocation, sequencing, and attributes.
- Audit C++17 library facilities, including the recorded missing `any` and `variant` support; add minimal local lifetime, assignment, access, and exception-contract tests.
- Reduce distinct cases from inspected upstream inputs into `Tests/`, reuse existing equivalent regressions, and track unverified behaviors explicitly. Passing this failure list alone does not establish exhaustive C++17 conformance.

## Execution and acceptance

1. Work with one compiler process at a time and stable root `cpc.exe`; investigate CPC failures using CPC. Use no external compiler without explicit authorization.
2. Repair the remaining failures above. Run each exact reproducer and focused related tests after its fix.
3. Run `Tests/test.exe -Regression` and `Tests/test.exe -All -Tier fast` after the focused repairs. Publish compiler changes through `scripts/build.exe`; use `-RebuildRuntime` when an explicit runtime ABI/code-generation refresh is needed. Preserve root `cpc.exe` and `lib/` if publication fails.
4. After the substantial C++17 changes, run the relevant full suites and the pedantic gate serially to detect failures masked by the original crash. Record compiler identity and fresh evidence in `build/`, update the coverage data, and remove resolved work from this document.
5. Close the remaining coverage gaps before declaring C++17 support locked in; retain all valid positive tests and intended rejection tests with their original semantics.
