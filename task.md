# Remaining work

Target: 100% of local tests and all applicable GCC checks, with no missing
coverage, weakened expectations, compiler internal errors, or timeout retries.

## Compiler and runtime

- Correct runtime behavior first: virtual inheritance and covariant return
  adjustment (`g++.dg/abi/covariant4.C`), temporary/aggregate cleanup during
  exceptions (`g++.dg/eh/aggregate1.C`), and the local chrono access violation.
- Repair internal value-stack errors in `g++.dg/ext/stmtexpr7.C`,
  `g++.old-deja/g++.martin/conv1.C`, and `g++.old-deja/g++.pt/cond2.C`.
- Complete template deduction, substitution failure, specialization, dependent
  lookup, and declaration-only member calls. Start with
  `g++.dg/template/access27.C`, `sfinae24.C`, and `partial-specialization4.C`;
  retain the local members/replay/lookup/substitution gates.
- Complete class conditional conversions (`g++.dg/overload/cond2.C`), overloaded
  function selection, hidden-name lookup, and reference `dynamic_cast`
  (`g++.dg/rtti/dyncast2.C`).
- Implement coroutine language/runtime support and `<coroutine>`; complete
  missing standard-library facilities exercised through `<tuple>`, `<optional>`,
  and `<bitset>`.
- Complete GNU compatibility exercised by the suite: statement expressions,
  inline assembly/asm-goto constraints, vector operations, `_Complex`, builtins,
  and attributes. Verify target and ABI applicability before implementation.
- Resolve local `std::function` callable/capture/conversion failures, async/future
  forwarding, constructor function-try-block handling, and shell data layout.
- Repair assembly output rejected by `Tests/test_AsmOutput.cmd` for member
  initializers. Keep object/link/native ABI gates alongside language coverage.

## Test coverage and usability

- Fetch transitive GCC fixture dependencies under `gcc.dg/`; five checked
  inputs reference files outside the current sparse checkout.
- Extend the GCC adapter with tested diagnostic matching, standard/target
  selection, extra-source and specialized-driver support, and output/assembly
  expectations. Keep unsupported cases visible until their results can be judged.
- Audit legacy HeapList/Perf fixtures for missing declarations and private
  class access before assigning failures to CPC; retain their intended runtime
  coverage and supply standalone, valid reproducers.
- Apply the five-second compiler/process ceiling to standalone test tools.
- Reach a portable package size of 1,000,000 bytes with the complete SDK/runtime.
  Use `scripts/windows/build-cpc-small.ps1 -Test` and enforce the target with
  `Tests/test_PortablePackaging.ps1 -CompilerPath build/compiler-small/cpc.exe -MaxBytes 1000000`.

Reproduce against a fresh build and follow the [development loop](Tests/DEVELOPMENT.md).
