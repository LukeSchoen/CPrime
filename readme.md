# CPrime

CPC compiles C, C++, and assembly to native Windows programs. Language and
runtime development is ongoing; see [remaining work](task.md).

## Build and use

```bat
Build.cmd
cpc.exe hello.c -o hello.exe
hello.exe
```

- `Build.cmd`: optimized Clang host (`-O3 -g0`). Requires
  `third-party/clang/bin/clang.exe`.
- `BuildClang.cmd`: faster development build (`-O0 -g0`).
- `Build.cmd --self`: serial self-build using the root bootstrap compiler.
- `BuildClangOptimised.cmd`: alias for the optimized build.

Builds regenerate the runtime, package the candidate, and run the regression
gate before replacing root `cpc.exe`. Preserve that executable and `lib/` for
bootstrapping. Generated output belongs in `build/`.

For solution builds, export a manifest with
`scripts/windows/export-build-manifest.ps1 -ProjectRoot <root> -SolutionPath <solution>`,
then use `build_project.ps1 -ProjectRoot <root>`. Paths and compiler selection
are configurable in the build driver.

## Test and develop

```bat
tests.cmd
powershell -NoProfile -ExecutionPolicy Bypass -File Tests/run.ps1 -Suite features/Templates
```

See [test commands and layout](Tests/README.md) and the
[development loop](Tests/DEVELOPMENT.md). Bug reports should include a standalone
reproducer, the command, and expected versus actual behavior.

Implementation lives in `src/`, headers in `include/`, test and benchmark inputs
in `Tests/`, and build scripts in `scripts/windows/`.
