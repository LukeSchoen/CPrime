# Tests

Use `Tests/run-all.ps1` for CPC-only language testing plus selected CPC-only
native gates. Use exact tests and subsystem gates first:

```powershell
./Tests/run.ps1 -Suite features/Templates -Select test_name.cpp
./Tests/gates.ps1 -Subsystem members
./Tests/run-all.ps1 -List
./Tests/run-checks.ps1 -List
```

Use root `cpc.exe`. Compiler-path overrides are for build-internal validation
only; do not keep alternate working compiler copies. Other compilers, including
reference comparisons and host rebuilds, need explicit user authorization.
Windows PowerShell may require
`powershell -NoProfile -ExecutionPolicy Bypass -File <script>`.

## Tiers

- **Fast:** unresolved retained GCC rows, plus new first-party regressions and
  complete standalone gates. Individual language compile/run steps and fast
  gates have a five-second ceiling. `Tests/run-checks.ps1` records gate timings
  and logs in `build/`, kills timed-out process trees and continues.
- **Pedantic:** the established first-party pass corpus plus packaging,
  build-driver, self-host and performance workloads. Run it after large changes
  or consolidation, not after routine edits.

`Tests/checks.json` assigns standalone gates and budgets. A fast gate exceeding
its budget fails; investigate and move it explicitly if its workload belongs in
pedantic testing, or keep it and fix the work. Never silently skip it.
`Tests/run-all.ps1` runs language suites alone; `-IncludeChecks` adds fast gates.
`tests.cmd` and `-IncludeChecks` include cross-compiler ABI gates and need
explicit authorization for those invocations.

## Layout

| Path | Purpose |
| --- | --- |
| `c_compat/`, `features/`, `debug/`, `payload/pass/` | Fast language suites |
| `integration/multi_source/`, `abi/`, `native_runtime/`, `runtime/` | Integration/native fixtures |
| `pedantic/gcc/` | Retained unresolved GCC failure corpus and assessment tools |
| `pedantic/performance/`, `benchmarks/` | Stress and profiling workloads |
| `progress/` | Machine-readable worker progress log and outstanding first-party list |
| `tools/` | Shared test helpers |
| `checks.json` | Fast/pedantic standalone gate catalog |

Keep all tests and required fixtures self-contained in CPrime, minimal and
deterministic. Reuse helpers and preserve distinct coverage when removing
duplicates. Generated output belongs under `build/` or an isolated temporary
directory; helpers live beside their tests and do not use the `test_` prefix.
See the [development loop](DEVELOPMENT.md) and
[remaining work](../task.md).

## Test format

Suites contain `pass/` and/or `fail/` directories with `test_*.c` and
`test_*.cpp`. Tests compile, link and run unless metadata in their first 12
lines says otherwise:

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
fail setup. All tests compile fresh, with arguments passed through response
files.

Retained GCC rows are repaired, not promoted: after a fix lands, its first-party
regression stays and the external row is deleted. See
[retained GCC checks](pedantic/gcc/README.md); the ordered wave plan and row
buckets live in [remaining work](../task.md).
