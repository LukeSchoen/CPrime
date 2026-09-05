# Language test runner

Run a suite with `powershell -NoProfile -ExecutionPolicy Bypass -File Tests/run.ps1 -Suite features/Classes`.
The runner discovers `test_*.c` and `test_*.cpp` in the suite's `pass` and `fail` directories.
Compiler arguments use response files, including when a compiler wrapper is selected,
so large manifest settings and quoted paths are preserved without command-line limits.
`Tests/test_RunnerFixtures.ps1` validates these settings and fixture setup failures.
Each test compiles, links, and runs by default. A test's first 12 lines may contain
`// EXPECT_NAME: value` metadata:

- `EXPECT_EXIT`: required process exit status (default `0`).
- `EXPECT_STDOUT`: exact required standard output, when specified.
- `EXPECT_COMPILE_FAIL: 1`: compilation must reject an invalid program.
- `EXPECT_COMPILE_ARGS`: additional compiler arguments separated by whitespace.
- `EXPECT_COMPILE_ONLY: 1`: produce and verify an object file without linking or running.
  Use this for translation-unit probes whose external definitions live in another project.
- `EXPECT_SOURCES`: a JSON array of additional source paths relative to the test,
  for example `["helper.cpp", "subdirectory/another helper.cpp"]`. The runner passes
  each source as a separate input, links the executable, and runs its assertions.
  Name helper files without the `test_` prefix so they are not independent tests.
- `EXPECT_MANIFEST_SOURCE`: a source path relative to the CodeClip manifest's
  `projectRoot`, or an absolute path, selecting the translation unit whose
  preprocessing settings the test requires. The runner inherits its include
  directories, definitions, undefinitions, and forced includes, including source
  overrides. Optimization and warning policy belong to `EXPECT_COMPILE_ARGS`.

For external CommonLib probes in the Templates suite, pass the canonical manifest:

```powershell
./Tests/run.ps1 -Suite features/Templates -BuildManifestPath 'path/to/cl/builds/manifest/Release-x64.json'
```

Alternatively, set `CPRIME_TEST_BUILD_MANIFEST` to that path. Generate the manifest
with CodeClip for the selected project first. A missing manifest, missing selected
source, or ambiguous source selection is a test setup failure, not a skipped test.
The runner does not substitute library sources or add machine-specific include paths.
Compile-only, multiple-source, and manifest-dependent tests always compile fresh;
they do not reuse the optional shared executable cache.
