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
- For Racer compatibility work, rebuild CPC, copy the verified compiler to the
  measurement project, and exercise the real compile/link build repeatedly.
  Before declaring it ready, verify the executable exists, report its exact
  path, and disclose runtime-affecting warnings or limitations.
- Keep implementation in src/, headers in include/, tests and benchmark inputs
  in Tests/, build scripts in scripts/, and generated output in build/.
  Preserve the root cpc.exe and lib/ bootstrap/portable runtime inputs.
