C++17 gap suite

Scope:
- Minimal, standalone reproducers for the CPC deficiencies found by the CL
  repository's C++17 gap probe (C:\Luke\Src\CL\CpcRegressions.txt).
- Every case is a valid program that must compile, link, and exit 0. Cases live
  in pass/ and stay there until the shared compiler or runtime behavior is
  repaired; none of them is relabeled an expected failure.
- Cases whose only cost is a missing header keep one header and one minimal use
  per file so a failure names one facility.

Run:
- Tests\test.exe -Suite features/Cpp17Gaps
- Tests\test.exe -Suite features/Cpp17Gaps -Select test_clamp.cpp

Gate placement:
- Every case is listed in Tests\tiers.json under "pedantic", so the routine fast
  gate and the publication -Checks gate are unaffected while the list is
  outstanding. The pedantic partition therefore reports these cases as failures
  until they are repaired; that red count is the work list.
- When a case passes on a published root cpc.exe, remove it from that list so
  the fast gate keeps it fixed.
