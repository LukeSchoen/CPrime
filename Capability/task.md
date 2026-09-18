# Capability: faster, better generated programs

Improve the quality and runtime speed of the executables root `cpc.exe`
produces, with real optimization levels, while keeping compilation fast. Aim at
the runtime speed of the equivalent clang, gcc or msvc build at comparable
flags, without ever becoming a shell around another backend. Never create
`done.x`: the bar keeps moving.

Work in this tree only. Delete nothing: `Capability\worker.cmd` and this file are
the user's control surface, and other worker folders may be in use on other
days.

This clone is one arm of a shared branch: up to three machines run the
Capability, Compatibility and Cost workers against the same origin, and
`Capability\worker.cmd` commits, fetches, rebases and pushes at every cycle
boundary. Expect the tree to hold the other agents' work when a cycle starts,
and expect the work left behind to be published to them. Leave git to the
worker: do not add, commit, fetch, rebase or push yourself, and never drop or
rewrite another agent's work to make a merge easier. When the worker opens a
cycle by naming an unfinished rebase, that merge is the cycle's task: keep both
sides' intent, run the area probe, the fast tier and `-Regression`, then finish
it with `git -c core.editor=true rebase --continue`.

## The two modes

- No optimization flags, the default: compile as fast as possible. Only cheap,
  effective transforms belong here, ones that cost no measurable compile time
  (the existing peephole and register promotion work is the reference), because
  compile speed is the reason to use CPC at all. A new pass that slows the
  default compile down without a clear runtime win does not belong in it.
- `-O1`, `-O2`, `-Os`: spend compile time to take on much more, trade depth
  against compile time, and still compile quickly compared to clang, gcc and
  msvc.

Today `-O1` is the cheap shared transform set and `-O2` adds the bounded C
rewrite pass; `-Os` still uses the `-O1` set. `cpc -hh` documents that.

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

## Open work

- Decide the `-Os` call-frame question. Promotion there costs text: +26 bytes on
  `test_cpp_call_compact_hot` and +17..+36 on the other retained `-Os` cases, for
  a 3x runtime win. Either find a size-neutral way to reach it (reclaiming the
  push region's slack, sharing a record shape, or promoting only the call case)
  or treat `-Os` promotion as a deliberate policy choice to be made with the
  user; the leaf cases must not grow just because the frame is enabled.
  `cpp_eh_has_code_offsets()` stays closed.
- Make the loop-head pad phase-neutral before claiming `-O2` runtime parity.
  `test_cpp_compact_leaf_134 -O2` moved +5.9 ms (8%) with its `hot` function
  byte-identical modulo position. The loop-head pad anchors to
  `align_base = start - FUNC_FAST_GAP` only when the gap is reclaimed, so check
  which absolute address the pad lands on for a reclaimed frame and whether the
  pre-change build was aligned at all.
- `test_template_list_runtime.cpp` and `test_heap_list_push_clear_perf.cpp`
  still hit the pre-existing private-member access failure.
- Closed unless a new measurement reopens them: Candidate B's inline-increment
  layout, the immediate-value fold (it stays out until a future layout removes
  enough branches), and the C `-O2` phase sweep (closed at phase 0).

## What cycle 0022 retained

Cycle 0022 replaced the fast frame's mov-to-slot nonvolatile saves with push/pop
saves and the shorter `UWOP_PUSH_NONVOL` unwind record. The frame gap is now 8
bytes (`FUNC_FAST_GAP`) instead of 16, the prolog pushes the promoted registers
between `mov rsp,rbp` and `sub rsp` (the allocation shrinks by what they
reserve, so the saved slots, the frame size and every local address are
unchanged), and the epilog drops the locals with `lea`, pops the registers in
reverse and pops `rbp`. The private unwind record is 12-16 bytes where the mov
frame needed 16-32, and its codes are one slot per push plus an allocation code,
listed in descending offset order.

Size, root `cpc.exe` -> diagnostic: `test_cpp_call_compact_hot -O2` 536 -> 492,
`hot_125/128/134 -O2` 352/372/368 -> 344/348/344, `leaf_125/128/134 -O2`
288/308/304 -> 280/284/280, `c.self.driver -O2` 1953500 -> 1932188 (-1.09%),
`c_compat.fast_nonvolatile_unwind -O2` 2174 -> 2006. Every `-O1` and `-Os`
retained binary and the C leaf cases are byte-identical, and no measured case
grew.

Compile-time medians for the same pair: `c.self.driver` 418.264 -> 395.744 ms
and `cpp.xbrz` 1556.382 -> 1557.848 ms. The paired runtime tables are in the
cycle-0022 evidence below.

Three defects were found by the retained cases and fixed before publishing:
push/pop of r12-r15 needs the +4 register bias under REX.B (`0x50+bit` pushed
and popped r8-r11); unwind codes must be listed in descending offset order (the
allocation goes first); and the reclaimed gap shifted the loop-head pad phase by
8 bytes, so the pad is anchored to the body's final address. The first two
failed `c_compat/test_fast_nonvolatile_unwind` until fixed.

Gates on the published compiler: `build.exe` packaging and regression 66/0;
Capability optimizations 3/0; Capability fast 2/0; Compatibility fast 29/0;
`-Regression` 66/0; Exceptions 63/0; the retained unwind case 1/0; all 21
retained case/level combinations compile and exit 0, and every published binary
is byte-identical to the diagnostic's. A second self-host build of the same
sources repeated 66/0 and the same `call_hot -O2` text; the published binary
hash changes per build only because the PE header carries a time stamp. Root
`cpc.exe` is
`0B66790B09B53CDED4662880BD2B47193598945557C66FEA7B7F81562DB2E5F1`.

Evidence lives under `Capability\build\cycle-0022`: size, runtime and
compile-time summaries, paired-run tables, bench logs and objects, unwind probes
and dumps, diagnostic compilers, gate logs and the retained case tables.

## Next action

Build a cycle-0023 diagnostic under `Capability\build` that reports, for one
promoted and one reclaimed function, the body's reserved and reclaimed
addresses, the pad anchor and the final loop-head address at `-O2`, then make
the pad land on the same absolute phase whether or not the epilog reclaims the
gap.  Verify with the exact retained cases, the affected suite, the fast tier
and `-Regression`, and re-measure the paired `test_cpp_compact_leaf_134 -O2` and
`test_cpp_call_compact_hot -O2` cells before touching the `-Os` policy question
again.
