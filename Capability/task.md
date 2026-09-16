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
