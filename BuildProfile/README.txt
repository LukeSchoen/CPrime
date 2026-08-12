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

Output:
- BuildProfile\out\build-profile.csv
- Console summary with average compile time per case.
