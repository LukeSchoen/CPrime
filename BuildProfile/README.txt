BuildProfile

Purpose:
- Measure how quickly CPC builds representative inputs.
- Keep this separate from correctness tests so timing data can be audited without
  changing pass/fail language coverage.

Cases:
- Put compile-time profile inputs in BuildProfile\cases.
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
- BuildProfile\compare-tcc.cmd -CpcPath BuildProfile\clang-build\cpc-clang.exe

Output:
- BuildProfile\out\build-profile.csv
- BuildProfile\out\tcc-compare.csv
- BuildProfile\clang-build\cpc-clang.exe
- Console summary with average compile time per case.

Raw tcc baseline:
- Upstream tcc is vendored in third-party\tcc.
- BuildProfile\compare-tcc.cmd compiles only C profile cases, first with raw
  tcc and then with CPC, and reports CPC speed as a percentage of raw tcc.
