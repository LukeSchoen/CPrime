# Remaining work: C++17 compatibility and compilation speed

## Scope and completion

C++17 is the language cutoff. Do not implement concepts, coroutines, C++20
aggregate parentheses, or later language features to retire tests. Delete a
failing row only when its required behavior is post-C++17; directory names and
bug dates are not evidence. Keep existing passing extension coverage in
pedantic. GNU extensions, C compatibility, shipped library headers, linking,
and optimizer-dependent GCC checks remain separate, visible work packages.
The retained corpus is a compatibility sample, not all of GCC or a claim of
complete C++17 conformance.

This plan replaces the one-row-per-cycle contract. Prepare a whole work package:
map every related row, identify shared mechanisms and independent blockers,
reduce distinct behaviors, implement the coherent change, and validate the
package once. Do not repeatedly sweep the corpus after each small edit. Do not
stop after an arbitrary session fraction or promote a first diagnostic into a
proven root cause. Update the inventory when a second blocker appears.

Completion requires all four conditions, independently:

1. Every in-scope row in [the GCC inventory](Tests/pedantic/gcc/README.md) and
   every issue below is resolved with observable first-party coverage. The
   retained manifest and outstanding failure list are empty; exclusions have
   reasons and never count as repairs. No crashes, retries, lost assertions, or
   expected-failure rewrites of accepted programs.
2. The fast language tier, established pedantic language tier, and applicable
   native/integration gates pass. Fast language wall time is at most 10 seconds
   on this audit machine; standalone fast gates remain below five seconds.
   Measure compiler/program time separately from runner overhead. A green
   language corpus alone does not establish green native gates.
3. Root CPC self-builds correctly and compilation speed meets
   [Performance/task.md](Performance/task.md): repeatable improvement on the
   identical full self-compile workload, and CPC faster than TCC on matched C
   workloads. No partial input, warm output reuse, alternate CPC host, relaxed
   correctness gate, or mixed-workload ratio.
4. Worker output identifies the backlog, current cycle, result, changes and log;
   it never equates row deletion with successful testing or speed acceptance.
   Create `done.x` only after conditions 1-3 have recorded passing evidence.

## Audit baseline and unresolved evidence

Audit date: 2026-09-13. Compiler SHA256:
`d6b7b79850efcfb1334c20e2b0350a283c79b80c96d3c93dd9993b25c210286a`.
The CPC-only `Tests/run-all.ps1 -Tier all` audit passed all 1,764 discovered
first-party cases; its only failing suite was GCC (51 compile/link failures,
zero passes). Raw evidence: `build/preparation-language.log` and
`build/gcc-preparation-audit/{metadata.json,results.jsonl,summary.json}`.
After excluding the coroutine row, 50 GCC rows remain. The original 191-case
fast first-party tier took about 12.29 seconds including the GCC assessment.
The current partition is 48 representative fast cases and 1,713 established
pedantic cases. All 1,761 first-party cases passed the post-consolidation audit
(`build/preparation-language-after.log`); the fast run passed its 48 cases and
still failed the 50 GCC rows in about 5.81 seconds
(`build/preparation-fast-after.log`). These wall-time estimates use the captured
logs' creation-to-final-write span on the same machine, not compiler-only time.
The lower source count combines five namespace cases into one and adds one
combined member-pointer regression; assertions remain represented.
Reporter/progress, corpus integrity, runner fixtures, suite discovery and tier
fixtures pass. An isolated worker fixture verifies one-cycle accounting,
per-cycle output and absence of automatic completion for an empty backlog.
Fresh speed evidence and measurement limitations are in Performance/task.md.
These observations do not clear the pending cases or native gates below.

The coroutine row `g++.dg/coroutines/pr113457.C` is outside the cutoff; its
concepts/ranges/promise work is cancelled. The `cpp26/aggr-init1.C` failure is
retained because its failing expression uses C++14 constexpr aggregate default
member initialization. A minimal C++17 reduction must preserve that defect.

## Work packages and dependency order

