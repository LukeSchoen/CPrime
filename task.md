# C++17 language-support work

## Immediate compiler correctness / Clang-compatibility defects

1. **C parser accepts a typedef name before its declaration.**
   `include/cprime/cprime.h:366` declares
   `Sym *bound_member_receiver_object;` in `SValue`, but the defining
   `typedef struct Sym { ... } Sym;` does not appear until line 452. In C,
   `Sym` is not a type name at line 366; only `struct Sym *` is valid. CPC
   currently accepts this invalid C input while Clang correctly rejects it.
   Repair the declaration ordering or use `struct Sym *`, then add a minimal
   C negative regression proving an undeclared typedef name is rejected.
2. **Constexpr temporary allocator writes an `int` through a 16-bit field.**
   `src/compiler/frontend/cprimegen_constexpr.inc:905` passes
   `&result->r2` (`unsigned short *`) to `get_temp_local_var`, whose declared
   parameter is `int *` (`cprimegen.c:4597`). This is an incompatible pointer
   conversion and can write four bytes into the two-byte `SValue::r2` field.
   Make the slot type and allocator interface agree, audit every caller, and
   add a focused constexpr temporary-object regression.

The root `cpc.exe` is the only compiler used for this work. Keep one compiler
process active at a time. A valid test stays valid: repair the shared compiler
or runtime behavior and retain a minimal deterministic regression in `Tests/`.

## Current checkpoint

- Member-pointer constexpr evaluation now covers calls, assignments, equality,
  null conversion, receiver adjustment, access checks, and C-style conversion
  controls.
- `Tests/features/Cpp17Gaps` now holds the minimal reproducers ported from the CL
  repository's C++17 gap probe: 44 distinct surviving defects, three of them
  silent runtime miscompiles. They are listed in the pedantic partition so the
  fast gate stays usable while the list is worked down.
- The member-pointer ambiguity check is repaired in source.
  `class_has_unique_base` now counts base subobjects, so virtual inheritance
  paths no longer escape the `.*`/`->*` declaring-base check, and identity is no
  longer reported as a base. That identity case also made
  `class_value_is_derived_from(T, T)` true, which silently removed the C++17
  prvalue return slot and forced a move construction from `return T(...)`
  (including the packaged `<future>`). Retained as
  `features/Constructors/pass/test_prvalue_return_elision_with_deleted_copy.cpp`.
  A candidate compiler passes the publication regression set (58/58) and every
  fast suite; root `cpc.exe` predates the repair, so publishing is the last step.
- The last published compiler passed 58 regression cases, 308 fast cases across
  26 suites, 935 template cases, and 113 operator cases.
- This is a progress checkpoint, not a C++17 conformance claim. Historical raw
  logs are disposable; `Tests/cpp17-coverage.json` records the durable scope.

## Next wave

1. Work down `Tests/features/Cpp17Gaps` from the runtime miscompiles outward:
   inline variable initialization, deque `front()`/`back()` returned directly,
   `stringstream` extraction, then the missing core-language and library
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
