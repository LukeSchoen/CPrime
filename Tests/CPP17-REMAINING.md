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

Run it with `Tests\test.exe -Suite features/Cpp17Gaps`. Every case is listed in
`tiers.json` under `pedantic`, so the fast partition and the publication
`-Checks` gate are unaffected while the list is outstanding; the pedantic
partition reports them as failures until they are repaired. Remove a case from
that list when it passes on a published root compiler.

Outstanding groups:

- Runtime miscompiles: `static inline` data member reads fault, a local deque's
  `front()`/`back()` returned directly reads freed storage, and `stringstream`
  extraction never assigns.
- Core language: CTAD, explicit `constexpr` lambdas, class-specifier `alignas`,
  structured bindings in range-for, native alternative operator tokens, and the
  `__cplusplus` language-mode value.
- Missing headers: `string_view`, `variant`, `any`, `filesystem`, `shared_mutex`,
  `list`, `forward_list`, `stack`, `set`, `random`, `complex`, `numeric`,
  `charconv`, `system_error`, `typeindex`, `memory_resource`, and `execution`.
- Missing contents in present headers: `std::queue` is undeclared; `std::byte`,
  `std::scoped_lock`, `std::make_unique`, `std::not_fn`, `std::bind` results,
  `std::smatch` overloads, `std::aligned_storage`, and `std::atomic_flag` are
  absent; `std::clamp`, `std::as_const`, `std::size`, `std::make_tuple` are
  declared without definitions; `std::launder`, `std::tuple` construction,
  `std::vector`'s initializer_list constructor, `std::bitset::count`, and
  `std::optional::value_or` do not resolve.

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
