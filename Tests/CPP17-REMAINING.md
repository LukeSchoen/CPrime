# C++17 remaining scope

Working queue only: open gaps and the rules for closing them. Completed work is
code, retained cases are the record of what passes, and git is the record of
what changed. Root `cpc.exe` is the only compiler, one process at a time.

Every entry below was re-checked against root `cpc.exe` on 2026-09-16 and
records what the compiler does today; the quoted errors are verbatim. Re-verify
before trusting an entry that has aged.

## Test loop

```
Tests\test.exe -All -Tier fast        the loop: the open gaps below, ~0.2s
Tests\test.exe -Suite features/X      one suite, every retained case
Tests\test.exe -Regression            publication gate
scripts\build.exe                     self-host, validate, publish cpc.exe
```

Do not run the pedantic tier. It is close to banned: run it only as the last
and only step of an important confirmation, and avoid it if at all possible.
The affected suite plus `-Regression` is the broad check.

`Tests/tiers.json` lists the fast subset, which is the open work list; every
other retained internal case belongs to the pedantic tier, so a case leaves the
loop by not being listed.
Short self-contained pass cases compile and run in combined units at suite
scale; a combined unit that fails is recompiled and rerun case by case, and
`-GroupSize 1` disables combining.

## The gate is red on purpose

`features/Cpp17Gaps` carries one case per open gap, so the fast tier is red by
exactly the number of open cases. That count is the work list, and it falls as
gaps close.

The open-gap cases are the whole of `Tests/tiers.json`'s fast list, so the
routine loop reports the work list instead of hiding it until the pedantic run,
and a green fast run means the queue is empty. Fast is ~0.2s; pedantic carries
every representative case. A fixed case leaves `fast` by not being listed, and
returns only if the routine loop needs that behavior covered on every pass.

Each case names its facility, so a fix starts by running it:

```
Tests\test.exe -Suite features/Cpp17Gaps
```

## Where the cases live

- `features/Abi/pass` - Microsoft x64 layout, nullptr and record-return facts
- `features/Cpp17Gaps/pass` - minimal standalone reproducers for the open gaps;
  every case is a valid program that must compile, link and exit 0
- `features/Declarations/pass`, `features/Statements/pass`,
  `features/Templates/pass`, `features/Includes/pass` - structured bindings,
  selection initializers, variadic class and pack semantics, runtime header and
  container behavior
- `features/StdConcurrency/pass` - mutex, lock and thread behavior

## Rules

- Reproduce first, then repair the shared mechanism, then retain one case. A
  failed case stays in `pass/`, never relabelled as an expected failure.
- Keep tests minimal, deterministic, and fast; reuse an existing case when it
  already proves the behavior.
- Put one-off reproducers in `build/` and delete them once the durable case
  exists.
- Run the exact selected case, then the affected suite, then the fast tier at a
  boundary. Publication does not need the pedantic tier: `scripts\build.exe`
  runs `-Regression`, which is the gate.
