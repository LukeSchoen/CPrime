# Cost: compile faster

Make root `cpc.exe` compile quicker, first for itself and then for C and C++
sources in general, without giving up correctness. Keep the whole tree
self-hosting: a speed change that cannot pass packaging and `-Regression` is
not a change. The queue never empties: when a package closes, take on the next
hardest case instead of declaring the job done, and never create `done.x`.

Work in this tree only. Delete nothing: `Cost\worker.cmd` and this file are the
user's control surface, and other worker folders may be in use on other days.

This clone is one arm of a shared branch: up to three machines run the Cost,
Capability and Compatibility workers against the same origin, and
`Cost\worker.cmd` commits, fetches, rebases and pushes at every cycle boundary.
Expect the tree to hold the other agents' work when a cycle starts, and expect
the work left behind to be published to them. Leave git to the worker: do not
add, commit, fetch, rebase or push yourself, and never drop or rewrite another
agent's work to make a merge easier. When the worker opens a cycle by naming an
unfinished rebase, that merge is the cycle's task: keep both sides' intent, run
the area probe, the fast tier and `-Regression`, then finish it with
`git -c core.editor=true rebase --continue`.

## What to make faster

1. The compiler building itself: `src\scripts\build.exe` (serial C-only self-host,
   cached package, validate, publish). Time the whole run. The per-cycle proxy
   is the retained `c.self.driver` case
   (`Cost/tests/compile/test_self_driver.c`), which compiles the complete
   driver translation unit.
2. Compiling C and C++ translation units, hardest case first:
   - heavy headers: long include chains, repeated inclusion, large declarations,
     deep nesting, system header weight
   - heavy templates: many instantiations, deep recursion, variadic packs,
     nested member templates, SFINAE and overload probes
   - deep constant evaluation and conversion ranking
   - macro-heavy preprocessing and very long token streams
   - large translation units, many functions and symbols, deep call graphs
   - repeated declaration, member, owner and source lookups

## Measure, every cycle, before and after

```
src\scripts\performance.exe -Root . -CpcOnly -NoGate -Quiet -Results Cost\build\perf-cycle-NNNN.tsv
src\scripts\performance.exe -Root . -CpcOnly -NoGate -Iterations 5 -Warmups 1 -RawSamples Cost\build\raw.tsv
src\scripts\build.exe                                  full self-host, publish on success
```

The worker runs the first command every cycle (its rows also land in
`Cost\build\cycles.csv`), and `Compatibility\tests\test.exe -Regression` as the
gate.

- The harness reads case metadata (`PERF_NAME`, `PERF_TIER`, `PERF_ITERATIONS`,
  `PERF_ARGS`, `PERF_SOURCE`) from the leading comment lines of
  `Cost/tests/compile/*.c`, runs strictly one compiler at a time and
  reports medians.
- Read the spread with `src/tools/perf_dispersion.c` (build it with root
  `cpc.exe` into `Cost\build\perf-dispersion.exe`) before claiming a win. One
  sample is not a result; repeat a serial run on the same input, flags and
  output path and compare medians.
- Record with every number: the exact command, the input, the flags, the output
  directory and what the machine was doing. Keep raw evidence under
  `Cost\build`.
- `cpc -bench` and `CPC_PROFILE_SCANS` show where compilation time goes; use
  them to choose the next target instead of guessing.

## Rules

- Root `cpc.exe` only, one compiler process at a time. Clang, GCC, MSVC and TCC
  runs are unauthorized unless the user authorizes them for a measurement; when
  that happens they are references only, never a build path. Record unauthorized
  checks as blocked rather than working around them.
- The C++ compile cases (`Cost/tests/compile/*.cpp`, `competitive/`) are
  measured through metadata-only `Cost/tests/compile/*.c` descriptors whose
  `PERF_SOURCE` names the `.cpp` translation unit. The harness scans only the
  top level of `Cost/tests/compile`; the `costs/` subdirectory is not scanned.
