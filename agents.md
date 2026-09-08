# Agent Instructions

- Run Cprime compilation and project builds serially: one thread and one compiler
  process at a time. Improve algorithms and data handling, not concurrency.
  Compiled programs may still use threads. MSVC benchmarks may run in parallel;
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
- Keep implementation in src/, headers in include/, tests and benchmark inputs
  in Tests/, build scripts in scripts/, and generated output in build/.
  Preserve the root cpc.exe and lib/ bootstrap/portable runtime inputs.
- Use only C, C++, assembly, batch (.bat/.cmd), and PowerShell (.ps1) for
  first-party code, including test tools. Do not embed C# or require Python.
  Leave third-party sources/tooling unchanged; documentation and data are exempt.
- Test exact reproducers and curated subsystem gates first (Tests/DEVELOPMENT.md).
  Use fast tests for normal checkpoints and runner summaries for results.
- Avoid pedantic sweeps except at the end of large changes that warrant deep
  testing. Exact retained reproducers and focused runner unit tests are allowed
  during repairs. Keep fast standalone gates under five seconds; move genuinely
  heavier workloads to the explicit pedantic tier, never silently skip them.
- Keep Markdown limited to usage, current requirements, and remaining work.
  Keep decisions and completed-work history in git; preserve regression coverage
  in tests and clean old build logs routinely.
