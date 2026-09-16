C++17 gap suite

Scope:
- Minimal, standalone reproducers for the open C++17 gaps. Tests\CPP17-REMAINING.md
  owns the queue and the reproducer shapes; an external bug report is reduced to
  a local case here before any repair.
- pass/ holds valid programs that must compile, link and exit 0. A case that
  still fails names an open gap; a case that passes stays as the regression
  cover for its repair. None of them is relabeled an expected failure.
- fail/ holds programs that must be rejected, with EXPECT_COMPILE_FAIL. Those
  guard diagnostics, so they pass while the defect they describe is present and
  must keep passing once it is repaired.
- Cases whose only cost is a missing header keep one header and one minimal use
  per file so a failure names one facility.

Run:
- Tests\test.exe -Suite features/Cpp17Gaps
- Tests\test.exe -Suite features/Cpp17Gaps -Select test_clamp.cpp

Gate placement:
- The open-gap cases drive the fast tier, so the routine loop is the work list
  and its red count is visible on every pass. Fast is red by exactly the cases
  listed in Tests\tiers.json.
- As of 2026-09-16 the suite is 74 passed, 20 failed; those 20 are the current
  C++17 fast queue for this suite.
- Every other retained case belongs to the pedantic tier. Do not run that tier:
  it is close to banned, and `-Regression` is the publication gate.
- Keep every case in the suite once it passes: it is the regression cover for
  the repair.