- After changing `src/tools/*.c`, rebuild the workflow executables with
  `src\scripts\tool-build.exe` (serial).
- Verify correctness with the exact case, then the affected suite, then
  `Compatibility\tests\test.exe -All -Tier fast` and `Compatibility\tests\test.exe -Regression`. Do not run
  the pedantic tier. Publish with `src\scripts\build.exe` only when packaging and
  the regression gate pass.
- Keep one focused change per cycle and revert experiments that do not hold up.
  No name-specific hacks, no disabling or relabelling cases, no retained scratch
  files.
- New compile-stress inputs are welcome when a hard aspect has no case, and are
  authorized by this worker's purpose: keep them deterministic, fast,
  self-contained and in `Cost/tests/compile`, with the case metadata in
  the leading comment.

## Leads

- `src/compiler/README.md` holds the current structural plan: phase
  measurements, remaining global scans in template and member lookup,
  declaration registration behind a small interface, explicit replay and
  function scratch storage, object/link isolation.
- Cost classes to attack with evidence: repeated environment and loader
  lookups, repeated path construction, token copying, linear scans over saved
  bodies or overload sets, per-token allocation, unbounded output writes.
- Batch and incremental paths (`--batch`, `-M` dependency runs, the tool chain
  in `src/scripts/`) compile many units with one compiler invocation: their state
  reset and cache validation are part of compile speed.
- `Cost\baseline\perf-baseline.tsv` records the retained numbers the
  harness gates against; keep the baseline meaningful when a case changes.

## The measurement bar, from cycle 0009

The profiling clock costs 46.5 ns per timestamp and every boundary takes two
(`Cost\build\clock-cost.c`), so a per-token bucket of about 100 ns is measuring
its own clock and the 1-in-64 sampled lexer window is worse. Read per-token rows
as upper bounds, and only chase a change whose per-occurrence cost is at least a
microsecond. Small harness cases carry an ~8 ms process-creation and loader
floor while the compiler's own phases total 2.9 ms, and the compiler's startup
is only about 1.5 ms above a trivial program's; that floor is not attackable in
CPC.

## Ground already covered

Retained product changes, each with its retained case or count -- do not rebuild
them:

- Cycle 0016: C-only fast path in `parse_btype()`; non-C++ identifiers go
  straight to `btype_typedef_resolution` instead of walking the class/template
  machinery whose C result is discarded (about -12.5 ms `btype_base`, -7.4 ms
  `btype_template`).
- Cycle 0017: `try_expand_cpp_alias_template()` is gated on
  `is_cpp_translation_unit()` at its `parse_btype()` and `unary()` call sites
  (333,028 C++-only probe calls per self-driver compile removed).
- Cycle 0018: `complete_deferred_nested_layouts()` returns early for C (924,672
  empty bucket visits per compile removed).
- Cycle 0019: the C++ implicit-virtual and defaulted-body emission block in the
  tagdecl tail is guarded for C (`emit_ms` 11.868 -> 0.043).
- Cycle 0020: the member declarator keeps its first `type_size()` result and
  recomputes only when it is negative (5,254 of 10,512 calls removed).
- Cycle 0021: the C identifier path carries its resolved binding into typedef
  resolution and reuses a non-scoped binding instead of calling
  `global_symbol_find()` again (36,423 sites).
- Cycle 0023: `next()` calls `cpp_alternative_operator_token()` only in C++ mode.
- Cycle 0024: the typedef-resolution label reached from `default:` with a
  non-identifier token skips symbol lookups, which can only miss there
  (global fallbacks 37,866 -> 17,683; the remaining ones are identifier
  entries).
- Cycle 0027: the restricted C static scalar initializer fast path and the
  single-integer-literal shortcut in `cprimegen_initializers.inc`.
- Cycle 0028: the same restricted path takes a single visible enum constant
  directly (1,026 generic expression parses removed).
