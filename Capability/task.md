# Capability: faster, better generated programs

Improve the quality and runtime speed of the executables root `cpc.exe`
produces, with real optimization levels, while keeping compilation fast. Aim at
the runtime speed of the equivalent clang, gcc or msvc build at comparable
flags, without ever becoming a shell around another backend. Never create
`done.x`: the bar keeps moving.

Work in this tree only. Delete nothing: `Capability\worker.cmd` and this file are
the user's control surface, and other worker folders may be in use on other
days.

## The two modes

- No optimization flags, the default: compile as fast as possible. Only cheap,
  effective transforms belong here, ones that cost no measurable compile time
  (the existing peephole and register promotion work is the reference), because
  compile speed is the reason to use CPC at all. A new pass that slows the
  default compile down without a clear runtime win does not belong in it.
- `-O1`, `-O2`, `-Os`: spend compile time to take on much more, trade depth
  against compile time, and still compile quickly compared to clang, gcc and
  msvc.

Today `-On` only defines `__OPTIMIZE__`; the levels are not real levels yet.
Making them mean something, and documenting them in `cpc -h`, is the work.

## Measure, every cycle

- Runtime speed of generated executables: fixed workloads, repeated runs,
  medians, same machine state, exact flags recorded. Workloads live in
  `Capability\tests\runtime\`, `Capability\tests\performance\pass\` (measured
  directly, not through the pedantic tier), and new runtime benchmark inputs
  authorized by this worker's purpose: deterministic, fast, self-contained, and
  kept with their inputs.
- Code quality: executable and object size, instruction counts where a
  disassembly comparison settles an argument, and the reported `-bench`
  statistics.
- Compile-time budget: the cycle already runs
  `src\scripts\performance.exe -Root . -CpcOnly -NoGate -Quiet`. An optimization
  that multiplies compile time is a regression even when the output is faster,
  so state both numbers for every change.
- Raw evidence, commands and timings stay under `Capability\build`. One sample is
  not a result.

## Invariants

- Optimizations must not change observable behaviour: constructors,
  destructors and calls that the program requires still happen; exceptions,
  RTTI, volatile access, atomics, floating-point behaviour and the Microsoft
  x64 ABI facts are unchanged; no object is read past its lifetime.
- Keep the retained cases green: the exact case, the affected suite,
  `Compatibility\tests\test.exe -All -Tier fast`, `Compatibility\tests\test.exe -Regression`. Do not run the
  pedantic tier. Publish with `src\scripts\build.exe` once packaging and the gate
  pass.
- Never make speed by weakening a case, relabelling a failure, or special-casing
  a benchmark by name. Revert experiments that fail; keep the tree self-hosting.
- `cpc.exe` must still compile itself, and the compiler it produces must still
  pass the gate: a faster program from a broken compiler is worth nothing.

## Leads

- Extend the bounded passes that already exist rather than building an IR:
  peephole selection, register promotion, local constant folding and
  propagation, dead store and dead code elimination, common subexpression reuse
  inside one function, better addressing modes and `lea` shapes, strength
  reduction, leaf frame elision, branch layout, tail calls, and a careful look
  at when inlining pays.
- Decide the inlining policy per level: the default keeps compile time, `-O2`
  may inline more when the callee is small, known and side-effect free.
- Check the optimizer against the pass list at `-O0`: nothing optional may run
  by default, so the fast mode stays fast.
- Compare against reference builds only when the user authorizes the external
  toolchains, and record the versions, flags and commands with the numbers.
  Unauthorized comparisons are blocked, not estimated.
- Track where the generated code loses to a reference at run time
  (function-call overhead, redundant loads and stores, spills, unaligned or
  unvectorized loops) and attack the top cost with measurements.

## Current cycle

`-On` is now a real level. `CPRIMEState.opt_level` records it, `-O0` clears the
optimize flag entirely (so `__OPTIMIZE__` is not defined and no transform
runs), `-O1` is the existing cheap set, `-Os` is the same set requested with
size over speed, and `-O2` parses as the top level. `cpc -h` documents the
four, and `__OPTIMIZE__` follows the level: it is defined for `-O1` and above
only.

Measured runtime (one 33-statement helper called in a 20M-iteration loop, three
runs of each flag, wall clock):

```
-O0: 1.338 1.324 1.428 s
-O1: 0.731 0.744 0.701 s
-O2: 0.745 0.701 0.755 s
-Os: 0.729 0.713 0.712 s
```

The level that pays is `-O0` versus `-O1`: 1.32-1.43 s against 0.70-0.74 s, so
the existing cheap transforms are worth 1.8-2.0x and turning them off is a
real level. `-O2` currently produces the same code as `-O1` (the numbers
overlap), so its promised extra pass is still open work.

An experiment that did not hold up: making `-O2` widen the fast inliner's
192-byte leaf-candidate limit to 1024 produced a clear 0.59-0.60 s against
`-O1`'s 0.70 s on the same loop, but the compiler it published could not
recompile the driver (`-O2` failed with a heap-corruption exit where the
previous compiler succeeded). The larger budget overflows a bound in the
inliner on the toolchain's own translation unit, so the change was reverted
and the root compiler rebuilt; self-host and the gate pass again. A safe way
to spend the `-O2` budget is the next thing to find, not a bigger number to
paste in.

Invariants were re-checked with the retained suites and the regression gate
before publishing, and the Capability probe passed at the default level. The
compile-time budget moved only for `-O2` inputs, which is the intended trade.

Next: split `function_ms` further and pick the strongest remaining gap between
the generated code and a reference build, then make `-Os` mean something
beyond "not `-O2`" (it currently shares the `-O1` budget).
