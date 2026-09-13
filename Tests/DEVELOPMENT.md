# Development and validation

The scope, work packages and acceptance criteria are in [task.md](../task.md).
C++17 is the cutoff. Resolve a complete mechanism/package, including its related
rows and negative controls, rather than cycling through one row at a time.
Plans record remaining work; code, tests and git history record completed work.

## Compiler and validation discipline

Use the repository root cpc.exe explicitly; PATH may select an unrelated binary.
One compiler process at a time. Build.cmd self-hosts, validates and publishes;
a failed build preserves root CPC. Compiler overrides are for build-internal
staging only. Do not use other hosts. The requested TCC performance comparison
is separate from MSVC/Clang/GCC ABI or build authorization.

Start with exact GCC selections and the nearest subsystem gate. Preserve the
original expected behavior and reduce external inputs to standalone local
fixtures. For a completed compiler package, run the related regressions, then
self-build and the appropriate language tiers once. Do not repeatedly run the
whole corpus after each edit. Individual language processes and fast standalone
gates retain their five-second ceiling; never retry a timeout with a larger one.

## Commands and truthful status

- `Tests/run.ps1 -Suite features/Templates -Select test_name.cpp` selects an
  exact regression across tiers unless a tier is explicitly restricted.
- `Tests/gates.ps1 -Subsystem members` selects related small regressions.
- `Tests/run-all.ps1 -Tier fast` includes the unresolved GCC assessment and
  therefore fails while positive retained cases fail; it is not currently green.
- `Tests/run-all.ps1 -Tier pedantic` guards established first-party language
  coverage, including already passing post-C++17 extensions. Run after a large
  package or consolidation; it does not run every native/performance workload.
- `Tests/run-all.ps1 -Tier all` runs both language partitions once.
- `Tests/run-checks.ps1 -Select <name>` selects a native gate; inspect it for
  external compilers/tools first. `tests.cmd` and `-IncludeChecks` include
  external ABI gates and are not blanket-authorized CPC-only validation.

A zero outstanding-row count is inventory, not proof of passing tests. Pending
reproducers and historical native/batch issues in task.md remain acceptance
work even if discovered language tests pass. The GCC reader/runner must accept
an empty corpus before the final row can be retired cleanly.

## Test consolidation

Combine related positive cases with identical flags and compatible scope into
one small source, using named checks or distinct failure return codes. Map all
original assertions to the combined test before deleting originals. Keep
compile-only declaration shapes, negative diagnostic cases and multi-source
linkage cases separate when combining would change what is exercised.

Tests/tiers.json lists the pedantic partition; an unlisted discovered case is
fast. Promote proven regressions to pedantic after consolidating them, retaining
a small representative fast set for active mechanisms. Keep stress workloads
explicitly pedantic. Record both process count and end-to-end time; runner
startup/cleanup time is distinct from compiler time. Batch compiler state must
be correct before using in-process batching to speed up testing.

Current pending reductions live in Tests/pending/ and are named in the outstanding
list. They are not expected-failure passes. Compile directly with root CPC;
after repair, move them to the matching first-party pass/fail suite and remove
the pending path. The initializer redefinition case must reject normally; the
other accepted-program reductions must compile/link/run successfully.

Runner/tier edits require the focused suite/fixture/tier checks. Corpus edits
also require `Tests/pedantic/gcc/test_corpus.ps1`. Do not expand old PowerShell
tooling by default: new shared helpers belong in src/ as native C; test-specific
sources in Tests/; generated executables and logs in build/.

## Worker reporting

worker.cmd reports current inventory counts, changes since the previous sample,
cycle exit, changed files and a distinct build/worker-cycle-N.log path. It omits
rate/trend/ETA projections from its normal display: differently sized fixes and
scope removals make those projections poor completion estimates.

The native tool retains the existing log protocol and optional detailed output:
`cpc.exe -o build/worker-status.exe src/tools/worker_status.c`, then
`build/worker-status.exe --root . --brief`. Use an explicit root path when
invoking CPC from outside this directory. Tests/progress/log.tsv remains
append-only; scope removal is not a repaired case even though counts fall.

Only validated task.md acceptance justifies done.x. Empty backlog no longer
creates it automatically. The worker retains its existing commit-at-cycle-boundary
behavior; do not run it to test formatting on an active working tree. Validate
the native reporter directly and worker control flow in an isolated fixture.