- Cycle 0029: `paren_type_probe_possible()` skips the tentative `parse_btype()`
  probe for a `(` in C whose next token is below `TOK_IDENT` (14,222 skipped
  probes, `prefix_paren` -7.5 ms).
- Cycle 0032: `TOK_HAS_MACRO_STREAM_VALUE()` classifies the contiguous
  `TOK_CCHAR..TOK_U8CHAR` range with one unsigned range test.
- Cycle 0035: the C-only `DIF_SIZE_ONLY` aggregate skip in
  `decl_initializer_impl()`.
- Cycle 0036: `skip_balanced_block_c()` for the C size-only aggregate fast path.

Measured and rejected -- all reverted, and none should be retried without a
direct timer showing the work is material:

- Cycle 0013: the bucket-growth-safe non-const-alternative cache in the general
  template-member arm removed 717,440 candidate visits (`alt` 755,200 -> 37,760)
  for no measurable win; the visits fail the `class_tok` test early. The `alt`
  walk is counted, not costly.
- Cycle 0025: calling `next_nomacro_body()` directly from `next()` when detail
  profiling is off regressed the ordinary sample (CPU 390.625 -> 406.250 ms).
- Cycle 0031: the C-only `skip_or_save_block_mode()` fast path that skipped
  template-angle and qualified-name bookkeeping measured 7.218 vs 7.214 ms over
  eight interleaved pairs.
- Cycle 0033: the `macro_stream_space` fast path moved the parent rows more than
  its own row and is below run noise.
- Cycle 0022: the three member-prefix groupings (`VT_TYPEDEF` test first, cached
  `is_cpp_translation_unit()`, one grouped C++ probe guard) all failed to hold up.
- Cycle 0005: gating `materialize_incomplete_template_type()` on
  `is_cpp_translation_unit()`, and the early C `gfunc_param_typed()` path.
- Cycle 0006: the identity fast path for C argument conversion matched 5 of
  34,696 arguments; skipping `gen_assign_cast()` for C arguments failed 5 of 62
  regression cases.
- Cycle 0007: the C non-typedef parenthesized fast path, and the
  alias-template gate at the two hot call sites.

Measured without a product change:

- Cycle 0008: `paren_fallback` is the nested expression parse itself; only the
  1.9 ms speculative probe is removable, and cycle 0007 showed a token-class
  shortcut there does not pay.
- Cycle 0010: `member_address` is 0.751 us per access and its only redundant work
  is the zero-offset `vpushi(cumofs); gen_op('+')` pair, which must reproduce
  `gen_op`'s `range_direct` clearing, array-bounds capture and
  `constexpr_check_pointer_bounds()`. Measured but unclaimed, so it stays open
  below.
- Cycle 0026: the member-restricted prefix `probe`/`dispatch` children are 20-23
  ns and 44-51 ns per iteration. The parent rows stop being comparable as soon
  as child timers add one clock reading per boundary.
- Cycle 0030: the identifier-led parenthesized probe class is 0.323 us per probe,
  below the bar, so `paren_type_probe_possible()` is unchanged;
  `alloc_size_known` is noise (0.036 ms over 604 calls).
- Cycle 0034: every macro-loop row and the append growth/copy split are below 1
  us per occurrence; the append copy is 64 ns per token and growth is a single
  `tok_str_realloc`.

Still-open items that outlived their cycles:

- The cycle-0008 flaky gate case
  `test_static_member_template_unqualified_specializations.cpp` failed once in
  `cycle-0008-gate.log` with `unexpected compiler exit 1` and did not reproduce
  in eleven further attempts. It needs a deterministic reproducer before it is
  closed.
- The C++ `member_call` and virtual paths are dark on xBRZ because a C
  translation unit never reaches them (`member_call_types=1100` from
  `CPC_PROFILE_SCANS`), so the C++ lookup machinery is measured only through the
  scan counters.
