# CPrime

CPC compiles C, C++, and assembly to native Windows programs. Language and
runtime development is ongoing; see [remaining work](task.md).

## Build and use

```bat
Build.cmd
cpc.exe hello.c -o hello.exe
hello.exe
```

Use root `cpc.exe` for development. Plain `Build.cmd` rebuilds it serially with
that same CPC host and replaces root `cpc.exe` after validation. There are no
host-selection flags or alternate working compiler copies. Failure preserves
the working compiler and stops the build.
Agents must not switch to Clang, GCC, MSVC, or another compiler for builds,
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
