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
  require authorization. Verify external projects use the freshly validated CPC.
  This also applies to test wrappers: `tests.cmd`/`-IncludeChecks` include
  cross-compiler ABI gates. Use `Tests/run-all.ps1` and selected CPC-only gates
  unless those external compiler invocations are explicitly authorized.
- Run Cprime compilation and project builds serially: one thread and one compiler
  process at a time. Improve algorithms and data handling, not concurrency.
  Compiled programs may still use threads. Explicitly requested MSVC benchmarks may run in parallel;
  use identical inputs, record settings/concurrency, and separate compiler time
  from build-driver overhead.
- Reproduce reported bugs, add regression coverage, implement a coherent fix,
  and run focused and relevant surrounding tests. Continue while useful progress
  is possible; stop only when complete or blocked on necessary external input.
- Revert failed experiments. Do not retain name-specific hacks or turn active
  reproducers into expected failures to disguise incomplete work. Keep changes
  buildable and reviewable as meaningful commit-sized units.
- Keep CPC a general-purpose compiler. Tests and build tools must not depend
  on sibling application repositories or application-specific libraries.
- Keep CPC project builds native: use `BuildProject.cmd` or
  `build/build_project.exe`, rebuilt with `scripts/windows/build-project-tools.cmd`.
  Do not add PowerShell to that build path. PowerShell test harnesses may launch
  the native tools; keep their fixtures and performance gates in `Tests/`.
- Keep all tests, regression reproducers, and their required fixtures in this
  CPrime repository. Reduce external bug reports to standalone local cases.
  Keep tests minimal, deterministic, and fast; reuse helpers and remove redundant
  fixtures without losing distinct behavior coverage. Apply this to both tiers.
- Keep regression reports concise and reproducible: repository-relative test
  path, exact command, compiler/runtime identity, expected versus actual result,
  and relevant runner summary/timing. Store generated reports and logs in build/;
  keep only unresolved work in Markdown and completed history in git.
- Keep implementation in src/, headers in include/, tests and benchmark inputs
  in Tests/, build scripts in scripts/, and generated output in build/.
  Preserve the root cpc.exe and lib/ bootstrap/portable runtime inputs.
- Use only C, C++, assembly, batch (.bat/.cmd), and PowerShell (.ps1) for
  first-party code, including test tools. Do not embed C# or require Python.
  Leave third-party sources/tooling unchanged; documentation and data are exempt.
- Prefer C sources (.c) compiled with root `cpc.exe` into native .exe tools for
  build drivers, test helpers, and similar automation. Their lower startup
  overhead makes repeated invocations faster, and they are easy to edit and
  recompile. Avoid new PowerShell (.ps1) implementations by default. Consider
  trying PowerShell when a C/native executable approach is impractical or still
  performs poorly after optimization attempts; measure the alternatives on the
  same workload. Keep tool implementation in src/, test-specific sources in
  Tests/, and generated executables in build/.
- Test exact reproducers and curated subsystem gates first (Tests/DEVELOPMENT.md).
  Use fast tests for normal checkpoints and runner summaries for results.
- Avoid pedantic sweeps except at the end of large changes that warrant deep
  testing. Exact retained reproducers and focused runner unit tests are allowed
  during repairs. Keep fast standalone gates under five seconds; move genuinely
  heavier workloads to the explicit pedantic tier, never silently skip them.
- Keep Markdown limited to usage, current requirements, and remaining work.
  Keep decisions and completed-work history in git; preserve regression coverage
  in tests and clean old build logs routinely.
