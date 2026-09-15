# C++17 remaining scope

`cpp17-coverage.json` is the durable feature map. This page is the working
queue, not a history of individual experiments or raw test output.

## CL gap suite

`Tests/features/Cpp17Gaps` holds the minimal reproducers ported from the CL
repository's C++17 gap probe (`C:\Luke\Src\CL\CpcRegressions.txt` and
`tmp_cpc_gaps/results.csv`). The probe was a 139-case single-feature sweep; the
suite keeps one file per distinct surviving defect, drops cases that the current
root `cpc.exe` already passes, and drops duplicate or transitive-include-only
cases.

Run it with `Tests\test.exe -Suite features/Cpp17Gaps`. The former work list is
now empty: every gap case passes on the published root compiler and belongs to
the fast partition.

Status: no case remains in the pedantic work list. All 45 gap cases pass on
root `cpc.exe` SHA256
`6AE287BF0A7F621D742707CE15A4EB7D295E70BCB069D1146DDCC5E08DC0691F`.
The repaired areas are the C++17 `__cplusplus` value and alternative operator
spellings, class-head `alignas`, structured bindings in range-for, the runtime
headers and definitions listed by the gap probe, tuple construction and
`apply`, vector initializer-list construction, and the missing members in
`bitset`, `optional`, `memory`, `mutex`, `atomic`, `iterator`, `utility`,
`algorithm`, `regex`, and `new`.

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