- The `member_address` zero-offset add candidate stays measured but unclaimed:
  its 0.751 us per access is below the bar on its own, and removing the pair
  means reproducing `gen_op`'s range, bounds and constexpr side effects.
- One workload dominates the unknown-size save path: `x86_64-asm.c:230` saves
  39,558 tokens into a 65,536-token buffer for `asm_instrs[]`, far above the next
  largest (932 tokens).
- Housekeeping: remove `Cost\build\cpc-before.exe` and
  `Cost\build\cpc-after.exe`, the cycle-0029 A/B compiler copies, which the
  delete tool refused.

## Current handoff (cycle 0036 -> 0037)

Cycle 0036 carried out the cycle-0035 handoff: the size-only replay was split
into `loop`, `designator`, `elem_decl`, `skip` and `body` exclusive rows with
deterministic counters, which showed 480 aggregate skips at about 3.0 us each
against 0.12 ms of loop body, 0.13 ms of designator setup and 0.19 ms of
element-call overhead. The retained product change is `skip_balanced_block_c()`
in `cprimegen_statements.inc`, used only from the C size-only aggregate fast
path: it performs the delimiter/nesting walk C needs and omits the C++ template
and qualified-name bookkeeping the general `skip_or_save_block()` maintains.
Three interleaved exact-profile pairs using the preserved split compiler and the
final candidate measured `init_impl_array_skip` 1.271 -> 0.998 ms and
`alloc_size_unknown_replay_init` 1.643 -> 1.385 ms (medians), with the control
rows unchanged (body 0.122 -> 0.126, designator 0.116 -> 0.120, elem_decl
0.194 -> 0.201 ms).

All final gates passed: the exact self-driver compile exited 0 and wrote a
3,335,576-byte object, `features/Declarations` 78/78,
`features/Expressions` 230/230, `features/GnuExtensions` 24/24,
`-All -Tier fast` 29/29 suites, `Cost\tests` reports no suites, `-Regression`
66/66, and the final `src\scripts\build.exe` packaging/regression run passed
66/66 (`cycle-0036-final-build.log`; compiler 0.618 s, pack 0.098 s, regression
2.060 s, total 2.776 s). The published compiler SHA-256 is
`5BD48D8A8C66ABA0DD72D1DF6143F1F3DCB5A05A1FA0AF53B9C2CADAE2A23DAC`,
`fnv1a64=84d95ad44f12427d`.

Evidence is under `Cost\build`: `cycle-0036-build-before.log`,
`cycle-0036-instrument-build.log`, `cycle-0036-instrument2-build.log`,
`cycle-0036-candidate-build.log`, `cycle-0036-final-build.log`,
`cycle-0036-exact-self.log`, `perf-cycle-0036-before.tsv`,
`perf-cycle-0036.tsv`, `perf-cycle-0036-run2.tsv`,
`perf-cycle-0036-before2.tsv`, `perf-cycle-0036-after2.tsv`,
`raw-cycle-0036-before.tsv`, `raw-cycle-0036.tsv`,
`raw-cycle-0036-run2.tsv`, `raw-cycle-0036-before2.tsv`,
`raw-cycle-0036-after2.tsv`, their dispersion reports, the
focused/fast/Cost/regression logs, `cycle-0036-summary.tsv`, the preserved
`cpc-0036-baseline.exe` and `cpc-0036-split.exe`, the scratch
`probe-0036-cskip.c` and its passing run log, and under
`Cost\build\profile`: `cycle-0036-replay-before-{1,2,3}.txt`,
`cycle-0036-split-{1,2,3}.txt`, `cycle-0036-split2-{1,2,3}.txt`, and
`cycle-0036-ab-{split,cskip}-{1,2,3}.txt`.

