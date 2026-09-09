# Remaining work

Target: 100% of fast tests and the retained pedantic GCC checks, with no missing
coverage, weakened expectations, compiler internal errors, or timeout retries.

Use build/task-validation.txt for the current compiler/runtime identity, commands,
results, and remaining failures. The retained inventory is recorded in
build/task-gcc-verified/results.jsonl; keep all failing cases active.

## Compiler and runtime

- Bring the uncached, serial OTServ Release build below 30 seconds. Measure the
  complete native wrapper with `-Unity -Configuration Release -Rebuild`, including
  preparation and linking; keep all 215 sources and use root `cpc.exe`. The target
  remains unverified with the retained compiler changes. Continue profiling
  template deduction and constructor-conversion probes; record machine load and
  compiler identity alongside elapsed and CPU time in `build/`.
  The OTServ project path and native build entry point are still needed.

- Complete virtual-base construction/destruction semantics and the remaining
  retained GCC runtime failures. Select exact failures from the current inventory
  and retain small local regressions alongside each repair.
- Complete template deduction, substitution failure, specialization, dependent
  lookup, and declaration-only member calls in the remaining retained cases;
  retain the local members/replay/lookup/substitution gates.
- Complete overloaded function selection and hidden-name lookup in the
  retained GCC cases.
- Implement coroutine language/runtime support and `<coroutine>`; complete
  missing standard-library facilities exercised through `<tuple>`, `<optional>`,
  and `<bitset>`.
- Complete GNU compatibility exercised by the suite: statement expressions,
  inline assembly/asm-goto constraints, vector operations, `_Complex`, builtins,
  and attributes. Verify target and ABI applicability before implementation.
- Resolve the bundled Yasm GAS parser limitation in `Tests/test_AsmOutput.cmd`:
  it rejects quoted Microsoft C++ symbol names. Preserve exact symbol identity
  and all CPC/Yasm round-trip expectations; leave third-party tools unchanged.
  Keep object/link/native ABI gates alongside language coverage.

## Test coverage and usability

- Extend the GCC adapter with tested diagnostic matching, standard/target
  selection, extra-source and specialized-driver support, and output/assembly
  expectations. Keep unsupported cases visible until their results can be judged.
- Audit legacy HeapList/Perf fixtures for missing declarations and private
  class access before assigning failures to CPC; retain their intended runtime
  coverage and supply standalone, valid reproducers.
- Keep fast gates within their enforced five-second budget; deeper checks are
  explicit pedantic work after large changes.
- Reach a portable package size of 1,000,000 bytes with the complete SDK/runtime.
  Enforce the target with
  `Tests/test_PortablePackaging.ps1 -CompilerPath cpc.exe -MaxBytes 1000000`.

Reproduce against a fresh build and follow the [development loop](Tests/DEVELOPMENT.md).
