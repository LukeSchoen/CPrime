# Tests

Use `Tests/run-all.ps1` for normal CPC-only language testing, plus selected
CPC-only native gates. `tests.cmd` adds all fast native checks, including ABI
gates that invoke Clang; agents need explicit authorization before running those.
Use exact tests and subsystem gates first:

```powershell
./Tests/run.ps1 -Suite features/Templates -Select test_name.cpp
./Tests/gates.ps1 -Subsystem members
./Tests/run-all.ps1 -List
./Tests/run-checks.ps1 -List
```

Use root `cpc.exe`. Compiler-path overrides are for build-internal validation;
do not maintain alternate working compiler copies. Other compilers, including
reference comparisons and host rebuilds, require explicit user authorization.
Windows PowerShell may require `powershell -NoProfile
-ExecutionPolicy Bypass -File <script>`.

## Fast versus pedantic

- **Fast:** individual language compile/run steps and complete standalone gates
  have a five-second ceiling. `run-checks.ps1` records gate timings and logs in
  `build/`, kills timed-out process trees, and continues to report other failures.
- **Pedantic:** `tests_pedantic.cmd` explicitly runs the retained GCC failure
  corpus, packaging/build/self-host checks, and performance workloads. Use it
  only at the end of a large change that warrants deep verification. Do not run
  it after routine edits or automatically before every merge.
- `Tests/checks.json` assigns standalone gates and budgets. Pedantic gate totals
  may take up to 120 seconds; local/GCC compiler and program invocations retain
  five seconds. A fast gate exceeding its budget fails; investigate and move it
  explicitly if its workload belongs in pedantic testing. Never silently skip it.

Native ABI/runtime/linker and multiple-source checks are in the fast tier.
Speed classification does not authorize another compiler.
`run-all.ps1` runs language suites alone; `-IncludeChecks` adds fast gates.
`run-checks.ps1 -Select test_NativeTls` runs one fast gate.
For a justified deep check, select `tests_pedantic.cmd -Group gcc|checks|performance`.
No normal entry point discovers `Tests/pedantic/`.

## Layout

| Path | Purpose |
| --- | --- |
| `c_compat/`, `features/`, `debug/`, `payload/pass/` | Fast language suites |
| `integration/multi_source/`, `abi/`, `native_runtime/`, `runtime/` | Integration/native fixtures |
| `pedantic/gcc/` | Retained upstream failure corpus and assessment tools |
| `pedantic/performance/`, `benchmarks/` | Stress and profiling workloads |
| `tools/` | Shared test helpers |
| `checks.json` | Fast/pedantic standalone gate catalog |

Generated output belongs under `build/` or an isolated temporary directory.
Helpers live beside their tests and do not use the `test_` prefix.
Keep all tests and required fixtures self-contained in CPrime, minimal and
deterministic. Reuse helpers and preserve distinct coverage when removing
duplicates. Keep regression reports concise and reproducible as described in the
[development loop](DEVELOPMENT.md); generated reports belong in `build/`.

## Test format

Suites contain `pass/` and/or `fail/` directories with `test_*.c` and `test_*.cpp`.
Tests compile, link, and run unless metadata in their first 12 lines says otherwise:

| Metadata (`// NAME: value`) | Meaning |
| --- | --- |
| `EXPECT_EXIT` | Required exit status; default `0` |
| `EXPECT_STDOUT` | Exact stdout when supplied |
| `EXPECT_COMPILE_FAIL: 1` | Require diagnostic rejection; crashes never pass |
| `EXPECT_COMPILE_ARGS` | Additional compiler arguments |
| `EXPECT_COMPILE_ONLY: 1` | Verify object generation without linking/running |
| `EXPECT_SOURCES` | JSON array of extra source paths relative to the test |
| `EXPECT_MANIFEST_SOURCE` | Translation unit selected from a build manifest |

`-BuildManifestPath` or `CPRIME_TEST_BUILD_MANIFEST` supplies a schema-version-2
manifest with per-source preprocessing settings. Missing or ambiguous fixtures
fail setup. All tests compile fresh, with arguments passed through response files.

See the [development loop](DEVELOPMENT.md), [remaining work](../task.md), and
[retained GCC checks](pedantic/gcc/README.md).
