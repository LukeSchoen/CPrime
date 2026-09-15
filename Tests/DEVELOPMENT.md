# Development loop

Use root `cpc.exe` and one compiler process at a time. Reproduce a defect with a
minimal source under the matching suite, repair the shared mechanism, run the
selected native test, then run the publication regression set. Use
`scripts/build.exe` only when compiler changes are ready for a validated
self-host publication.

```
Tests\test.exe -Suite features/Templates -Select test_name.cpp
Tests\test.exe -Regression
scripts\build.exe
```

The fast partition is the routine package gate; keep it short and focused. Move
large matrices, stress cases, duplicate cases, and costly cross-feature checks
to the pedantic partition in `tiers.json`. Run selected reproducers while
developing, the affected fast suite after a repair, and the complete fast gate
only at a suite boundary or before publication. The pedantic partition is for
large changes and release validation. Cross-compiler work remains isolated in
explicitly named native executables and requires separate authorization.

Keep raw diagnostics, response files, identities, and timing samples in
`build/` only while they are useful. Remove stale generated output. Keep
remaining-work Markdown current, but put completed behavior in code and tests.