Remaining work: the C-only balanced skip is still the dominant exclusive child
at a median 0.998 ms over 480 calls, about 2.1 microseconds per occurrence.
The helper still calls the general `next()` path for every token and performs
the delimiter/nesting branch on each returned token. Its `next()` advancement
and its delimiter bookkeeping are not yet separated, so the next change would
otherwise be a guess.

The exact next action is: add opt-in exclusive timers inside
`skip_balanced_block_c()` in `src/compiler/frontend/cprimegen_statements.inc`
for (1) `next()` token advancement and (2) delimiter/nesting bookkeeping.
Rebuild with `src\scripts\build.exe`, run the following command three serial
times, and make the first product change only in the larger exclusive child:

```
CPC_PROFILE_DETAIL=1 CPC_PROFILE_PHASES=1 .\cpc.exe -c -o Cost\build\profile\stage-replay.o -Isrc/include/runtime -Isrc/include/cprime -Isrc/third-party/win32-sdk/include -Isrc/third-party/win32-sdk/include/winapi -Isrc/compiler/frontend -Isrc/compiler/middleend -Isrc/compiler/backend/x64 -I. -DCPRIME_TARGET_PE -DCPRIME_TARGET_X86_64 src/compiler/driver/cprime.c
```

If `next()` dominates, measure and remove only the C++/logical-operator work
that still reaches this C skip; if delimiter bookkeeping dominates, reduce its
per-token branch work. Retain only with a repeatable A/B median improvement and
the exact case, affected suites, fast tier, regression gate and full self-host
build passing.

## External C++ workload: root cause (18 Sep, measured with root `cpc.exe`)

