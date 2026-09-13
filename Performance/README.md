# Performance measurements

The remaining measurement repairs, audit numbers and speed acceptance criteria
are in [task.md](task.md). The existing harness compares shared C cases against
vendored TCC; the full CPC self-driver case has no TCC measurement.

Use `scripts\performance.exe` for the current serial CPC-only speed
assessment, or add `-Fast` to omit the self-driver. Root CPC builds the native
helpers. TCC measurements require the task's explicit authorization; other
external compiler/ABI gates also remain blocked. Normal runs never extract a
reference compiler or update baselines.

Native helper implementation lives in `src/tools/`, workflow entry points live
in `scripts/`, and shared compile inputs live in `Tests/benchmarks/compile/`.
Generated tools, logs, and results stay in `build/perf/`; baseline and progress
data live here.

The existing drift gate can pass while CPC remains slower than TCC. Use per-case
results and the matched shared-C aggregate; the CPC-only self-driver never enters
that ratio. Raw rows must be read with `build\perf\perf-dispersion.exe` before
claiming a speed result.
