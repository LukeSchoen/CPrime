# C++17 verification remaining

The machine-readable scope and source mapping is in `cpp17-coverage.json`.
Historical per-case observations are in `cpp17-verification.json`. These are
not a claim of complete C++17 conformance or of running entire upstream suites.
Raw compiler/build logs and downloaded reference inputs are under
`build/cpp17-*` and the failure directories named in those logs.

1. Obtain exclusive use of this checkout and root `cpc.exe`. Another session
   committed changes and replaced the compiler during the audit. Run
   `scripts/build.exe`, record compiler/source identities before and after each
   run, and repeat all retained cases on that stable build. Do not treat the
   historical results as certification of the current executable.
2. Fix valid tests for array structured bindings, selection expression
   initializers, and optional contained-object lifetime. These stay active in
   the existing feature suites; do not convert them into expected failures.
   The unpublished selection experiment was reverted after its publication
   gate failed; its diff is retained in `build/cpp17-selection-unpublished.patch`.
3. Investigate the four isolated template failures listed in the verification
   log. The full template batch terminated at
   `test_auto_return_repeated_forwarding_chain.cpp` with stack overflow. The
   following cases were subsequently run in groups of 20; missing batch results
   must never be mistaken for independently reproduced language failures.
4. Investigate the other baseline failures in Constructors, Expressions,
   InlineLifecycle, and OperatorOverloads, including possible contamination
   after failed compilations.
5. Extend fold coverage to constexpr evaluation, non-type and multiple packs,
   nested folds, overloaded operators, binary member-pointer forms, invalid
   cast-expression grammar, and the feature-test macro. A runtime scalar
   operator matrix cannot certify those distinct behaviors.
6. Audit remaining C++17 language/library facilities listed in the coverage
   manifest. MSVC's public STL sources are library tests, not its private
   frontend suite. The Turbo distribution supplies historical examples, not
   C++17 conformance tests. Its DOS APIs and obsolete source syntax are outside
   this target.

Run retained cases using the existing native runner, for example:

```
Tests/test.exe -Suite features/Templates -Select test_cpp17_fold_association.cpp test_cpp17_fold_operators.cpp test_cpp17_fold_member_pointer.cpp test_cpp17_fold_short_circuit.cpp test_cpp17_fold_empty_add.cpp test_cpp17_fold_mismatched_operators.cpp test_cpp17_fold_both_operands_pack.cpp
Tests/test.exe -Suite features/Statements -Select test_cpp17_selection_init.cpp test_cpp17_selection_init_scope.cpp
Tests/test.exe -Suite features/Declarations -Select test_cpp17_structured_binding_array.cpp test_cpp17_structured_binding_count.cpp
Tests/test.exe -Suite features/Includes -Select test_cpp17_optional_lifetime.cpp
```

The new sources are automatically discovered by the unified feature suites.
No external compiler or separate imported test suite is needed to run them.
