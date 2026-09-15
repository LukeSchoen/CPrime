# C++17 remaining scope

`cpp17-coverage.json` is the durable feature map. This page is the working
queue, not a history of individual experiments or raw test output.

## First priority: constexpr object model

- Aggregate returns and copies through indirect/member calls, nontrivial
  construction, union, bit-field, and reference-member forms.
- Evaluation-owned storage: pointer provenance, subobjects, array bounds,
  temporary destruction, reference binding, and escape rejection.
- Local mutation and initialization: subobjects, constructors, declaration
  scope, lifetime, discarded runtime calls, and failure recovery.
- Statement evaluation: class range/sentinel lookup, selection and loop
  conditions, control flow, and iterator lifetime.

Retain the member-pointer, aggregate-return, range, temporary, and batch-reset
regressions already in the feature suites. Add a test only when it proves a
distinct semantic rule.

## Second priority: parsing and core C++17

- User-defined literals, digit separators, raw/encoded literals, Unicode
  escapes, concatenation, and static-assert locations.
- Structured bindings, variable templates, fold expressions, `if constexpr`,
  lambdas, CTAD, inline variables, noexcept types, allocation, sequencing, and
  attributes.
- Deleted functions, special-member availability, constructibility traits, and
  minimal `any`/`variant` behavior.

## Execution

Use `Tests/test.exe` with the smallest applicable `-Suite` and `-Select` list
while developing. Run regression plus the affected fast suite after each repair.
Use `-All -Tier fast` at a boundary, and reserve pedantic for broad, expensive,
or cross-feature coverage. Use CPC only and one compiler process at a time.