| Order | Package | Deliverable and exit check |
| --- | --- | --- |
| 1 | Test and measurement infrastructure | Empty inventory support, matched CPC/TCC accounting and raw wall/CPU/load samples are repaired. Native helpers now live in `src/tools/`, shared inputs in `Tests/benchmarks/compile/`, and `perf-dispersion.exe` reports raw wall median/range while labelling sub-tick CPU/load observations unavailable. The recovered 88-job prefix is `Tests/batch/historical-first-88.rsp`; its two-job anonymous-class reduction and the complete prefix pass with the root compiler. The permanent batch gate covers forward, reversed and interleaved orders. The separate historical 1,602-job stop remains blocked on recovery of its exact response file. |
| 2 | C/self-build speed | Preserve the C lookup, sparse cleanup and lazy import decisions in Performance/task.md. Three migrated CPC-only seven-sample self-driver runs have medians of 301.479, 302.023 and 299.761 ms, so the prior local timing spread is resolved. Matched CPC/TCC acceptance, including the narrow margin below 1.0, remains blocked because this task does not authorize TCC. Speed remains a gate on every later package. |
| 3 | Template ownership, substitution and lookup | Resolve packages T1-T4 in the GCC inventory in dependency order: declaration identity, replay scope, deduction, member-pointer representation. Consolidate variations into named checks; retain separate translation units when linkage is the behavior. |
| 4 | Initialization, lifecycle and exceptions | Resolve I1/E1 plus pending initializer defects; preserve constexpr versus dynamic initialization, cleanup order and emitted call behavior. |
| 5 | GNU/C, layout and library | Resolve X1/L1 including static C complex initialization, generic atomics, inheritance capacity and bitset; no platform-specific shortcuts or name tests. |
| 6 | Optimization and native correctness | Resolve O1 and native/tooling issues below. Required dead-code elimination is tested as such, not hidden by defining missing sentinel symbols. |
| 7 | Consolidation and release validation | Retire repaired external rows, run language tiers and required gates once, perform a serial Build.cmd, and repeat matched speed measurements on the resulting root CPC. Record any externally blocked ABI check explicitly. |

Compiler touchpoints and per-row acceptance are in the GCC inventory. Packages
may share a fix; do not merge distinct lookup, linkage, or diagnostic contexts
just because their current error text matches. Revert failed experiments.

## Known issues outside the GCC manifest

Paths below are relative to Tests/. Reproducers live in the repository; output
belongs in build/. The outstanding list is
`Tests/progress/first-party-failures.txt`; historical reports still requiring
reproduction are identified separately, rather than claimed as current passes.

| Issue | Evidence and plan | Required proof |
| --- | --- | --- |
| Initializer tag redefinition | `pending/initializer_redefinition.cpp` incorrectly compiles. Make constexpr probe/replay reuse declaration identity only for the same source declaration; preserve hard errors through recovery. Touch constexpr/initializers and declaration scope. | Genuine duplicate rejected, same initializer replay accepted, separate local tag allowed. |
| Lambda at start of braced initializer | `pending/lambda_braced_initializer.cpp` fails with `array type expected`. Make direct designator recognition use the lambda/designator distinction already needed by saved template bodies. | Noncapturing and capturing lambda elements run; GNU designators still initialize the correct slots. |
| Static C complex values | `pending/static_complex_initialization.c` fails with `initializer element is not constant`. Lower constant scalar/complex conversions to the two-part static representation in complex/initializer code. | File and local static objects have correct real/imaginary values, without C++ dynamic initialization. |
| Heap-list timing fixture | `pedantic/performance/pass/test_heap_list_push_clear_perf.cpp` still fails at GetTickCount64 lookup. Supply the correct declaration through a first-party compatibility header, then validate the fixture separately. | Compile and run the original workload; leave third-party SDK unchanged. |
| Batch state leakage | The exact historical first-88 prefix is retained as `Tests/batch/historical-first-88.rsp`. The two-job reduction was an unreset `defining_class_stack`/`active_member_class_tok` boundary: an anonymous class in job 1 qualified `Controls` in job 2, so its out-of-class definitions could not find the declaration. `cprimegen_init` now resets that parser-owned state. `Tests/check_batch_build.ps1` retains forward, reversed and interleaved runtime batches, and the full 88-job prefix passes. Evidence: `build/package1-cycle/{anonymous-pair.log,build.log,anonymous-pair-after.log,check-batch.log,historical-first-88.log}`. The historical 1,602-job/228-result stop remains independent and unreproduced. Next: recover its exact worker response file or explicitly record that it is unavailable; do not infer it is repaired from the 88-job result. | Retain the ordered regression, compare jobs with fresh-process root-CPC results, and reproduce the separate 228-result stop only from its exact worker response file. |
| Native record-return ABI | Historical `test_MsvcRecordReturn` failed on native_make<Defaulted4>, exit 8. Audit defaulted-constructor aggregate/trivial-record classification in the ABI path. External MSVC invocation is not authorized by this task. | Local CPC producer/consumer reduction plus the external ABI gate when authorized; keep ABI verification outstanding until then. |
| Assembly output | Historical `test_AsmOutput.cmd` failed before and after the language changes. Separate CPC direct/object/assembly paths and determine the first mismatch. Existing wrapper also invokes YASM; inspect before running. | Minimal local assembly input, correct symbols/relocations, equal runtime behavior and the gate's required object/executable checks. |
| Partial NRVO | Current optimization only handles a constructor-shaped local with a single trailing return in the outer function block. This is not automatically a language failure. Audit non-elided copy/move/destruction semantics before broadening optimization. | Distinguish optional elision from required C++17 prvalue elision; add a failure only for incorrect observable behavior. |

