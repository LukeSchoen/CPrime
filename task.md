# Remaining work

Target: 100% of fast tests and the retained pedantic GCC checks, with no missing
coverage, weakened expectations, compiler internal errors, or timeout retries.

Current retained state: 426 unresolved retained GCC rows (398 FAIL_COMPILE,
17 FAIL_RUN, 11 FAIL_RUN_CRASH) on compiler SHA256
87fc1fb6bd540d822a51c37111288011f4d2566680d20cff12d0e1998d800ff7. The exact
result file, compiler identity, and per-case diagnostics are under the newest
`build/pedantic-gcc-*/` directory; the compact cluster report is
`build/compiler-bug-triage.txt` (with machine-readable paths in
`build/compiler-bug-triage.json`).

This work is time-boxed to 30 minutes total; solve as much as possible within
that time and then stop, do not keep looping past the budget. Work without
stopping after individual repairs or asking for approval: regenerate the
triage with `Tests/triage_retained_failures.ps1`, choose a cluster that shares
one first diagnostic in one source area, fix it, verify it, promote verified
passes to pedantic in `Tests/tiers.json`, then continue immediately with the
next cluster until the 30-minute budget is spent. At the 30-minute mark, stop
regardless of remaining clusters and report what was completed, what remains,
and any blocker encountered. If before the budget is spent you become convinced
that all fast tests and retained pedantic checks are passing, create an empty
`done.x` file in the repository root and stop; `worker.cmd` watches for that
marker and will not start another cycle.

Full inventories stay in the generated result/triage files under `build/`; do
not paste them into task notes or chat. Verified passes are assigned to
pedantic in `Tests/tiers.json`; `Tests/run-all.ps1` then focuses on the
unresolved set. Keep all failing cases active, including runtime crashes and
wrong results.

Prefer clusters with two to five tests sharing one diagnostic and source area
over broad feature headers (for example coroutines or `_Complex`) until each
cluster is reduced to a small local regression.

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
- Complete placement-delete unwinding for variadic allocation functions and
  audit class-valued placement arguments for copy and lifetime semantics.
- Complete template deduction, substitution failure, specialization, dependent
  lookup, and declaration-only member calls in the remaining retained cases;
  retain the local members/replay/lookup/substitution gates.
- Complete overloaded function selection and hidden-name lookup in the
  retained GCC cases.
  Include callable conversion candidates with differing parameter lists and
  competition between surrogate function calls and member call operators.
- Complete constexpr evaluation of local object values, assignments, and
  control flow; preserve scoped bindings, side effects, and constant-expression
  rejection checks while extending parameterized-function evaluation.
- Implement coroutine language/runtime support and `<coroutine>`; complete
  missing standard-library facilities exercised through `<tuple>`, `<optional>`,
  and `<bitset>`.
- Complete GNU compatibility exercised by the suite: statement expressions,
  inline assembly/asm-goto constraints, vector operations, `_Complex`, builtins,
  and attributes. Verify target and ABI applicability before implementation.
  Complete canonical Unicode identifier identity across UTF-8 and equivalent
  universal-character-name spellings, preserving preprocessing spelling.
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
