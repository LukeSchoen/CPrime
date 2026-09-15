# Native test workflow

`test.exe` is the CPC-only test harness. It discovers test sources, sends their
compile jobs through one serial CPC batch process per suite, then runs each
program with an independent timeout.

```
Tests\test.exe -Checks
Tests\test.exe -All -Tier fast
Tests\test.exe -All -Tier pedantic
Tests\test.exe -All -Tier all
Tests\test.exe -Suite features/Templates -Select test_name.cpp
Tests\test.exe -Regression
```

`-Checks` combines the first-party source policy, the publication regression
set, and the fast language partition. The publication build invokes
`-Regression` against its staged packaged compiler before replacing root
`cpc.exe`.

Keep the fast tier short: each case should cover one representative behavior
and finish quickly. Put broad matrices, stress cases, duplicate coverage, and
cross-feature cases in `tiers.json`'s pedantic list. While repairing a defect,
run an exact `-Select` set first; run the full fast tier only when the change
crosses suites or is ready for publication.

Test metadata uses leading source comments: `EXPECT_EXIT`, `EXPECT_STDOUT`,
`EXPECT_COMPILE_FAIL`, `EXPECT_COMPILE_ONLY`, `EXPECT_COMPILE_ARGS`, and
`EXPECT_SOURCES`. Generated files and timing evidence belong in `build/`. They
are disposable: retain only evidence for an active investigation, then remove
old logs and runner directories.

Cross-compiler ABI checks are separate and never run implicitly:

```
Tests\test-msvc.exe -RunExternal
```

That switch is an execution safeguard, not authorization by itself. Obtain
explicit authorization before invoking Clang or MSVC-compatible tools.

Rebuild native workflows after changing their C sources:

```
scripts\tool-build.exe
```