The historical templated member-pointer call report does not reproduce with the
new combined `features/Classes/pass/test_template_member_pointer_calls.cpp`.
Keep both owner shapes in that runnable regression; the unresolved GCC
member-pointer rows remain independent defects.

## Test consolidation decisions

Combine variations only when compiler flags, translation-unit scope and expected
outcome agree. Five namespace basics now share one file and distinct return
codes. Do not combine negative diagnostic cases: the first error masks later
checks. Do not merge multi-source linkage fixtures into one translation unit.
Move established passing post-C++17 extension tests to pedantic; retain their
coverage without funding further post-C++17 support.
The fast representatives cover C preprocessing/atomics, allocation and lifetime,
member pointers, lookup/substitution, initialization, exceptions, GNU extensions,
and ambiguity/rejection controls. The full tier remains required at package
boundaries; reducing routine invocations is not permission to delete that guard.

Next, consolidate the related member-pointer deduction/call forms, pretty-function
spellings, feature-query cases and template qualifier variants. Before deleting
an original, map every assertion and compile-only shape to a replacement check;
compare discovered counts and run the combined test. Use existing helper
headers rather than repeating type machinery. Move heavy stress tests to an
explicit pedantic gate with measured budgets. New runner implementation belongs
in src/ as C compiled by root CPC; no new PowerShell runner or compiler parallelism.

## Validation protocol

Use an explicit root compiler path, never PATH's bare cpc.exe. Run compiler
processes serially. During a package, run exact GCC selections and focused
first-party tests/subsystem gates. After consolidation, run both language tiers;
the GCC suite must continue to exit nonzero while unresolved positive cases fail.
A final green result cannot come from skipping that suite. `tests.cmd` and
`-IncludeChecks` include external ABI compilers and are not routine CPC-only checks.

Pending reductions are deliberately outside the passing suite discovery roots;
they are explicit unresolved work, not expected failures. Compile them directly
with root CPC, using `-c` for the expected-rejection case; accepted programs must
link and run to zero. On repair, move them into the appropriate pass/fail suite,
update the outstanding list, and delete the pending copy. For a genuine
rejection test, require an ordinary diagnostic rather than a crash.

Keep Markdown for remaining work and decisions. Completed implementation detail
belongs in code/tests and git history. Keep commands, identities, timing samples
and diagnostic logs under build/; summarize only evidence necessary to resume.

Cycle evidence (2026-09-13): empty corpus reader coverage, runner fixtures and
the current batch fixture passed in `build/package1/{test-corpus.log,test-runner.log,batch-current.log}`.
Root-CPC performance helper compilation and wrapper checks passed in
`build/package1/{perf-compare-build.log,performance-wrapper.log}`; matched and
missing-TCC accounting are demonstrated by `perf-run.log` and `perf-missing.log`.
No external ABI/compiler checks were run (unauthorized).

Cycle evidence (2026-09-13, package 1 continuation): root CPC built the raw
measurement helper (`build/package1-historical/perf-compare-build.log`); its
CPC-only seven-sample cost run wrote wall, child-CPU and system-busy columns to
`build/package1-historical/raw-cpc-only/samples.tsv`. Some short samples have
unavailable sub-tick CPU/load values and must not be interpreted as zero. The
historical batch investigation reproduced and reduced the failure; it is not a
passing fixture yet. No TCC, MSVC, Clang, YASM or other external compiler/ABI
invocation was run; the CPC/TCC acceptance comparison therefore remains
explicitly blocked.