The external workload is a consumer project's 206 translation units (a read-only
checkout outside this repository, not named here); the target is under 100 ms
per unit and the measured mean is 700 ms, 144 s for the project serially. Full
detail and every generating script are in `Cost\build\rca-workload\`
(`FINDINGS.md`, `all-units-clean.csv`, `floor_split.py`, `expr_cost.py`,
`conv_probe.py`).

- Per unit: mean 700 ms, median 531 ms, max 3503 ms (the largest unit); 22 of
  206 are under 100 ms. The 71 server units average 1257 ms and are 62% of the
  total, the 134 core units 410 ms. Runs a minute apart differ by 10-30%.
- Preprocessing and lexing are 2-5% of a unit. The binding cost is the header
  environment re-parsed per unit: a unit holding one shader header plus an empty
  function costs 396 ms for 76,325 preprocessed lines and one holding a unit's
  13 includes costs 921 ms for 69,659 lines. Across a ten-unit sample the header
  floor is 55% of the time.
- Own code is dominated by conversions, not parsing: in the real header
  environment `g_int = 1;` costs 9 us and `doc.Get(name)` about nothing, while
  `File::Exists("x")` costs 1.8 ms, `doc.Get(key)` 1.5 ms and
  `Str s = nodeValue;` (char * to a class type) 1.8-3.4 ms. A bare-file probe
  points the mechanism at the constrained template constructor (20 us plain,
  84 us with the constrained one); the remaining factor is in the conversion and
  overload machinery.
- Declarations: the header set holds 3,507 tags costing 700-1000 ms
  (120-150 us each), 9,028 member declarators at 29 us, and 33,868
  typedef-resolution fallbacks at 1.5-1.9 us. The largest unit alone runs
  1,583,850 template-member candidate scans.
- One real quadratic: `complete_deferred_nested_layouts()` (`cprimegen.c:13863`)
  scans all 1,024 `cpp_record_declarations` buckets and every chain on every
  class definition. 8,000 trivial structs in one file cost 805 ms, 621 ms of it
  inside that call; 2,000 cost 80 ms. The workload's header sets hold about 2,500
  records, so the walk costs 19-27 ms per unit today.
- Not a regression: cycle-0007, cycle-29, cycle-36, cycle-20 and the current
  compiler agree within 10% on the same files. clang 23.1.1 needs 3168-3287 ms
  for the header-only unit cpc does in 890-950 ms, 4279 ms for a 13-include
  unit (cpc 2852-3355 ms) and 6673 ms for the largest unit (cpc 3503 ms). The
  100 ms target is not reachable by matching another compiler:
  either the header environment stops being parsed per unit, or declaration and
  conversion work falls by about an order of magnitude.

Next action: instrument the implicit conversion to a class type in a
header-backed environment (`Str s = charPtr;` then `File::Exists("x")`) with
exclusive timers around candidate collection, constraint evaluation and the
selected constructor, and remove work only from the largest child. The
`complete_deferred_nested_layouts()` scan is an independent, bounded win: index
records by `owner_tok`/`lexical_owner_tok` instead of scanning every bucket per
class definition, measured on generated files of 2,000 and 8,000 trivial
structs before and after.

## Header environment: shape, and the work that has to become cheaper

Detail and evidence are in `Cost\build\rca-workload\ENVIRONMENT.md`; the target
list with per-item measurements is in `Cost\build\rca-workload\IDEAS.md`. No
reuse, no precompiled environment, no skipping: the same declarations get
parsed, the same code gets emitted, faster.

The shape of the work, measured on the external workload:

- Preprocessing (files, macros, conditionals, includes) is 3-21% of a unit.
  The rest is declaration and expression work.
- A unit's preamble opens 317 files on average out of 512 project-wide
  (11.6k-50.2k preprocessed lines). Parsing it costs 3,662 top-level
  declarations, 1,462 tags, 6,401 member declarators and 89,606 template probes
  for the shader-header core (372-432 ms, 55 MB), and 9,151/3,507/9,028/91,062
  for the server include set (848-958 ms, 101 MB). Clusters: the largest header
  818 ms, a creature header 543 ms, the shader core 394 ms, an XML header
  260 ms.
- Per item: 93 us per top-level declaration, 120-150 us per tag, 29 us per
  member declarator, 0.6-1.7 us per template probe.
- Headers are not passive: an empty body with one header already emits a
  50-67 KB object. Emission is real work in the deferred drain (264 ms of
  1011 ms for the server header set, 1301 ms of 3970 ms for the largest unit).
- The unit's own code is where the biggest per-occurrence costs are: `g_int = 1;`
  costs 9 us while `File::Exists("x")` costs 1.8 ms and
  `Str s = nodeValue;` 1.8-3.4 ms.

Targets, in the order they should be taken:

1. `parse_btype()` probes (`cprimegen.c:16381`, `16716`, `16787`): 89-110k
   probes per unit at 0.6-1.7 us each. The template lookup runs twice before the
   code checks for `<`, and the qualified-name probe heap-allocates a
   `TokenString` before discovering there is no `:`. Peek the next token first,
   look up and allocate only when it can continue the construct.
2. Remaining `template_defs[]` sweeps: template registration
   (`cprimegen_templates.inc:11064`, and it recomputes
   `make_current_namespace_tok` inside the loop), `cpp_saved_angle_opener_tok`
   (`cprimegen.c:1243`), `cprimegen_lifecycle.inc:5950`,
   `cprimegen_constexpr.inc:804`, `cprimegen_templates.inc:4372` and `:18094`.
   Use the existing `template_def_candidates()` index. Also the measured
   `complete_deferred_nested_layouts()` bucket scan (`cprimegen.c:13863`).
3. A C++ member-declaration fast path: 29 us per member declarator over ~9,000
   members per header unit, while the C fast paths never fire in C++
   (`decl_fastpath_ms=0.000 calls=0`).
4. The conversion and overload path, after one instrumentation cycle on
   `Str s = charPtr;`: candidate collection, constraint evaluation, ranking,
   selected constructor. The bare-file probe already shows a constrained
   template constructor tripling a conversion's cost.
5. `decltype`/`typeof` at 88-185 us per entry, and the deferred drain's pass
   structure, once their instruments justify the change.
