# Remaining work: C and self-compilation speed

## Acceptance and remaining margin

Keep serial compilation, complete source inputs, ordinary executable output,
C++ correctness, runtime regeneration and validated root publication intact.
Explicit `-x c` is a diagnostic, not a faster host or a reduced language build.
Do not change the benchmark baseline or omit slow cases to meet the threshold.

Use [the measurement data](progress/speed-2026-09-13.json) for the current
comparison and commands. The published eight-case ratio is 0.989 (63 samples
per case), versus the preparation audit's 1.473. The two preceding checks were
0.986 and 1.006: there is little headroom, and repeatable improvement beyond
that spread remains necessary. All per-case results remain in the data; the
many-functions case still trails TCC. The harness records medians, not raw
samples, so these are not confidence intervals.

Self-compilation still needs a controlled stability check: the two seven-sample
diagnostic medians are 367.386 and 314.775 ms, against the earlier audit's
345.981 ms and historical 322.493 ms. Do not claim a stable win from the faster
observation alone. Separate CPU time, scheduling/load and process wall time.
The self-driver source is CPC-only in the harness; no result establishes that
TCC compiled the same source. Use the shared C cases for that comparison.

Keep these implementation decisions when extending C++ support:

- Ordinary C identifier lookup must not traverse C++ template, namespace and
  implicit-receiver probes.
- Invalidate member-name tokens between translation units by generation;
  clear populated lookup tables and dirty filters without touching unused
  C++ tables on every C or preprocessing job.
- Retain complete DLL export catalogs for late references; materialize only
  referenced linker symbols and preserve first-defined/provider ordering.
- Use retained archive indexes to prove a rescan unnecessary; retain ordered
  rescans whenever a member can satisfy an unresolved reference.
- Amortize directory enumeration over repeated searches and retain exact-file
  opens for small inputs. Built-in declarations must retain their types and
  linkage while avoiding repetitive declaration-generating macros.

The combined cold-path gate is
`scripts/windows/test-compiler-cold-paths.cmd` (catalog name
`test_compiler_cold_paths`). It covers import precedence, missing/lazy symbols,
C++/C/C++ reset and a dependency discovered in a later archive. Keep this gate
under a few seconds. Use the existing include-search gate and focused C tests
when changing these paths. Diagnostic metadata under
`Tests/benchmarks/compile/costs/` reuses the same input for `-E`, `-c`, default,
explicit C and C++ modes, with the complete driver in its separate heavy row.

## Measurement work before optimization

1. Completed 2026-09-13: `perf_compare.c` now sums only matched successful
   CPC/TCC rows, reports CPC-only time separately, and fails incomplete shared
   measurements. Evidence: `build/package1/{perf-run.log,perf-missing.log}`.
2. The old noise heuristic was removed because it used CPC/TCC slowdown itself
   as evidence of a busy
   machine. That can discard a real compiler regression. Use independent load
   evidence and sample dispersion; keep raw failures/times and explain excluded
   samples. Ratios are useful but not machine-independent guarantees.
3. Completed 2026-09-13: failed root-CPC helper rebuilds stop the wrapper, and
   normal runs do not extract HEAD's second CPC. The native helper writes
   individual wall time, child CPU time, warmup state, exit and system-busy
   samples. Raw v2 headers record source/flag FNV-1a identities, root/compiler
   identities and runtime identity; `perf-dispersion` computes median/range and
   labels zero-tick CPU or negative-load rows unavailable rather than zero.
4. Completed 2026-09-13: automatic extraction/execution of HEAD's second CPC
   was removed from the normal path. Use root CPC only, with before/after data
   records taken around validated publication. Do not update the
   baseline, enable alternate hosts, or compensate with a tolerance increase.
5. Completed 2026-09-13: helper implementations moved to `src/tools/` and the
   shared inputs to `Tests/benchmarks/compile/`; `PerformanceTests.cmd` is the
   thin retained entry point and builds all native helpers with root CPC. Cost
   metadata and the preprocessing include path moved with the cases. The
   baseline/progress data remains here and generated results remain under
   `build/`; `-Out` creates nested evidence directories. There is no second
   benchmark harness.

## Profiling and implementation plan

Use root CPC, fixed source/flags and fresh output, with no competing compiler
processes. Establish repeated interleaved CPC/TCC shared-C samples and separate
root-CPC self-driver samples (at least seven measured repetitions, with raw
samples). Keep benchmark validation and timing outside each other's measured
intervals. Record the exact command from the self-driver metadata; `-E` measures
preprocessing alone and cannot substitute for full object/executable generation.

