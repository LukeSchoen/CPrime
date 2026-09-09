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
