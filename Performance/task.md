# Remaining work: C compilation speed

Goal: lower the wall time cpc takes to compile C, measured by the suite in this
directory, without losing correctness anywhere else in the tree.

## Measure

- `Performance\PerformanceTests.cmd -Fast` runs the fast tier; leaving `-Fast`
  out adds `c.self.driver`, the heavy case that compiles
  `src/compiler/driver/cprime.c` with both cpc and the reference build.
- Portable numbers are the ratios `cpc/tcc` and `cpc/reference`, not
  milliseconds. tcc and the reference build absorb how fast the machine is, so
  the ratio from one run still means something in the next. `cpc/reference`
  answers "has today's compiler lost speed against the published one?" by
  construction, because the reference is HEAD's `cpc.exe`, extracted at run
  time and never committed twice.
- The gate in `Performance\baseline\perf-baseline.tsv` allows 25% drift
  (`-Tolerance`). Record a new baseline only for a deliberate, reviewed change.
- The suite exits 3, and logs nothing, when the machine is too busy to measure:
  a compiler that needs ten times as long for an empty file is a machine under
  load, not a slower compiler. `-NoiseLimit` moves that threshold.
- Measure with no other worker on the tree. A build running beside the suite
  inflates cpc's absolute milliseconds several times over; the ratios survive,
  the gate does not.
- Records land in `build\perf\perf-results.tsv`; worker cycles also append to
  `Performance\progress\log.tsv`.

## Automatic checks

`Performance\src\perf_check.c` counts what a fast compiler should not accumulate.
Counts may fall freely; a rise fails the suite. The baselines are in
`Performance\baseline\checks.tsv`.

- `env.crt` (`getenv(`), `env.win` (`GetEnvironmentVariable`), `env.expand`,
  `env.module`, `env.registry`: environment, loader and registry lookups. Every
  `getenv` call scans the whole environment block, so one on a hot path is a
  real cost. Resolve each once into a static and reuse it.
- `marker.todo`, `marker.fixme`, `marker.hack`, `marker.xxx`, `dead.if0`:
  unfinished work, workarounds and disabled code left in the tree.
- `stray.files`: `scratch`, `probe`, `tmp`, `.orig`, `.rej`, `.bak` files under
  `src/` or `include/`.

## Constraints

- Compile serially: one compiler process at a time, no threads in the driver.
- Root `cpc.exe` is the only host. Do not reach for Clang, MSVC or GCC.
- Keep the tree green: `powershell -NoProfile -ExecutionPolicy Bypass -File
  Tests\run-all.ps1 -Tier fast` must pass, and the compiler must still build
  itself (`Build.cmd`).
- Revert experiments that do not move the ratios. Do not weaken a case, a
  check or a baseline to make a change look good.
- Keep tools and cases first-party C built by cpc; no new PowerShell.

## Leads

- `c.empty.main` is pure per-invocation cost. cpc is roughly 1.5x tcc's time on
  the fast tier, and much of that gap is fixed cost paid before the first token:
  process start, image layout, driver setup and one-time state initialisation.
  Start there, then re-measure.
- Environment lookups already sit on the frontend's hot paths. A cached flag
  read once at startup replaces a scan per call.
- The heavy self-compile is dominated by preprocessing and code generation over
  a large translation unit, so it is the row that reacts to parser and
  code-generator work, while the small cases react to startup work. Improve both
  before claiming a win.

## Decisions

- Case metadata lives in the leading comments of each case file:
  `PERF_NAME`, `PERF_ARGS`, `PERF_SOURCE`, `PERF_TCC`, `PERF_TIER`,
  `PERF_ITERATIONS`, `PERF_WARMUPS`.
- The reference compiler is HEAD's `cpc.exe`, so no second compiler copy is
  committed and every run compares against the published build.
- Timing spawns the compiler directly (no shell) with its output redirected to
  `build\perf\logs`, and the child keeps the parent console: a detached console
  costs about as much as compiling these cases, which would swamp the numbers.
