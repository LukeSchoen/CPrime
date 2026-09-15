# C++17 language-support work

The root `cpc.exe` is the only compiler used for this work. Keep one compiler
process active at a time. A valid test stays valid: repair the shared compiler
or runtime behavior and retain a minimal deterministic regression in `Tests/`.

## Current checkpoint

- Root `cpc.exe` (SHA256 `18B7E339BBC998AB1A5E2BF561F41ADA88D083C8C5E722EFD8C3976BC47401D0`)
  is published from this source state and
  passes the publication gate: source policy, 58 regression cases, the process
  path helper, and 311 fast cases across 27 suites. The pedantic partition passes
  everywhere except the CL gap work list below.
- Member-pointer constexpr evaluation now covers calls, assignments, equality,
  null conversion, receiver adjustment, access checks, and C-style conversion
  controls.
- `Tests/features/Cpp17Gaps` holds the minimal reproducers ported from the CL
  repository's C++17 gap probe: 42 distinct surviving defects. They are listed
  in the pedantic partition so the fast gate stays usable while the list is
  worked down. Scalar return lowering now materializes a local deque
  `front()`/`back()` value before local cleanup, and member-operator lowering
  preserves scalar reference destinations for `basic_istream::operator>>`.
  Both reproducers are promoted to the fast tier. Inline static data members
  now define and initialize addressable storage in their class body;
  `test_inline_variable_static_member.cpp` is also promoted.
- The member-pointer ambiguity check is repaired and published.
  `class_has_unique_base` now counts base subobjects, so virtual inheritance
  paths no longer escape the `.*`/`->*` declaring-base check, and identity is no
  longer reported as a base. That identity case also made
  `class_value_is_derived_from(T, T)` true, which silently removed the C++17
  prvalue return slot and forced a move construction from `return T(...)`
  (including the packaged `<future>`). Retained as
  `features/Constructors/pass/test_prvalue_return_elision_with_deleted_copy.cpp`.
- A struct, union, or enum tag is no longer an ordinary type name in C: after
  `struct T;` the spelling `T *p;` is rejected while `struct T *p;` and explicit
  typedefs still work, and C++ keeps the C++-style tag spelling. Retained as
  `c_compat/fail/test_undeclared_struct_tag_typedef_name.c`; a broad C header
  smoke test (`windows.h`, `shellapi.h`, `objbase.h`, libc) still compiles.
- `get_temp_local_var` callers all pass an `int` slot; the constexpr temporary
  allocator fills an `int` local and narrows into `SValue::r2` instead of writing
  four bytes through the two-byte field.
- This is a progress checkpoint, not a C++17 conformance claim. Historical raw
  logs are disposable; `Tests/cpp17-coverage.json` records the durable scope.

## Next wave

1. Work down `Tests/features/Cpp17Gaps` from the runtime miscompiles outward:
   deque `front()`/`back()` returned directly, `stringstream` extraction, then
   the missing core-language and library
   facilities. Each case is a standalone `-Select` reproducer.
2. Complete constexpr object evaluation: aggregate returns and copies, indirect
   calls, nontrivial construction, union/bit-field/reference fields, ownership,
   provenance, temporary cleanup, and lifetime escape rejection.
3. Complete constexpr statements and local state: class ranges/sentinels,
   selection and loop scopes, mutation through subobjects, initialization order,
   references, pointer bounds, and discarded runtime calls.
4. Finish the lexer and literal path: user-defined literals, separators, raw and
   encoded strings, concatenation, Unicode escape diagnostics, and static-assert
   source locations.
5. Extend the remaining C++17 core features: structured bindings, variable
   templates, folds, `if constexpr`, lambdas, CTAD, inline variables, noexcept
   function types, allocation, sequencing, and attributes.
6. Finish deleted-function and constructibility semantics, then add focused
   coverage for missing library facilities, including `any` and `variant`.

Start each item with one standalone reproducer. Add only independent cases;
reuse an existing regression when it already proves the same behavior. Preserve
batch-state recovery after a rejected translation unit.

## Test cadence

- During a repair, run the exact selected tests and the smallest related suite.
- After a coherent repair, run `Tests/test.exe -Regression`, then the affected
  fast suite. Run `Tests/test.exe -All -Tier fast` only when a change crosses
  suite boundaries or before publication.
- Put broad, duplicate, costly, stress, or cross-feature coverage in
  `Tests/tiers.json`'s pedantic partition. Keep fast tests short, representative,
  and individually diagnosable.
- Publish with `scripts/build.exe` only after focused checks pass. For a large
  wave, run the relevant pedantic suites once on the published root compiler.

Generated executables, objects, logs, and temporary runner directories belong
under `build/` and are not project history. Keep only current evidence needed
to diagnose an active failure; remove it after recording the durable result in
tests or coverage metadata.

## Exit condition

Do not claim C++17 support is locked until the listed feature areas have focused
coverage, the fast gate and relevant pedantic gate pass on a freshly published
root compiler, and outstanding negative tests are checked for the intended
diagnostic rather than mere rejection.
