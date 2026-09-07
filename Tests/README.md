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

`Tests/check_ucrt_runtime.ps1` checks the default Windows UCRT target against
a native Clang object. It passes file streams and allocations between compilers,
checks formatted I/O, buffered file positions, locale/global accessors, startup
arguments, exit callbacks, `setjmp`/`longjmp`, and generated-code shutdown with
`-run`. The PE import check rejects accidental legacy MSVCRT composition.
Use `-CompilerPath` and `-RuntimeRoot` together when testing an unpacked compiler;
the runtime archive must have been rebuilt by the same target compiler.

`Tests/test_NativeTls.ps1` links native COFF TLS objects directly and through
`-r`. It checks ordered initialization/callback subsections, 64-byte TLS
alignment, main/child thread isolation, native global constructors, dynamic TLS
initialization/destruction, and exit/pretermination/termination ordering.
The Destructors suite also checks interleaved class destructors and ordinary
`atexit` callbacks; the same ordering is required for executable and `-run` use.

`Tests/test_MsvcRecordReturn.ps1` compiles a native provider with MSVC and checks
small C++ record returns in both directions, including access, bases, special
members, nested records, and const results. It discovers the installed MSVC x64
toolchain with `vswhere`; `-NativeCompilerPath` can select another installation.
This gate uses MSVC itself because older Clang releases use a different POD
classification for some defaulted and nested records.

`Tests/check_ucrt_module_exit.ps1` checks executable and DLL ownership of
termination callbacks, mixed native/CPC `_onexit` and `atexit` ordering,
`quick_exit`, and `-run` callback lifetime. Its native thread-static fixture
links the installed MSVC runtime guard implementation and checks thread-local
storage on four worker threads. Pass `-NativeLibraryPath` if the MSVC x64
library directory is outside the standard Visual Studio installation paths.

`Tests/test_CoffWeakExternals.ps1` checks native weak aliases, deferred
`/alternatename` directives, archive fallback selection, DLL import aliases,
and `SECTION`/`SECREL` relocations, including relocatable output. Together with
`Tests/test_CoffLabels.ps1`, it verifies archive order and native object metadata
without changing the external project's source or library list.
The label tests also query Windows' unwind lookup for native functions whose
code and `.pdata$` contributions have different ordering and alignment. The
final x64 exception directory must pack and sort runtime-function records by
their relocated code addresses, including inputs combined with `-r`.

## GCC C++ regressions

`python Tests/gcc/run.py --fetch` downloads a pinned GCC testsuite checkout and
runs the supported standalone checks serially. See [gcc/README.md](gcc/README.md)
for selecting cases, collecting a full source survey, and the distinction
between checked results and unverified GCC-specific expectations.

The shared runner rejects abnormal compiler exits even for tests marked
`EXPECT_COMPILE_FAIL`; crashing is never a successful diagnostic test.

`powershell -NoProfile -ExecutionPolicy Bypass -File Tests/test_PortablePackaging.ps1`
builds the native C helper and checks header normalization, BOM decoding and
recursive processing, then copies the compiler
to an isolated temporary directory and compiles/runs a Windows/C++ probe using
only its embedded headers and runtime. Pass `-CompilerPath` to select a build.

## Layout

- Language suites, ABI fixtures, integration tests, and test runners live here.
- benchmarks/compile and benchmarks/runtime contain the inputs used by
  BuildProfile and codeProfile.
- repro/ preserves standalone reductions and external-project probes. These
  are investigation inputs, not automatically discovered passing regressions;
  some require CommonLib headers from the measurement project.
- Third-party upstream tests remain with their vendored dependencies.
- Generated compiler and test output belongs under build/ or the runner's
  temporary directory, not beside source files.
