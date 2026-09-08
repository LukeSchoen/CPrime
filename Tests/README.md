# Tests

Run commands from the repository root in PowerShell (Windows PowerShell 5.1
is sufficient). Compilation is serial.

```powershell
./Tests/run.ps1 -Suite features/Templates -Select test_name.cpp
./Tests/gates.ps1 -Subsystem members
./Tests/run-all.ps1
```

`tests.cmd` runs all discovered language suites plus assembly-output and
multiple-source integration checks. `run-all.ps1 -List` lists language suites;
`-Suite c_compat,features/Expressions` selects a subset. Failures are collected
across suites and produce a nonzero exit. Standalone tools below run separately.
Use `-CompilerPath` and, for unpackaged hosts, `-RuntimeRoot` with language runners.
`run.cmd` forwards to `run.ps1`.

## Layout

| Path | Purpose |
| --- | --- |
| `c_compat/`, `features/`, `debug/`, `payload/pass/` | Discovered language suites |
| `integration/multi_source/` | Multiple-source fixtures run by `test_MultiSource.cmd` |
| `abi/`, `native_runtime/`, `runtime/`, `include/`, `payload/` | Native, runtime, header, and packaging fixtures |
| `gcc/` | Pinned upstream assessment adapter and its tests |
| `benchmarks/compile/`, `benchmarks/runtime/` | BuildProfile and codeProfile inputs |
| `tools/` | Test support programs |
| `check_*.ps1`, `test_*.ps1`, `test_*.cmd` | Standalone subsystem/integration checks |

Generated files belong in `build/` or an isolated temporary directory. Keep
helpers beside their owning tests; give helpers names without `test_`.

## Test format

Suites contain `pass/` and/or `fail/` directories with `test_*.c` and `test_*.cpp`.
Each test compiles, links, and runs by default. Put metadata in its first 12 lines:

| Metadata (`// NAME: value`) | Meaning |
| --- | --- |
| `EXPECT_EXIT` | Required exit status; default `0` |
| `EXPECT_STDOUT` | Exact stdout when supplied |
| `EXPECT_COMPILE_FAIL: 1` | Require a compiler diagnostic failure; crashes do not pass |
| `EXPECT_COMPILE_ARGS` | Additional compiler arguments |
| `EXPECT_COMPILE_ONLY: 1` | Verify object generation without linking/running |
| `EXPECT_SOURCES` | JSON array of extra source paths relative to the test |
| `EXPECT_MANIFEST_SOURCE` | Translation unit selected from a build manifest |

`-BuildManifestPath` or `CPRIME_TEST_BUILD_MANIFEST` supplies a schema-version-2
manifest. Selected units inherit include paths, defines, undefines, forced includes,
and source overrides. Missing or ambiguous fixtures fail setup. Compiler arguments
use response files. Compile-only, multiple-source, and manifest tests compile fresh.
`-UseSharedBinaries` optionally reuses outputs from `test_batch.cmd`; use fresh
compilation for verification of compiler changes.

## Subsystem checks

Run these with `powershell -NoProfile -ExecutionPolicy Bypass -File Tests/<script>`.
Check each script's parameter block for compiler/runtime or native toolchain paths.

| Area | Scripts |
| --- | --- |
| Build driver | `check_build_default.ps1`, `check_build_regression_gate.ps1`, `check_build_manifest.ps1`, `check_batch_build.ps1`, `check_incremental_build.ps1` |
| Compiler/self-host | `check_regressions.ps1`, `check_clang_selfhost.ps1`, `check_fast_codegen.ps1`, `test_ParserDiagnostics.ps1` |
| Runner/policy | `test_RunnerFixtures.ps1`, `test_SuiteRunner.ps1`, `check_source_languages.ps1` |
| Headers/packaging | `test_include_search.ps1`, `test_PortablePackaging.ps1`, `check_pack_cache.ps1` |
| Runtime | `check_ucrt_runtime.ps1`, `check_ucrt_module_exit.ps1`, `test_NativeTls.ps1`, `test_RunExceptions.ps1` |
| Native ABI | `test_MsvcClassLayout.ps1`, `test_MsvcMemberLinkage.ps1`, `test_MsvcNullptr.ps1`, `test_MsvcRecordReturn.ps1`, `test_MsvcStackProbe.ps1` |
| Linker | `test_CoffLabels.ps1`, `test_CoffWeakExternals.ps1`, `test_LinkerMap.ps1`, `test_PragmaLibraries.ps1`, `test_DynamicCastObjectLink.ps1`, `test_MemberPointerObjectLink.ps1` |

Native checks require installed Clang/MSVC as specified by their parameters.
Portable packaging checks use fresh extraction and verify SDK/runtime contents,
C/header probes, DLL/COM calls, and stack-probe ABI. `-FullIncludes` expands header
coverage; `-MaxBytes 1000000` enforces the portable size target. Use
`-RuntimeLibPath` for a package built with another runtime archive.

See the [development loop](DEVELOPMENT.md), [remaining work](../task.md), and
[GCC assessment](gcc/README.md).
