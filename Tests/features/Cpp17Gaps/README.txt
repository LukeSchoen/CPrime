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
- Every case runs in the pedantic tier, which is every retained internal case
  that is not excluded. A failing case therefore shows up as a red count in
  the pedantic run, and that red count is the work list; the fast tier only
  covers the representative cases listed in Tests\tiers.json.
- Keep every case in the suite once it passes: it is the regression cover for
  the repair.
