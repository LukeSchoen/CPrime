# Performance suite

Compares cpc against tcc, and against the published compiler from HEAD, on the C
cases in `cases/`. The rules and the measurement record are in `task.md`; the
commands are:

```bat
Performance\PerformanceTests.cmd            rem full suite, gated
Performance\PerformanceTests.cmd -Fast      rem skip the heavy self-compile case
Performance\PerformanceTests.cmd -NoGate    rem report only
Performance\PerformanceTests.cmd -UpdateBaseline
Performance\PerformanceWorker.cmd           rem measure, run codex, measure, loop
```

Layout:

- `cases/` - one C translation unit per case, metadata in the leading comments.
- `src/` - `perf_compare.c` (timing and ratios) and `perf_check.c` (leftover and
  lookup checks). Both are first-party C, built by root `cpc.exe` into
  `build/perf/`.
- `baseline/` - `perf-baseline.tsv` (ratios) and `checks.tsv` (counts). A run
  fails when it is more than `-Tolerance` percent slower, or when a check count
  rises.
- `progress/log.tsv` - one row per worker cycle: the measured ratios and the
  check totals.

Exit codes: 0 clean, 1 regression or new finding, 3 the machine was too busy to
measure (nothing was recorded).

Generated output (executables, compiler logs, results TSV) belongs in
`build/perf/`.
