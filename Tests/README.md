# Native test workflow

`test.exe` is the CPC-only harness. It discovers suites under `Tests/`, sends
each suite's compile jobs through one serial CPC batch, and runs the resulting
programs.

```
Tests\test.exe -All -Tier fast                    the routine loop (~1s)
Tests\test.exe -All -Tier pedantic                every retained case (~20s)
Tests\test.exe -Suite features/Templates          every case in one suite
Tests\test.exe -Suite features/Templates -Select test_name.cpp
Tests\test.exe -Regression                        publication gate
Tests\test.exe -RunnerChecks                      harness self-checks
```

Options: `-CompilerPath`, `-RuntimeRoot`, `-Timeout SECONDS`, `-Jobs N`
(parallel test programs, default 4), `-GroupSize N` (combined short cases per
unit, default 6, `1` disables), `-Verbose` (print PASS lines).

## Tiers

`Tests/tiers.json` holds the `fast` list and an `excluded` list.

- **fast** is the development loop: one representative case per area. It is
  deliberately tiny, so a passing case leaves the loop simply by not being
  listed.
- **pedantic** is every retained internal case that is not excluded. It is the
  release gate and it covers the whole corpus.
- **all** is the same as pedantic.

Incremental runs:

```
Tests\test.exe -Suite features/Templates -Select test_x.cpp
Tests\test.exe -Suite features/Templates
Tests\test.exe -All -Tier fast
Tests\test.exe -All -Tier pedantic
```

## Combined execution

Suites with at least 16 selected cases combine short, self-contained pass cases
into unity units: includes are hoisted and deduplicated, each body is wrapped
in its own namespace, and the unit returns the first failing member's status.
The case files never change; a combined unit that fails to compile or run is
recompiled and rerun case by case, so failures stay per case. Fail cases,
multi-source cases, and cases with output expectations always compile on their
own.

## Test metadata

Leading source comments: `EXPECT_EXIT`, `EXPECT_STDOUT`, `EXPECT_COMPILE_FAIL`,
`EXPECT_LINK_FAIL`, `EXPECT_COMPILE_ONLY`, `EXPECT_COMPILE_ARGS`,
`EXPECT_SOURCES`. A case with none of them must compile, run, and exit 0.

Generated files belong in `build/` and are disposable. Only the case itself is
durable.

## Rebuilding the workflows

The harness and other native workflows are first-party C sources:

```
scripts\tool-build.exe
```
