# Agent Instructions

- Use the repository's `cpc.exe` for compilation, builds, and regression work.
  Do not invoke Clang, GCC, MSVC, or another compiler, including host rebuilds,
  reference comparisons, and benchmarks, unless the user explicitly requests it.
  A CPC failure is a bug to investigate, not permission to switch compilers.
  Plain `Build.cmd` self-hosts with root `cpc.exe` and publishes back to that path.
  Keep one active compiler: root `cpc.exe`. Do not introduce host-selection modes
  or alternate working compiler copies. Staging belongs inside the build only;
  a failed build preserves root `cpc.exe` and stops without changing hosts.
  Clang builds must have Clang explicitly in their entry-point name and still
  require authorization. for external projects Verify they use fresh validated CPC.
  This also applies to test wrappers: `tests.cmd`/`-IncludeChecks` include
  cross-compiler ABI gates. Use `Tests/run-all.ps1` and selected CPC-only gates
  unless those external compiler invocations are explicitly authorized.
- Run Cprime compilation and project builds serially: one thread and one compiler
  process at a time. Improve algorithms and data handling, not concurrency.
  Compiled programs may still use threads. MSVC benchmarks may run in parallel.
- Reproduce reported bugs, add regression coverage, implement a coherent fix,
  and run focused relevant tests only.
- Revert failed experiments. Do not retain name-specific hacks or turn active
  reproducers into expected failures to disguise incomplete work.
- Keep all tests in this CPrime repository. Reduce external bug reports to standalone local tests.
  Keep tests minimal, deterministic, and fast; reuse helpers and remove redundant
  fixtures without losing distinct behavior coverage.
- Keep implementation in src/, headers in include/, tests and benchmark inputs
  in Tests/, build scripts in scripts/, and generated output in build/.
- Use only C, C++, assembly, batch (.bat/.cmd) first-party code, avoid C# Python or ps1.
  Leave third-party sources/tooling unchanged; documentation and data are exempt.
- Prefer C sources (.c) compiled with root `cpc.exe` into native .exe tools for
  build drivers, test helpers, and similar automation. Their lower startup
  overhead makes repeated invocations faster, Avoid new PowerShell (.ps1) implementations by default.
  Consider PowerShell only if a C/native executable approach is impractical or still
  performs poorly after optimization attempts; measure the alternatives on the
  same workload. Keep tool implementation in src/, test-specific sources in
  Tests/, and generated executables in build/.
- Avoid pedantic sweeps except at the end of very large changes that warrant deep
  testing. Exact retained reproducers and focused runner unit tests are allowed
  during repairs. Keep fast standalone gates under a few seconds; move genuinely
  heavier workloads to the explicit pedantic tier.
- Keep Markdown limited to remaining work, decisions and completed-work ARE code.

reference tests, or benchmarks without explicit user authorization.
Clang builds are available only through explicitly Clang-named entry points.
Investigate CPC failures using CPC; do not change the default host.

Builds regenerate the runtime, package the candidate, and run the regression
gate before replacing root `cpc.exe`. Preserve that executable and `lib/` for
bootstrapping. Generated output belongs in `build/`.

For solution builds, export a manifest with
`scripts/windows/export-build-manifest.ps1 -ProjectRoot <root> -SolutionPath <solution>`,
then use `BuildProject.cmd -ProjectRoot <root>` or `build/build_project.exe`
directly. The serial CPC driver is native C; normal project builds launch no
PowerShell. Rebuild its executables after tool-source changes with
`scripts/windows/build-project-tools.cmd`. Paths are configurable; CPC remains
the compiler. Build metrics and dependency caches live in the output directory.
Use `-Rebuild` to compile every unit again. `-Unity -UnityBatchSize 24` groups
up to 24 compatible C++ sources per unit; the default batch size is 32.
Keep file-local name conflicts in separate manifest `unityGroup` values.
Metrics include elapsed and CPU seconds for each tool process.

## Test and develop

```bat
powershell -NoProfile -ExecutionPolicy Bypass -File Tests/run-all.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File Tests/run.ps1 -Suite features/Templates
```

See [test commands and layout](Tests/README.md) and the
[development loop](Tests/DEVELOPMENT.md). Bug reports should include a standalone
reproducer, the command, and expected versus actual behavior.

The retained GCC corpus under `Tests/pedantic/gcc/` holds only unresolved rows:
a repaired row becomes a first-party regression and is then deleted from the
corpus. See [remaining work](task.md) for the current state and ordered work.

`tests.cmd` includes cross-compiler ABI gates and requires explicit authorization
for their external compiler invocations. Use `tests_pedantic.cmd` only
after a large change that warrants deeper language, packaging, or self-host
testing.

Implementation lives in `src/`, headers in `include/`, test and benchmark inputs
in `Tests/`, and build scripts in `scripts/windows/`.