Profile the current compiler, not a historical binary. The existing native
sampler is `src/tools/profile_process.c`; ensure the map matches the measured
image. Add opt-in phase/counter instrumentation only where sampling cannot
separate setup, payload/includes, preprocessing, semantic work, code emission,
object/link output and cleanup. Disabled instrumentation must have negligible
cost. Remove temporary tracing before publication.

Prioritize measured causes:

- C-path overhead added for C++ support: profile is_cpp_translation_unit guards,
  declaration/member/template probes and lifecycle bookkeeping on ordinary C.
  Put work behind semantic language gates without bypassing required C behavior.
- Global scans: use ownership/name indexes with stable ordering and explicit
  invalidation rather than re-scanning declarations per token/candidate.
- Token replay and allocation: measure copied bytes, probe count and allocations;
  use per-invocation or per-function owned scratch storage where justified.
  Preserve error cleanup, nested replay, and batch reset.
- Startup: measure packed payload opening, include lookup and driver state before
  introducing caches. Cached output or skipping real initialization is not a win.
- Code generation/output: reduce repeated passes and writes only when profiles
  show a material share. Source splitting alone is not an algorithmic speedup.

Historical experiments already found several token/keyword caches slower and
identified next_nomacro and unary as hot. Treat that as a lead, not current
profile evidence. Revert changes that do not produce repeatable improvement.
The module/ownership plan in `src/compiler/README.md` applies; no concurrency.

## Acceptance

A package is accepted only with correct generated behavior, focused regression
checks, serial self-build validation when compiler sources change, and repeatable
speed improvement beyond sample spread on the identical inputs. Run the broader
language tiers once after the completed performance package. Required outcomes:

- Matched shared-C aggregate CPC/TCC ratio below 1.0 in repeated idle runs, with
  every per-case ratio reported so a large case cannot hide startup regressions.
- Full driver compilation restored to the historical target under comparable
  conditions, then improved; no unsupported claim that TCC compiled this source.
- Full Build.cmd succeeds and its phase times are recorded. Root cpc.exe is
  replaced only by the normal validated self-host publication path.
- No correctness baseline weakening, missing case, raised timeout, compiler
  parallelism or redefined workload. Passing the existing 25% drift gate alone
  is insufficient.

Keep the static leftover/environment counters as secondary diagnostics; textual
counts do not establish hot-path cost. Do not sweep or refactor source merely
to lower a counter. New timing/validation tools are native C built by root CPC.

## Current cycle record

Package 1 batch-state repair republished root CPC through `Build.cmd`; the
published SHA256 is
`0dba091a6ffa403b6b2860f7dd852b47f44ffd8d9bbee41a2d1b46a284df325df`.
The migration and raw-dispersion work now pass. CPC-only full-driver medians
from three seven-sample runs are 301.479, 302.023 and 299.761 ms, with raw
rows, identities and reader reports in `build/package2-self-stability/` and
its sibling logs. The last sample followed a passing serial root-CPC `Build.cmd`
publication (`build/package2-self-stability-build.log`); the root SHA256 stayed
`0dba091a6ffa403b6b2860f7dd852b47f44ffd8d9bbee41a2d1b46a284df325df`.
No TCC run occurred because external compiler invocation is unauthorized, so
the matched CPC/TCC acceptance remains blocked. Exact next action: obtain that
authorization and collect at least seven interleaved shared-C samples with the
migrated harness, then use `perf-dispersion` on each raw file before evaluating
the below-1.0 ratio and per-case results.

Current cycle (2026-09-13): no performance measurement was run while completing
the T1 language repair; invoking TCC remains unauthorized. The serial root-CPC
publication used for that repair passed at `build/package3-t1/build.log`.
Existing matched-C acceptance evidence is unchanged and still insufficient;
the exact next performance action remains the authorized seven-sample
interleaved CPC/TCC shared-C collection above.

Current cycle (2026-09-13): no performance measurement was run for the T1 enum
non-type specialization repair. Its serial root-CPC publication passed at
`build/package3-t1-crash20/build.log`; this records correctness publication
only and does not change the matched-C or self-driver acceptance evidence.
TCC was not invoked because external compiler invocation remains unauthorized.

Current cycle (2026-09-13): no performance measurement was run for the T1
anonymous-typedef linkage repair. Its serial root-CPC publication passed at
`build/package3-t1-nolinkage/build-final.log`; this is correctness publication only
and does not change matched-C or self-driver acceptance. TCC was not invoked
because external compiler invocation remains unauthorized.

Current cycle (2026-09-13): no performance measurement was run for the T1
variable-template linkage repair. The serial root-CPC publication passed at
`build/package3-t1-dr2387/build-merge-definition.log`; this is correctness
publication only and does not change matched-C or self-driver acceptance.
TCC was not invoked because external compiler invocation remains unauthorized.
