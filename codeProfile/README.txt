codeProfile

Purpose:
- Measure runtime of code emitted by CPC.
- Keep generated-code speed audits separate from correctness tests and from
  compiler build-time measurements.

Cases:
- Put runtime profile inputs in Tests\benchmarks\runtime.
- Files named test_*.c or test_*.cpp are compiled once, then the executable is
  run once per sample.
- Optional metadata in the first 12 lines:
  // PROFILE_NAME: readable.case.name
  // PROFILE_ARGS: extra cpc arguments
  // EXPECT_EXIT: process exit code, default 0

Run:
- codeProfile.cmd
- codeProfile.cmd -Iterations 10 -Warmups 2
- codeProfile.cmd -CompilerPath path\to\cpc.exe

Output:
- build\profiles\runtime\code-profile.csv
- Console summary with average runtime per case.
