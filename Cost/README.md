# Cost area

Everything about compile speed lives here: the compile-cost cases
(`tests/compile/`), the retained baselines (`baseline/`), this area's worker
(`worker.cmd` with `task.md`) and its generated output (`build/`).

```
src\scripts\performance.exe                          serial CPC-only speed assessment
src\scripts\performance.exe -Fast                    omit the heavy self-driver case
src\scripts\performance.exe -Iterations 5 -RawSamples Cost\build\raw.tsv
src\scripts\build.exe                                full self-host, validates, publishes
```

The harness compares the shared C cases against vendored TCC when it is
present; the full CPC self-driver case has no TCC measurement. TCC and other
external compiler runs need explicit authorization. Normal runs never extract a
reference compiler or update baselines.

Native helper implementation lives in `src/tools/`, workflow entry points live
in `src/scripts/`, and shared compile inputs live in `tests/compile/`. Generated
tools, logs and results stay in `build/`; retained baseline data lives in
`baseline/`.

The existing drift gate can pass while CPC remains slower than TCC. Use per-case
results and the matched shared-C aggregate; the CPC-only self-driver never enters
that ratio. Raw rows must be read with a dispersion tool built from
`src/tools/perf_dispersion.c` (for example `Cost\build\perf-dispersion.exe`)
before claiming a speed result.

The compiler's own source-quality checks (`src/tools/perf_check.c`, built as
`build\perf-check.exe` when wanted) compare leftovers, environment lookups and
loader lookups against `baseline/checks.tsv`: counts may only fall.