Cycle evidence (2026-09-13, package 1 batch continuation): serial root-CPC
`Build.cmd` publication passed (`build/package1-historical/cycle63-build.log`)
but the published compiler still fails job 88 of
`Tests/batch/historical-first-88.rsp` after the anonymous-class job
(`build/package1-historical/cycle63-historical88.log`). Temporary diagnostics
were removed and root CPC was republished cleanly
(`build/package1-historical/cycle63-revert-build.log`). The scoped `Controls`
definition reaches `parse_cpp_scoped_member_def_body` with token 1615 and no
tag or ordinary binding (`cycle63-diagnostic4.log`); the ordinary class
declaration did not take the expected global `struct_decl` path in that
instrumented run. Next action: trace qualifier-token construction and the
class-declaration parser/replay boundary for the two-job manifest, then repair
the shared translation-unit ownership mechanism and add ordered, reversed and
interleaved passing batch coverage. No external compiler, ABI, TCC, MSVC,
Clang or YASM checks were run; all remain unauthorized.

Cycle evidence (2026-09-13, package 1 batch repair): root-CPC `Build.cmd`
publication passed and published SHA256 is
`0dba091a6ffa403b6b2860f7dd852b47f44ffd8d9bbee41a2d1b46a284df325df`.
The exact two-job response file failed before the repair
(`build/package1-cycle/anonymous-pair.log`) and passed afterward
(`anonymous-pair-after.log`). The retained `Tests/check_batch_build.ps1` gate
passed its existing protocol cases and the new forward, reversed and
interleaved anonymous-state runtime cases (`check-batch.log`); the recovered
88-job prefix passed through job 88 (`historical-first-88.log`). No external
compiler, ABI, TCC, MSVC, Clang or YASM check ran; those checks remain blocked
by authorization. Next package-1 action: migrate the performance helper/cases
and then add the required raw-sample dispersion reader before taking new
matched CPC/TCC evidence.

Cycle evidence (2026-09-13, package 1 migration and package 2 stability): root
CPC rebuilt `perf_check`, `perf_compare` and the new `perf_dispersion` from
`src/tools/`; the CPC-only wrapper check passed after the benchmark inputs moved
to `Tests/benchmarks/compile/` (`build/package1-migration-wrapper.log`). Raw
v2 samples now record root, runtime, compiler FNV-1a identities, source/flag
identities and the unchanged per-sample wall/CPU/load schema
(`build/package1-migration/raw-run/perf-samples.tsv`). The reader correctly
labels zero-tick CPU and negative-load samples unavailable
(`build/package1-migration-raw-dispersion.log`). CPC-only seven-sample full
driver medians were 301.479, 302.023 and 299.761 ms before/before/after a
passing serial `Build.cmd` publication; raw rows and dispersion reports are in
`build/package2-self-stability/{run1,run2,run3-postbuild}/perf-samples.tsv`
and `build/package2-self-stability-run{1,2,3-postbuild}-dispersion.log`.
Publication passed in `build/package2-self-stability-build.log`; root SHA256
remained `0dba091a6ffa403b6b2860f7dd852b47f44ffd8d9bbee41a2d1b46a284df325df`.
The wrapper and nested `-Out` creation check also pass in
`build/package1-migration-nested-wrapper.log` and
`build/package1-migration-nested-compare.log`.
No TCC, MSVC, Clang, YASM or external ABI/compiler check ran. Exact next action:
obtain explicit authorization for interleaved matched TCC measurements, then
compare every shared-C case against the recorded raw samples; do not create
`done.x` until that acceptance and the remaining correctness packages pass.

Cycle evidence (2026-09-13, package 3 T1 declaration ownership): root-CPC
`Build.cmd` publication passed after canonical qualified member-template lookup
was added in `cprimegen_templates.inc` (`build/package3-t1/build.log`); the
published root SHA256 is
`068b922789111aee4aea7715f8df6cc677371bb3e3168c492ec5ef506f5bd022`. The
published root compiler rejects neither the private class-head specialization
nor the primary/specialization/explicit-instantiation form: upstream
`g++.dg/parse/access1.C` and `g++.dg/template/access10.C` passed before their
corpus rows were retired (`build/package3-t1/gcc.log`), and the runnable
first-party replacement plus the focused Templates suite passed
(`build/package3-t1/{regression-after-retirement.log,templates-suite.log}`).
Corpus and runner fixture checks passed after the 50-to-48 inventory update
(`build/package3-t1/{test-corpus.log,test-runner.log}`). The fresh remaining
T1 map is `build/package3-t1/t1-remaining-results.jsonl`: variable templates,
local-linkage mangling, nested class-template-instance specialization, enum
non-type specialization, friend template-id parsing, and qualified friend
specialization remain distinct blockers. The enum row now reaches argument
matching, so it is not retired by the qualifier repair. No TCC, MSVC, Clang,
YASM, or external ABI/compiler check ran; matched CPC/TCC acceptance remains
blocked by authorization. Exact next action: implement enum non-type argument
canonicalization for `crash20` without weakening ordinary value-template
matching, then rerun its upstream row and a minimal runtime reduction before
continuing the remaining T1 mechanisms. `done.x` is not authorized: correctness
inventory and speed acceptance remain incomplete.

