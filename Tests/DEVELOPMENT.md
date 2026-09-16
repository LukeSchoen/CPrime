# Development loop

Use root `cpc.exe`, one compiler process at a time.

```
Tests\test.exe -Suite features/Templates -Select test_x.cpp   reproduce
Tests\test.exe -Suite features/Templates                      the suite
Tests\test.exe -All -Tier fast                                the loop
Tests\test.exe -Regression                                    publication gate
scripts\build.exe                                             publish
```

Reproduce with a minimal source in the matching suite, repair the shared
mechanism, then keep the case. Do not relabel a failing case as expected.

Do not run the pedantic tier. It is close to banned: run it only as the last
and only step of an important confirmation, and avoid it if at all possible.
The replacement for a broad check is the affected suite plus `-Regression`:

```
Tests\test.exe -Suite features/X
Tests\test.exe -Regression
```

Pedantic holds every retained internal case and combines short pass cases into
unity units, running test programs `-Jobs` at a time while compilation stays
one serial batch per suite, so it is a real gate rather than a fast one.
Publication does not need it: `scripts\build.exe` runs `-Regression`.

Keep reproducers and logs in `build/` and delete them once the durable case
exists. `Tests/CPP17-REMAINING.md` is the only open-work list; there are no
retained status or verification logs, since git and the retained cases are the
record.
