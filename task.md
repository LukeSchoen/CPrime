# Main task: lock in C++17 support

Make all 15 confirmed non-passing tests below pass with root `cpc.exe`, then close the remaining C++17 coverage gaps. Prioritize this work over unrelated development. Fix shared compiler/runtime behavior and retain deterministic regression coverage; do not weaken expectations or relabel valid tests as expected failures.

## Remaining confirmed failures

Reproduced individually and serially on 2026-09-14 with `Tests/test.exe -Suite <suite> -Select <filename>`. Paths below are relative to `Tests/`. Root compiler SHA256 before and after these checks: `1DCB3E2A73A9ABBEA029EEBE7A3CB5D65ED069E6E43392C9AD9BBC0BE379027B`. Per-test evidence is in `build/cpp17-current-<filename>.log`, including the native runner's diagnostic directory.

| Test | Current failure |
| --- | --- |
| `features/Declarations/pass/test_cpp17_structured_binding_array.cpp` | Compiler result mismatch; compiler exit 1 |
| `features/Statements/pass/test_cpp17_selection_init.cpp` | Compiler result mismatch; compiler exit 1 |
| `features/Includes/pass/test_cpp17_optional_lifetime.cpp` | Compiler result mismatch; compiler exit 1 |
| `features/Templates/pass/test_auto_return_repeated_forwarding_chain.cpp` | Missing compiler result; compiler crash, exit 3221225725 |
| `features/Templates/pass/test_builtin_type_specifier_arguments.cpp` | Compiler result mismatch; compiler exit 1 |
| `features/Templates/pass/test_out_of_class_conversion_operator_identity.cpp` | Compiler result mismatch; compiler exit 1 |
| `features/Constructors/pass/test_const_default_member_initialization.cpp` | Runtime mismatch; exit 2 |
| `features/Constructors/pass/test_function_static_aggregate_initialization.cpp` | Runtime crash; exit 3221226505 |
| `features/Constructors/pass/test_global_aggregate_dynamic_initialization.cpp` | Runtime mismatch; exit 1 |
| `features/Expressions/pass/test_constant_string_address.cpp` | Compiler result mismatch; compiler exit 1 |
| `features/Expressions/pass/test_generic_lambda_member_capture.cpp` | Compiler result mismatch; compiler exit 1 |
| `features/Expressions/pass/test_nested_lambda_capture_scope_and_identity.cpp` | Compiler result mismatch; compiler exit 1 |
| `features/InlineLifecycle/pass/test_inline_ctor_dtor_with_parameters.cpp` | Compiler result mismatch; compiler exit 1 |
| `features/InlineLifecycle/pass/test_scoped_ctor_dtor_with_parameters.cpp` | Compiler result mismatch; compiler exit 1 |
| `features/OperatorOverloads/pass/test_conversion_non_type_template.cpp` | Compiler result mismatch; compiler exit 1 |

This is the current reproduction of the known failure inventory, not a fresh exhaustive suite run. Reconcile `Tests/cpp17-verification.json` and `Tests/cpp17-coverage.json` against stable compiler evidence as fixes land. The historical 669 failures include a template batch crash cascade and must not be treated as 669 independent defects.

## Remaining coverage before claiming C++17 support

- Extend structured bindings to aggregate and tuple protocols, cv/reference identity, access checks, and meaningful invalid-count diagnostics.
- Validate selection initializer scope and lifetime, and optional construction/destruction without requiring a default-constructible contained type.
- Complete fold coverage: operand grammar, nested folds, constexpr identities/dereference, multiple and non-type packs, binary member-pointer folds, overloaded operators, larger folds, and the feature macro.
- Audit `if constexpr`, constexpr lambdas, CTAD, inline variables across translation units, noexcept function types, over-aligned allocation, sequencing, and attributes.
- Audit C++17 library facilities, including the recorded missing `any` and `variant` support; add minimal local lifetime, assignment, access, and exception-contract tests.
- Reduce distinct cases from inspected upstream inputs into `Tests/`, reuse existing equivalent regressions, and track unverified behaviors explicitly. Passing this failure list alone does not establish exhaustive C++17 conformance.

## Execution and acceptance

1. Work with one compiler process at a time and stable root `cpc.exe`; investigate CPC failures using CPC. Use no external compiler without explicit authorization.
2. Repair the failures above, starting with the template compiler crash and the three direct C++17 feature failures. Run each exact reproducer and focused related tests after its fix.
3. Run `Tests/test.exe -Regression` and `Tests/test.exe -All -Tier fast` after the focused repairs. Publish compiler changes through `scripts/build.exe`; use `-RebuildRuntime` when an explicit runtime ABI/code-generation refresh is needed. Preserve root `cpc.exe` and `lib/` if publication fails.
4. After the substantial C++17 changes, run the relevant full suites and the pedantic gate serially to detect failures masked by the original crash. Record compiler identity and fresh evidence in `build/`, update the coverage data, and remove resolved work from this document.
5. Close the remaining coverage gaps before declaring C++17 support locked in; retain all valid positive tests and intended rejection tests with their original semantics.