Cycle evidence (2026-09-13, package 3 T1 enum non-type specialization): root-CPC
`Build.cmd` publication passed after member-template value-parameter lookup began
replaying in the declaring record owner scope (`build/package3-t1-crash20/build.log`);
the published root SHA256 is
`f86a7a3ef2b377f795bd46ca443a08303c7538ff2d9489412fb85c5e65549122`.
The upstream enum specialization/typeid assembly row failed before the repair
(`baseline.log`) and passed afterward (`final-upstream.log`); its minimal
first-party runtime replacement passed (`regression.log`) and the related enum
identity, cv-deduction and non-type overload controls passed
(`related-regressions.log`). The row was retired, leaving 47 retained GCC rows.
No TCC, MSVC, Clang, YASM, or external ABI/compiler check ran; those checks
remain unauthorized and matched CPC/TCC acceptance remains blocked. Exact next
action: continue the remaining T1 cases in inventory order, beginning with
variable-template declaration/storage identity in `g++.dg/DRs/dr2387-aux.cc`;
map its companion-TU linkage requirement before implementation. `done.x` is
not authorized: the correctness inventory, native gates, and speed acceptance
remain incomplete.

Cycle evidence (2026-09-13, package 3 T1 variable-template linkage): root-CPC
`Build.cmd` publication passed at
`build/package3-t1-dr2387/build-merge-definition.log`; published root SHA256 is
`03e836d17e6e2b275e556b26578304ed64906acdf0814af6391acc031311057c`.
The upstream row failed before the repair (`baseline.log`) and passed after it
(`final-upstream-2.log`). The first-party two-TU runtime regression passed at
`build/package3-t1-dr2387/{regression.log,regression-run.log,regression-suite.log}`;
existing static-data and extern-template controls passed at
`{related-static-data.log,related-extern-template.log}`. T1 reassessment kept
the four independent rows `nolinkage1a`, `using3`, `instantiate11`, and `ttp53`
failing; exact diagnostics and summaries are under
`build/package3-t1-dr2387/t1-reassessment/`. The repaired row was retired,
leaving 46 GCC rows. No TCC, MSVC, Clang, YASM, or external ABI/compiler check
ran; those checks remain unauthorized and matched CPC/TCC acceptance remains
blocked. Exact next action: continue T1 with
`g++.dg/cpp0x/nolinkage1a.cc`, first mapping its local-linkage mangling forms
and any companion-TU requirement. `done.x` is not authorized: the correctness
inventory, native gates, and speed acceptance remain incomplete.

Cycle evidence (2026-09-13, package 3 T1 anonymous typedef linkage): mapped
`g++.dg/cpp0x/nolinkage1a.cc` into its independent global anonymous-record
pointer-typedef linkage failure and local template-argument control; it has no
companion translation unit. Root-CPC `Build.cmd` publication passed at
`build/package3-t1-nolinkage/build-final.log`; published root SHA256 is
`19d01bbe56021a5a9ce9571680eec5603938bfc68a14646e972bd1beee949788`.
The original row failed before the repair (`baseline/results.jsonl`) and passed
afterward (`after-upstream/{results.jsonl,summary.json}`). The first-party
runtime replacement and both related anonymous-typedef/local-class controls
passed in `build/package3-t1-nolinkage/{regression-final-suite.log,related-anonymous-control.log,related-local-control.log}`.
The row and unused support header were retired; corpus integrity passed with 45
rows (`test-corpus-final.log`). Independent T1 rows `using3`, `instantiate11`, and
`ttp53` remain failing with their exact diagnostics in
`build/package3-t1-nolinkage/t1-remaining/`. No TCC, MSVC, Clang, YASM, or
external ABI/compiler check ran; those remain unauthorized and matched CPC/TCC
acceptance remains blocked. The primary-versus-explicit nested-class
specialization map for `g++.dg/parse/using3.C` is captured in
`build/package3-t1-nolinkage/{using3-source.txt,using3-related-search.txt}`:
the `a<int>::a1` explicit specialization must suppress the invalid `a1 : int`
primary while `a<b>::a1` retains its dependent-base using declaration. Exact
next action: reduce those two instantiations, then trace pending nested-class
specialization selection before class-body replay; repair selection without
relaxing invalid primary-base rejection. `done.x` is not authorized: the correctness
inventory, native gates, and speed acceptance remain incomplete.
