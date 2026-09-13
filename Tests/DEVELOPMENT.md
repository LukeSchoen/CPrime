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

The fast partition is the routine package gate; the pedantic partition is for
large changes and release validation. Cross-compiler work remains isolated in
explicitly named native executables and requires separate authorization.

Keep raw diagnostics, response files, identities, and timing samples in
`build/`. Keep remaining-work Markdown current, but put completed behavior in
code and tests.
