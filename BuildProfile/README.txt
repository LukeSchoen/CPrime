BuildProfile

Purpose:
- Measure how quickly CPC builds representative inputs.
- Keep this separate from correctness tests so timing data can be audited without
  changing pass/fail language coverage.

Cases:
- Put compile-time profile inputs in Tests\benchmarks\compile.
- Files named test_*.c or test_*.cpp are compiled once per sample.
- Optional metadata in the first 12 lines:
  // PROFILE_NAME: readable.case.name
  // PROFILE_ARGS: extra cpc arguments

Run:
- BuildProfile.cmd
- BuildProfile.cmd -Iterations 10 -Warmups 2
- BuildProfile.cmd -CompilerPath path\to\cpc.exe
- BuildProfile\build-cpc-clang.cmd
- BuildProfile\compare-tcc.cmd -BuildTcc -Iterations 20 -Warmups 3
- BuildProfile\compare-tcc.cmd -BuildTcc -TccBuildCompiler path\to\clang.exe
- BuildProfile\compare-tcc.cmd -CpcPath build\clang\cpc-clang.exe
- BuildProfile\compare-asm.cmd -Iterations 50 -Warmups 5

Output:
- build\profiles\compile\build-profile.csv
- build\profiles\compile\tcc-compare.csv
- build\profiles\compile\asm-compare-samples.csv
- build\profiles\compile\asm-compare-summary.csv
- build\clang\cpc-clang.exe
- Console summary with average compile time per case.

Raw tcc baseline:
- Upstream tcc is vendored in third-party\tcc.
- BuildProfile\compare-tcc.cmd compiles only C profile cases, first with raw
  tcc and then with CPC, and reports CPC speed as a percentage of raw tcc.

Section-byte assembly serialization comparison:
- Yasm is vendored in third-party\yasm as a small standalone external assembler.
- BuildProfile\compare-asm.cmd emits CPC's current `-Sbytes` section-byte assembly
  serialization once per case, assembles it repeatedly with CPC and Yasm, links
  both objects with CPC, and reports timing plus byte-exactness against the
  direct CPC executable.

Full self-build packaging (2026-09-08)
- Build.cmd, self-built CPC, serial compilation, full embedded SDK/runtime.
- Header processing, payload serialization and compression use a native C helper
  rebuilt by CPC on each invocation. No C#, Add-Type, payload cache or
  incremental-build shortcut is used by packaging.
- Previous interpreted-PowerShell full build: 14.01 seconds. Native C runs:
  4.227, 4.366, 4.410 seconds; median 4.366 seconds. The first run followed
  deletion of build/compiler.
- Timings include command startup, both runtime builds, compiler compile/link,
  helper compilation, full packaging and replacement of the root executable.
- Original/new compressed payloads are byte-identical; cpc.exe is 2,222,398 bytes.
- Validation: test_PortablePackaging.ps1 (including isolated SDK/runtime use),
  c_compat (23 passed), features/Includes (41 passed).
