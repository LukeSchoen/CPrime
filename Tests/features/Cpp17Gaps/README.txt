C++17 gap suite

Scope:
- Minimal, standalone reproducers for the CPC deficiencies found by the CL
  repository's C++17 gap probe (C:\Luke\Src\CL\CpcRegressions.txt).
- pass/ holds valid programs that must compile, link and exit 0. They stay
  there until the shared compiler or runtime behavior is repaired; none of them
  is relabeled an expected failure.
- fail/ holds programs that must be rejected, with EXPECT_COMPILE_FAIL. Those
  guard diagnostics, so they pass while the defect they describe is present and
  must keep passing once it is repaired.
- Cases whose only cost is a missing header keep one header and one minimal use
  per file so a failure names one facility.

Run:
- Tests\test.exe -Suite features/Cpp17Gaps
- Tests\test.exe -Suite features/Cpp17Gaps -Select test_clamp.cpp

Gate placement:
- The open-gap cases are the whole of the fast tier, so the routine loop is the
  work list and its red count is visible on every pass.
- The suite is red by exactly the number of open gaps. As of 2026-09-16 that is
  18 cases: 17 in pass/ for missing facilities and one in fail/ for a missing
  diagnostic. Tests\CPP17-REMAINING.md owns the list.
- Every other retained case belongs to the pedantic tier. Do not run that tier:
  it is close to banned, and `-Regression` is the publication gate.
- Keep every case in the suite once it passes: it is the regression cover for
  the repair.
