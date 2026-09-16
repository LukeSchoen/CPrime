# C++17 remaining scope

Working queue only: open gaps and the rules for closing them. Completed work is
code, retained cases are the record of what passes, and git is the record of
what changed. Root `cpc.exe` is the only compiler, one process at a time.

## Test loop

```
Compatibility\tests\test.exe -All -Tier fast        the loop: the open gaps below, ~0.2s
Compatibility\tests\test.exe -Suite features/X      one suite, every retained case
Compatibility\tests\test.exe -Regression            publication gate
src\scripts\build.exe                 self-host, validate, publish cpc.exe
```

Do not run the pedantic tier. It is close to banned: run it only as the last
and only step of an important confirmation, and avoid it if at all possible.
The affected suite plus `-Regression` is the broad check.

`Compatibility/tests/tiers.json` lists the fast subset, which is the open work list; every
other retained internal case belongs to the pedantic tier, so a case leaves the
loop by not being listed.
Short self-contained pass cases compile and run in combined units at suite
scale; a combined unit that fails is recompiled and rerun case by case, and
`-GroupSize 1` disables combining.

## Open gaps

`Compatibility/tests/tiers.json` holds one fast-tier case per gap that is still
red, so a green fast run means the queue is empty. The open gaps:

- `features/Cpp17Gaps/pass/test_attribute_before_function_template.cpp` - a
  standard attribute between declaration specifiers (`inline [[noreturn]] void
  f(int const &);`) is rejected. The Explorer++ probe reaches this shape through
  `boost/throw_exception.hpp`; the shape and the work required are in
  `Compatibility\KNOWN-ISSUES.md`.
- `features/All/pass/test_runtime_absolute_value_overloads.cpp` -
  `std::abs(-1L)` is ambiguous where `<cstdlib>` imports the C overload set into
  `namespace std` and also declares those signatures there; the shape and the
  work required are in `Compatibility\KNOWN-ISSUES.md`.

To add a gap: add one minimal case under `features/Cpp17Gaps/pass`, list it in
the fast list, reproduce it with root `cpc.exe`, repair the shared mechanism,
then retain the case and drop it from the fast list. A crash reproducer starts
in a suite of its own so one crash cannot abort a shared compile batch.

External CPC bug reports that are not part of the C++17 queue are recorded in
`KNOWN-ISSUES.md`.

## Where the cases live

- `features/Abi/pass` - Microsoft x64 layout, nullptr and record-return facts
- `features/Cpp17Gaps/pass` - minimal standalone reproducers, one per gap;
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
  boundary. Publication does not need the pedantic tier: `src\scripts\build.exe`
  runs `-Regression`, which is the gate.
