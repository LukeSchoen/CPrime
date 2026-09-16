# Cost: compile faster

Make root `cpc.exe` compile quicker, first for itself and then for C and C++
sources in general, without giving up correctness. Keep the whole tree
self-hosting: a speed change that cannot pass packaging and `-Regression` is
not a change. The queue never empties: when a package closes, take on the next
hardest case instead of declaring the job done, and never create `done.x`.

Work in this tree only. Delete nothing: `Cost\worker.cmd` and this file are the
user's control surface, and other worker folders may be in use on other days.

## What to make faster

1. The compiler building itself: `src\scripts\build.exe` (serial C-only self-host,
   cached package, validate, publish). Time the whole run. The per-cycle proxy
   is the retained `c.self.driver` case
   (`Cost/tests/compile/costs/self_driver.c`), which compiles the complete
   driver translation unit.
2. Compiling C and C++ translation units, hardest case first:
   - heavy headers: long include chains, repeated inclusion, large declarations,
     deep nesting, system header weight
   - heavy templates: many instantiations, deep recursion, variadic packs,
     nested member templates, SFINAE and overload probes
   - deep constant evaluation and conversion ranking
   - macro-heavy preprocessing and very long token streams
   - large translation units, many functions and symbols, deep call graphs
   - repeated declaration, member, owner and source lookups

## Measure, every cycle, before and after

```
src\scripts\performance.exe -Root . -CpcOnly -NoGate -Quiet -Results Cost\build\perf-cycle-NNNN.tsv
src\scripts\performance.exe -Root . -CpcOnly -NoGate -Iterations 5 -Warmups 1 -RawSamples Cost\build\raw.tsv
src\scripts\build.exe                                  full self-host, publish on success
```

The worker runs the first command every cycle (its rows also land in
`Cost\build\cycles.csv`), and `Compatibility\tests\test.exe -Regression` as the
gate.

- The harness reads case metadata (`PERF_NAME`, `PERF_TIER`, `PERF_ITERATIONS`,
  `PERF_ARGS`, `PERF_SOURCE`) from the leading comment lines of
  `Cost/tests/compile/*.c`, runs strictly one compiler at a time and
  reports medians.
- Read the spread with `src/tools/perf_dispersion.c` (build it with root
  `cpc.exe` into `Cost\build\perf-dispersion.exe`) before claiming a win. One
  sample is not a result; repeat a serial run on the same input, flags and
  output path and compare medians.
- Record with every number: the exact command, the input, the flags, the output
  directory and what the machine was doing. Keep raw evidence under
  `Cost\build`.
- `cpc -bench` and `CPC_PROFILE_SCANS` show where compilation time goes; use
  them to choose the next target instead of guessing.

## Rules

- Root `cpc.exe` only, one compiler process at a time. Clang, GCC, MSVC and TCC
  runs are unauthorized unless the user authorizes them for a measurement; when
  that happens they are references only, never a build path. Record unauthorized
  checks as blocked rather than working around them.
- The C++ compile cases (`Cost/tests/compile/*.cpp`, `competitive/`) are
  not wired into the harness yet: it scans only `*.c`. Bringing the hardest C++
  inputs into the measurement is fair game and is often the difference between
  measuring and guessing.
- After changing `src/tools/*.c`, rebuild the workflow executables with
  `src\scripts\tool-build.exe` (serial).
- Verify correctness with the exact case, then the affected suite, then
  `Compatibility\tests\test.exe -All -Tier fast` and `Compatibility\tests\test.exe -Regression`. Do not run
  the pedantic tier. Publish with `src\scripts\build.exe` only when packaging and
  the regression gate pass.
- Keep one focused change per cycle and revert experiments that do not hold up.
  No name-specific hacks, no disabling or relabelling cases, no retained scratch
  files.
- New compile-stress inputs are welcome when a hard aspect has no case, and are
  authorized by this worker's purpose: keep them deterministic, fast,
  self-contained and in `Cost/tests/compile`, with the case metadata in
  the leading comment.

## Leads

- `src/compiler/README.md` holds the current structural plan: phase
  measurements, remaining global scans in template and member lookup,
  declaration registration behind a small interface, explicit replay and
  function scratch storage, object/link isolation.
- Cost classes to attack with evidence: repeated environment and loader
  lookups, repeated path construction, token copying, linear scans over saved
  bodies or overload sets, per-token allocation, unbounded output writes.
- Batch and incremental paths (`--batch`, `-M` dependency runs, the tool chain
  in `src/scripts/`) compile many units with one compiler invocation: their state
  reset and cache validation are part of compile speed.
- `Cost\baseline\perf-baseline.tsv` records the retained numbers the
  harness gates against; keep the baseline meaningful when a case changes.
