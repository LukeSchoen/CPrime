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

Run the pedantic tier when a change crosses suites or before publication:

```
Tests\test.exe -All -Tier pedantic
```

It runs every retained internal case, including the fast subset, and is fast
enough to use at every boundary: short pass cases are combined into unity
units and test programs run `-Jobs` at a time, while compilation stays one
serial batch per suite.

Keep reproducers and logs in `build/` and delete them once the durable case
exists. `Tests/CPP17-REMAINING.md` and `task.md` list open work only.
