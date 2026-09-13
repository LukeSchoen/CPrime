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
The earlier CPC-only full language audit passed all 1,764 discovered
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
| 7 | Consolidation and release validation | Retire repaired external rows, run language tiers and required gates once, perform a serial `scripts/build.exe`, and repeat matched speed measurements on the resulting root CPC. Record any externally blocked ABI check explicitly. |

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
A final green result cannot come from skipping that suite. `Tests/test-msvc.exe`
is an external ABI workflow and is not a routine CPC-only check.

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
rows (`test-corpus-final.log`).

Cycle evidence (2026-09-13, package 3 T1 nested-class specialization): root
CPC `scripts/build.exe` publication passed after explicit nested-class
specializations were registered separately from their outer class templates;
the root compiler now compiles upstream `g++.dg/parse/using3.C` and the
first-party runtime replacement plus the Templates suite and `Tests/test.exe
-Checks` passed. The row was retired, leaving 44 GCC rows. T1 reassessment
kept `instantiate11` and `ttp53` failing independently. No TCC, MSVC, Clang,
YASM, or external ABI/compiler check ran; matched CPC/TCC acceptance remains
blocked by authorization. Exact next action: repair the remaining qualified
friend-specialization row `g++.old-deja/g++.pt/ttp53.C`. `done.x` is not authorized: the
correctness inventory, native gates, and speed acceptance remain incomplete.

Cycle evidence (2026-09-13, package 3 T1 friend operator template-id): root
`scripts/build.exe` publication passed with SHA256
`506aa1f30d54eb0d7f6865c7c9f78951581662b2b6f939a5677de1d9cc0421da`.
The original `instantiate11.C` compiled and ran before its row was retired;
the first-party runnable replacement and the full Templates suite passed, as
did `Tests/test.exe -Checks`. The manifest now has 43 remaining rows. No TCC,
MSVC, Clang, YASM or external ABI/compiler check ran; matched CPC/TCC speed
acceptance remains blocked by authorization.

Cycle evidence (2026-09-13, package 3 T1 qualified friend specialization):
root `scripts/build.exe` publication passed with SHA256
`f62c030e28676ce1dfad40eec5d4a4c428d27874132eccb4eb8709d4f990538a`.
The upstream assemble-only `g++.old-deja/g++.pt/ttp53.C` passed before
retirement; its runnable first-party replacement, the Templates suite and
`Tests/test.exe -Checks` passed (`build/package3-t1-ttp53/{regression.log,
templates-suite.log,checks.log,build.log}`). The manifest now has 42 remaining
rows. No TCC, MSVC, Clang, YASM or external ABI/compiler check ran; matched
CPC/TCC speed acceptance remains blocked by authorization. Exact next action:
start T2 with `g++.dg/ipa/pr98075.C`, mapping unqualified global allocation
lookup from the saved template-member body before implementation. `done.x` is
not authorized: correctness inventory, native gates and speed acceptance
remain incomplete.

Cycle evidence (2026-09-13, package 3 T2 allocation lookup investigation):
the root compiler still rejects `g++.dg/ipa/pr98075.C` with an attempted
`xg<int>::operator new` lookup (`build/package3-t2-pr98075/baseline.log`). A
replay-only operator guard did not reach that lookup and was reverted; root CPC
was republished from the retained T1 source
(`build/package3-t1-ttp53/revert-t2-experiment-build.log`). Next: trace the
class-member name registration that selects the nonexistent member, preserving
actual class-specific allocation lookup. No external compiler, ABI or timing
work was run; speed acceptance remains blocked by TCC authorization.

Cycle evidence (2026-09-13, package 3 T2 allocation lookup repair): direct
`operator new(...)` and `operator delete(...)` expressions in member bodies no
longer take the implicit-`this` operator path; actual allocation expressions
continue through class-aware lifecycle lookup. Root `scripts/build.exe`
publication passed at `build/package3-t2-pr98075/build-final.log`. Upstream
`g++.dg/ipa/pr98075.C` compiled at `upstream-final.log`; the runnable
first-party replacement, Templates suite, class-allocation control and
`Tests/test.exe -Checks` passed at
`{regression.log,templates-suite.log,allocation-control.log,checks.log}`.
The row was retired, leaving 41 GCC rows. No TCC, MSVC, Clang, YASM, external
ABI/compiler or performance measurement ran; matched CPC/TCC speed acceptance
remains blocked by authorization. Next: reassess the remaining T2 rows as a
package before selecting the next independent blocker; `done.x` remains
unauthorized because correctness inventory, native gates and speed acceptance
are incomplete.

T2 reassessment after the allocation repair retained five independent blockers:
dependent nested class types in `pr48967.C` and `vararg-5.C`, lexical free
operator lookup in `pr96818.C`, deferred nested layout in `access37.C`, and
qualified explicit-instantiation parameter scope in `access6.C`
(`build/package3-t2-pr98075/t2-reassessment.log`). No row was inferred repaired
from another row's pass.

Cycle evidence (2026-09-13, package 3 T2 lexical operator lookup): saved member
body rewriting now distinguishes an operator-id from an ordinary identifier
whose spelling begins with `operator`. Root `scripts/build.exe` publication
passed at `build/package3-t2-pr96818-build-final2.log` with SHA256
`d257bb6bd03b15ed5452b634b0663e72cdb97b4e4ab157ba5efcf3c10b1564b4`.
Upstream `g++.dg/pr96818.C`, a runnable first-party lexical-prefix replacement,
and the static-member operator regression passed; `Tests/test.exe -Checks`
passed at `build/package3-t2-pr96818-checks.log`. The upstream row was retired,
leaving 39 GCC rows. No TCC, MSVC, Clang, YASM or external ABI/compiler check
ran; speed acceptance remains blocked by authorization. Next T2 work is the
independent dependent-nested-type pair, beginning with `vararg-5.C`; `done.x`
remains unauthorized because correctness inventory, native gates and speed
acceptance are incomplete.

Cycle evidence (2026-09-13, package 3 T2 dependent nested alias): member
`typedef` and `using` aliases now request only a referenced class
specialization's identity, matching the existing generated-template alias path.
The new first-party runtime regression
`features/Templates/pass/test_explicit_specialization_nested_alias.cpp`, the
serial root `scripts/build.exe` publication (SHA256
`119cb2e1dae60b8a4a10cc4179e06791bdf4b4e45072371564460e7d6fd51e99`) and
`Tests/test.exe -Checks` passed
(`build/package3-t2-vararg/{build-final.log,regression.log,checks.log}`). The
upstream `g++.dg/other/vararg-5.C` advances past its original
`b__void::c` nested-typedef error but now fails independently at the final
partial-specialization constructor call (`upstream-final.log`), so its row is
retained. Next: map that constructor deduction before altering candidate
selection. No external compiler, ABI or TCC measurement ran; correctness and
speed acceptance remain incomplete.

Cycle evidence (2026-09-13, package 3 T2 vararg dependent primary lookup): an
explicit template-id in a partial specialization now resolves through its
primary before matching a partial, and a following nested alias is consumed as
part of that type argument. The new runnable first-party regression
`features/Templates/pass/test_partial_specialization_explicit_primary_alias.cpp`
and the focused Templates suite passed; serial root `scripts/build.exe`
publication passed at `build/package3-t2-vararg-final-build.log` with its 28
native regressions. The upstream `g++.dg/other/vararg-5.C` advances past the
former line-18 arity failure but still fails independently at line 24: no
matching constructor for the selected partial with one argument
(`build/package3-t2-vararg-upstream-final.log`), so its row is retained. Next:
map construction/candidate deduction for `a<float, float>(r(...))` without
weakening the repaired primary lookup. No external compiler, ABI, TCC, MSVC,
Clang, or YASM work ran; `done.x` remains unauthorized.

Cycle evidence (2026-09-13, package 3 T2 vararg constructor mapping):
non-publishing root-CPC candidates built with `scripts/build.exe -NoPack
-NoValidate -NoPublish`. The representative row still fails only at line 24.
Trace evidence shows the partial's member-template declaration is captured,
but the original method scanner walks into the constructor initializer and
records `a::f(0)` as method `f`, so no constructor candidate reaches overload
deduction (`build/package3-t2-vararg-diagnostic4-trace.log`). Correcting that
registration in a diagnostic candidate still left the one-argument functional
call unmatched (`build/package3-t2-vararg-diagnostic7-trace.log`), so all
temporary tracing and unproven constructor-replay changes were reverted
without publication. Next: trace `instantiate_template_member_for_call`'s
concrete-member branch to record the argument type of `r(4, 6, 9, 7)` and the
exact rejection of the four-parameter member-template constructor. No broad
gate or external tool ran; the published root compiler remains unchanged and
`done.x` is not authorized.

Cycle evidence (2026-09-13, package 3 T2 vararg direct-partial member path):
the non-publishing concrete-member trace established that `r(4, 6, 9, 7)` has
type `a<int, int, int, int, int>`. The selected partial's constructor template
is recorded against the concrete specialization, but the normal instantiated
class path deliberately bypasses that direct entry and replays only primary
members. A diagnostic direct-entry candidate exposed the constructor and then
reached a distinct line-21 failure: the constructor initializer attempts a
default construction of `e<f, void>`
(`build/package3-t2-vararg-concrete4.log`). All direct-entry, declaration-name
and tracing experiments were reverted without publication. A subsequent
non-publishing candidate printed the saved initializer after member-template
substitution: `: a<float, float, int, int, int>::f()`. The line-21
`e<f, void>` failure is therefore an inherited-typedef lookup/lowering defect,
not loss of the injected class name or qualifier
(`build/package3-t2-vararg-init2.log`). Next: reduce qualified inherited
typedef lookup through `parse_constructor_member_initializers` to a
first-party runtime control, then repair it with direct partial
member-template selection only if both controls share the semantic lookup
path. No broad gate or external tool ran; `done.x` remains unauthorized.

Cycle evidence (2026-09-13, package 3 T2 vararg shared base substitution):
the direct partial-member diagnostic was rebuilt only with
`scripts/build.exe -NoPack -NoValidate -NoPublish`; no root publication or
broad gate ran. Base-edge instrumentation proves the first partial instance
records `a<float,int,int,int,int> -> e<float,void>`, but the representative
partial records `a<float,float,int,int,int> -> e<f,void>`. The latter is
already malformed before its constructor initializer is lowered, and its
inherited `f` consequently resolves to `e<f,void>`; native linkage merely
reports the unresolved formal. The shared mechanism is class-template replay
of a dependent qualified template-id (`a<j>::f`) in a base spec: the replay
substitutes `j` but leaves the terminal nested alias unevaluated, unlike the
free-function dependent nested-type path. A broadening trial of the existing
single-parameter dependent-alias branch did not affect this template-id form,
so it was reverted. Next: extend the class replay's template-id substitution
path to materialize a known qualified alias after its argument list is bound,
with the existing inherited-typedef initializer runtime control plus a
positive/negative nested-alias control. All tracing and unproven code changes
were reverted; only the durable first-party control remains untracked.

Cycle evidence (2026-09-13, package 3 T2 qualified-template-id candidate):
a diagnostic class-template `Template<args>::Alias` lowering candidate made
`vararg-5.C` compile and preserved the established alias regressions while the
ambiguous-partial negative control still rejected. The inherited-typedef
initializer runtime control then exposed an earlier, independent initializer
token boundary (`Holder<T>::type(37)` reports `'(' expected (got ':')`; the
`Wrapper::type` spelling reports `member initializer name`). The control cannot
yet validate the constructor path, so that candidate and all tracing were
reverted without publication. Next: reduce the qualified-initializer token
boundary independently, then rerun the full representative/control batch.
The root compiler and retained corpus are unchanged.

Cycle evidence (2026-09-13, package 3 T2 coherent vararg batch): the repaired
candidate passed `vararg-5.C`, both established nested-alias regressions, the
inherited-typedef initializer runtime control, the array partial-specialization
positive control, and the ambiguous partial-specialization negative control in
one serial CPC batch. `scripts/build.exe` then ran its full 28-test native
candidate regression gate successfully, but publication failed at the Windows
candidate-to-root replacement step and restored the root compiler. No broader
Templates gate or corpus retirement occurred. Next: diagnose the native
publish replacement failure (candidate is removed after the failed attempt;
root remains present), then rerun exactly one validated publication and the
Templates gate before retiring `vararg-5.C`.

Cycle evidence (2026-09-13, package 3 T2 qualified-template-id continuation):
the class-replay qualified-template-id path now resolves the terminal alias
through the ordinary known-specialization lookup, and constructor initializer
lookup preserves an inherited typedef's concrete target. A serial candidate
build followed by the representative batch compiled `vararg-5.C` and
`access6.C`; `access37.C` still fails independently on incomplete nested
layout. The inherited-typedef initializer and explicit-primary-alias runtime
controls both passed after a serial root `scripts/build.exe` publication, as
did its 28 native regressions. `Tests/test.exe -Checks` then reached the
Templates suite but failed nine existing batch-result cases, so the GCC row
is retained and no broad-gate acceptance is claimed. No external compiler,
ABI, TCC, MSVC, Clang, or YASM work ran; speed acceptance remains blocked by
authorization.

Cycle evidence (2026-09-13, package 3 T2 declaration-scanner and retirement):
a constructor-initializer guard in template member-name scanning had stopped at
both `:` and `::`, causing nine unrelated out-of-class template member and
conversion-address regressions to fail only in a native compiler batch. It now
stops only at a single initializer colon. A non-publishing candidate passed
all nine batch cases together with `vararg-5.C`, `access6.C`, their first-party
positive controls, and the retained `access37.C` negative control. A validated
serial root publication, all 32 Template tests, and `Tests/test.exe -Checks`
passed. `vararg-5.C` and `access6.C` are retired with their source rows
deleted; 37 rows remain. `access37.C` is an independent deferred-layout
mechanism. No external compiler, ABI, TCC, MSVC, Clang, or YASM work ran.

T3 reassessment (2026-09-13): all seven retained T3 rows still fail, but their
first blockers partition into default-argument declaration scope (`defarg4`),
non-type expression parsing (`arg6` and `spec18`), partial/member-pointer
deduction and completion (`array21`), inheritance conversion ranking
(`overload12`), explicit-argument-before-deduction (`explicit81`), and
template-template owner substitution (`ttp23`). Do not combine them merely
because they enter template overload resolution; begin the next implementation
cycle with the independent explicit-argument ordering mechanism.

Cycle evidence (2026-09-13, package 3 T3 functional non-type casts): the
largest viable T3 parsing cluster reduced to `arg6.C`; `spec18.C` is instead
a function-specialization pointer-argument parser failure and remains an
independent control. A candidate built with `-NoPack -NoValidate -NoPublish`
compiled and ran `arg6.C` plus the new first-party functional-cast runtime
regression, while `spec18.C` continued to reject and the partial-specialization
negative control continued to reject. Validated root publication, all 33
Template tests, and native regressions passed. `arg6.C` is retired; 36 GCC
rows remain. Next T3 mechanism: explicit arguments must be substituted before
deduction (`explicit81.C`). No external compiler, ABI, TCC, MSVC, Clang, or
YASM work ran.

Cycle evidence (2026-09-13, package 3 T2 vararg completion): the direct
partial-specialization member-template path and class-replay qualified
template-id alias lowering were published through `scripts/build.exe`; its
native regression gate passed 28/28. The relevant Templates gate passed 32/32
after a follow-up scanner correction that distinguishes a single constructor
initializer colon from `::` in out-of-class member definitions. The repaired
upstream `g++.dg/other/vararg-5.C` was retired with its first-party inherited
typedef initializer control. The published root also compiles
`g++.dg/template/access6.C`, whose scoped-instantiation regression passed in
the Templates gate, so that independently repaired row was retired as well.
`g++.dg/template/access37.C` remains an independent T2 blocker at incomplete
nested layout; next work starts there. No external compiler or ABI tool ran.

Cycle evidence (2026-09-13, package 3 T2 access37 deferred-layout probe): a
non-publishing CPC candidate deferred only an inner record whose by-value field
names an enclosing class currently being defined, then completed that inner
layout after the outer layout. This removed the original incomplete-field error
for both `DECLARE_FRIEND` variants. `DECLARE_FRIEND=1` compiled, while the
upstream/default form then failed only at `f(i)` with no matching overload;
existing friend-template controls remained accepted. The deferred layout is
therefore viable but reveals an independent ordering issue: the hidden friend
defined in the deferred nested record is not registered for ADL unless the
outer declaration makes it visible. All probe code was reverted without
publication. Next: preserve the nested hidden-friend declaration/ADL binding
while deferring its layout, then rerun both variants and the focused controls.

Cycle evidence (2026-09-13, package 3 T2 access37 deferred nested layout):
an inner record containing its enclosing template specialization by value now
defers only its own layout while that enclosing specialization is on the
definition stack, then completes all recorded concrete nested records after
the outer layout. The retained `access37.C` compiled and ran, as did the new
first-party `test_nested_class_enclosing_value_hidden_friend.cpp`; existing
friend-template-id, qualified-friend, and nested-class controls compiled in
the same serial candidate batch. The `DECLARE_FRIEND=1` variant remains an
intentional negative control: its additional inner friend declaration does not
make the hidden name reachable by ordinary lookup and rejects `f(i)`.
`scripts/build.exe -NoPack -NoValidate -NoPublish` was used for candidates;
one validated root publication passed its 28 native regressions and
`Tests/test.exe -Suite features/Templates` passed 34/34. `access37.C` is
retired with its corpus manifest and remaining-work row removed; 35 GCC rows
remain. No external compiler, ABI, TCC, MSVC, Clang, or YASM work ran. Next:
reassess the remaining T3 default-argument/deduction mechanisms before
selecting the largest independent cluster.

T3 reassessment (2026-09-13, after access37 retirement): root-CPC serial
reproduction confirms six distinct first blockers: member-template default
argument lookup (`defarg4.C`), array/member-pointer partial completion
(`array21.C`), inheritance conversion ranking (`overload12.C`), explicit
non-type arguments on member/static template calls (`explicit81.C`),
pointer-argument function-specialization parsing (`spec18.C`), and
template-template owner substitution (`ttp23.C`).

Cycle evidence (2026-09-13, T3 qualified static explicit arguments): the
object-qualified member path consumed `<args>` and then lowered a static
member-template specialization as an instance call, injecting a receiver into
the zero-argument function. It now selects the static specialization before
generic member lowering. A `-NoPack -NoValidate -NoPublish` candidate compiled
`explicit81.C`; its serial control batch passed the new observable
`test_qualified_static_member_template_calls.cpp` and three established
static/qualified controls, while retained `spec18.C` and the member-class
partial-specialization mismatch continued to reject. One validated root
publication passed 28 native regressions and the Templates gate passed 35/35.
`explicit81.C` is retired with its manifest and remaining-work row removed; 34
GCC rows remain. Next: reassess the remaining independent T3 rows, beginning
with the default-argument and partial-completion mechanisms. No external
compiler, ABI, TCC, MSVC, Clang, or YASM work ran.

Cycle evidence (2026-09-13, T3 defarg4 ownership probe): a diagnostic root-CPC
candidate built with `-NoPack -NoValidate -NoPublish` showed that `func` is
unqualified both when the member-template default is captured and when it is
replayed: `default_arg_class` is zero, and the instantiated function identity
does not recover the enclosing `foo`. Capture-time static-template
qualification and replay-time owner recovery therefore both leave the same
line-12 error. The probes were reverted. `defarg4.C` requires owner propagation
when an in-class member-template declaration is saved, before default-argument
lookup can be fixed; it is independent of the already accepted static-call
mechanism. Next T3 cluster: array/member-pointer partial completion (`array21.C`).

Cycle evidence (2026-09-13, T3 array21 partial-selection probe): serial
non-publishing candidates built with `-NoPack -NoValidate -NoPublish` traced
the base request through synchronous completion. The encoded
`void (file_reader::*)(int&)` argument never selects the
`dynamic_dispatch<void (TC::*)(int&)>` partial (`matching_partial_class_template`
returns none), so each completion attempt replays only the forward primary and
leaves its layout at `c=-1`. This is member-pointer pattern deduction, not a
pending-layout ordering failure; all trace edits were reverted. It shares the
T4 member-pointer representation family but is independent of T3 default
arguments and function-specialization pointer parsing. Next reassess the T4
two-row member-pointer identity cluster before changing shared representation.

T4 reassessment (2026-09-13): the six retained member-pointer rows do not form
one blanket parser failure. `g++.dg/inherit/ptrmem2.C` and
`g++.old-deja/g++.other/pmf4.C` share early member-pointer/function identity
redefinition failures across non-primary/virtual bases and are the largest
coherent next cluster. `member2.C` is native linkage encoding, `anonunion1.C`
is addressed member-template materialization, `g++.old-deja/g++.pt/ptrmem2.C`
is NTTP substitution through typedefs, and `pr60640-3.C` is covariant virtual
receiver lookup. Serial root-CPC reproduction preserved all six first
diagnostics; begin with the two-row identity cluster and retain runnable
receiver-adjustment controls.

Cycle evidence (2026-09-13, T4 member-pointer declarator identity probe): both
representatives reach `type_decl` with correct owner/name pairs (`C`/`FP`,
`D`/`D_f`, and `C`/`B_f`/`C_f`), but fail before `mk_cpp_member_pointer`
constructs any storage type. A trace-only candidate incidentally accepted both,
while the clean serial candidate restored `ptrmem2.C`'s incompatible `D_f`
redefinition; it is diagnostic-layout-sensitive parser state, not a valid
repair. The probe was reverted. The two rows remain a shared C-style
member-pointer declarator identity issue before backend representation or
receiver adjustment; next inspect saved/replayed declarator scope and
symbol-frame restoration around `type_decl`.

Cycle evidence (2026-09-13, T4 two-row identity split): a serial diagnostic
candidate accepted and ran a reduced control containing non-primary abstract
inheritance, heap construction, typedef-based C-style member-pointer casts,
and invocation. It still reproduced both retained rows. Thus the rows do not
share a general declarator, conversion, receiver-adjustment, or storage-type
mechanism: `ptrmem2.C` retains a virtual-override layout/identity interaction,
while `pmf4.C` retains global member-pointer initializer identity. The temporary
control and all instrumentation were removed. Next T4 work: select the larger
of the remaining independent member-pointer mechanisms rather than publishing
a trace-sensitive parser change.

Cycle evidence (2026-09-13, T4 member2 linkage trace): `member2.C` selects the
value-template overload but reaches `cpp_ms_template_name` with its written
`<1>` represented as the type token `int` while the linkage value mask marks it
non-type. The explicit-member-argument cache retains argument tokens but not
the source value/type classification or constant payload, so native linkage
cannot encode the selected integer specialization. This is independent of the
split C-style PMF rows. Next repair must extend explicit member argument
interning/binding with value-kind and constant identity, then test both
`bar<int>` and `bar<1>` against the inherited overload set.

Cycle acceptance (2026-09-13, T4 member2): explicit member-argument cache
entries already preserve `value_mask`; zero-argument explicit calls were
bypassing that viability check. The deduction gate now rejects only a
type/value-mismatched sibling and retains the original explicit key for a
viable zero-argument specialization. Candidate serial controls passed the
inherited type/value overload regression, qualified static controls, runnable
`member2.C`, and the independent rejecting `spec18.C`. Publication passed 28
native regressions; Templates passed 36/37. `member2.C` is retired. The sole
Templates failure, `test_member_template_default_static_function.cpp`, is the
separate bare static-template-address/default-expression mechanism and is next.

Cycle acceptance (2026-09-14, T3 defarg4): generated member-template
prototypes now carry an owner only for default-argument capture; ordinary
pending-specialization replay lookup remains unchanged. A bare static
member-template address is retained as an owner-tagged unresolved overload and
deduced from its target function-pointer type, rather than colliding with the
class-qualified template declaration. Candidate controls passed the new
default-static regression, static-template address and static-function
argument positives, and the rejecting specialization negative; `defarg4.C`
compiled. Root publication passed 28 native regressions and Templates passed
37/37. `defarg4.C` is retired. The array/default-decay failure remains a
separate T3 mechanism; next batch-reassess the remaining partial-completion
and overload-ranking rows before selecting its largest coherent cluster.

Cycle evidence (2026-09-14, next T3 reassessment): after `defarg4.C` retired,
`array21.C` no longer reaches its previously recorded member-pointer partial
selection blocker: root CPC first rejects its line-3 unnamed non-type function
template parameter (`template<int> void* get(int)`) with `typename expected`.
`asm1.C` reproduces the same `template <int> int f` first diagnostic. In
contrast, `tmplattr2.C` and `pr25811-3.C` stop at independent earlier errors.
Class-template controls already accept `template<int>`, so `array21.C` and
`asm1.C` form a function-template parameter-parser cluster, distinct from
member-pointer partial completion. The existing member-pointer/template
controls and the new default-static regression all pass. Next: repair unnamed
non-type function-template parameter parsing for those two rows before
returning to `array21.C`'s deferred partial-specialization completion.

Follow-up (2026-09-14): the current non-publishing candidate advances both
rows past that header (root CPC still retains the published first diagnostic):
`array21.C` reaches its real incomplete
`dynamic_dispatch<void (file_reader::*)(int&)>` base, while `asm1.C` reaches
the independent inline-assembly `%` operand parser. They are not a coherent
repair cluster and no publication was attempted. Inspection confirms that the
partial matcher handles `R (K::*)(T...)` only when the return `R` is itself a
template parameter; `array21.C` starts with concrete `void`, so the outer
partial matcher compares `void` to the full member-pointer argument before it
can bind `TC`. A narrow concrete-builtin member-pointer matcher was rejected:
it preserved all focused controls but still did not select the partial, so it
was reverted. Next `array21` repair: trace the actual argument token/type at
the outer matcher and unify concrete-return handling with the existing
member-pointer pattern parser rather than adding a parallel shape parser.

Follow-up partial trace (2026-09-14): the outer matcher sees pattern `void`
and the encoded argument `__cpc_template_type_struct___cpc_member_pointer_...`;
`make_type_from_type_arg_tok` resolves it to a real member-pointer record with
owner `file_reader`, function member type, and `void` return. Thus neither
the encoded argument nor return identity is missing. The failed narrow matcher
did not complete the pattern binding/consumption and was removed. Next trace
must compare the candidate's written `TC::*(int&)` token cursor with the
existing `R (K::*)(T...)` consumption path, then extend that one parser.

Cycle evidence (2026-09-14, comdat1 exception-spec probe): the member suffix
parser already consumes legacy `throw()`, but applying it after every generic
function declarator makes the candidate reject the focused retained
cross-thread exception regression. A narrower post-declarator `throw()` hook
has the same regression and was reverted. `comdat1.C` therefore needs the
dynamic exception specification represented during normal function declarator
attribute construction, before declaration/type merging; do not add a
post-declarator token consumer. The EH rows remain independent of the template
partial-deduction work.

Cycle acceptance (2026-09-14, T3 array21): class-partial matching now accepts
a concrete builtin return before a member-function-pointer owner pattern and
compares builtin parameters through their reference wrapper while retaining
reference-category checks. This binds `TC` from `void (TC::*)(int&)` to the
encoded member-pointer owner without weakening the existing parameterized
return path. Candidate validation passed `array21.C`, the new compile-only
`test_partial_specialization_concrete_member_pointer_return.cpp`, member-pointer
deduction/address positives, template-template shape positive, default-static
positive, and rejecting specialization negative. Publication passed 28 native
regressions and Templates passed 40/40. `array21.C` is retired. Next reassess
the remaining T3 overload and specialization rows; `asm1.C` remains an
independent inline-assembly parser mechanism.

Cycle evidence (2026-09-14, T4 addressed member-template reassessment):
`g++.dg/template/anonunion1.C` reaches the existing direct qualified
member-pointer path with the correct `F`/`bar` pair and contextual
`void (F::*)(int)` target. It fails because captured inline member-template
metadata records `template <zm>` as a type parameter (`value_mask == 0`), so
the explicit non-type `&Z::foo` is rejected before materialization and lookup
falls through to `F_bar`. The typedef and anonymous-union body are not the
failure. A candidate metadata reconstruction was not reached because the
stored declaration begins with a wrapper record; it was reverted. This row is
independent of `member2.C`. Next: trace wrapper/parameter capture from
`skip_or_save_template_member_decl` through `add_template_member_def`.

Follow-up: capture inspection confirms the saved stream is line records plus
the intact `template <zm> void bar(int)` header. A narrow normalization at
`add_template_member_def` did not alter the later candidate metadata or the
row's failure and was reverted. The loss is therefore after initial capture,
in declaration merging/replay; inspect that path next rather than adding a
downstream kind inference fallback.

Further controls: skipping leading replay line records and preserving a lone
typedef token as an unnamed non-type declaration both left `anonunion1.C`
unchanged; both experiments were reverted. The rejection is downstream of
template-header kind parsing, during addressed-specialization materialization
or member overload registration. Next trace the specialization symbol emitted
by `instantiate_template_member_if_needed` against
`find_member_func_address_candidate`.

Cycle acceptance (2026-09-14, T4 addressed member template): a complete
explicit member-template argument list now identifies its specialization after
kind validation, rather than requiring deduction from function parameters that
do not mention a non-type parameter. This materializes addressed non-type
member specializations before member-pointer candidate lookup. The new runnable
member-pointer/non-type regression, existing explicit-kind/static-address
controls, compile-only `anonunion1.C`, and rejecting `spec18.C` passed in one
serial candidate batch. Validated publication passed 28 native regressions and
Templates passed 38/38. `anonunion1.C` is retired. The remaining T4 rows remain
the independent C-style PMF identity and member-pointer partial-specialization
mechanisms; reassess those next.

Next-cluster diagnostic (2026-09-14): fresh candidate reproduction split the
remaining two T4 PMF rows again: `g++.old-deja/g++.pt/ptrmem2.C` stops at the
member-template-containing `h` declaration (`invalid type for 'h'`), while
`g++.old-deja/g++.other/pmf4.C` stops earlier at global PMF initializer identity
(`B_f` incompatible redefinition). The new addressed-member-pointer regression
passes in the same batch. Treat the rows as independent; begin with ptrmem2's
template field/type construction, not PMF storage conversion.

Cycle acceptance (2026-09-14, T4 comma member-function declarators): an
in-class member function declarator followed by `,` now declares that function
and continues the declaration list, rather than treating the valid comma as an
invalid terminator. The new runnable comma/template-address regression and the
full runnable `ptrmem2.C` passed; the independent `pmf4.C` still rejected.
Validated publication passed 28 native regressions and Templates passed 39/39.
`ptrmem2.C` is retired. Next: PMF initializer type identity in `pmf4.C`.

PMF follow-up: `pmf4.C` is not a member-pointer conversion failure. The
declared member `B::f` is lowered to global `B_f`, colliding with the source
global PMF variable `B_f`; the redefinition occurs while parsing that variable's
initializer. Temporary tracing was reverted. Treat this as a C++ member-linkage
namespace collision, requiring a naming/alias audit beyond T4 PMF storage.

Naming audit: ordinary non-static members use `make_member_func_tok`, and that
is the same joined token used for static-member lookup (`B_f`). The global
symbol table therefore cannot distinguish `B::f` from a source global `B_f`.
Repair requires a canonical member-linkage token and preserved source-name
lookup across declaration, call, virtual dispatch and member-address paths;
do not apply a PMF-local rename.

Implementation audit: `declare_member_func` records the emitted token in the
member-overload registry, but `make_member_func_tok_for_type` and several
non-overloaded resolution paths still probe the joined base token directly.
A safe canonical-token migration must update declaration, direct resolution,
out-of-class definition matching, virtual dispatch and member-address lookup as
one batch; partial PMF renaming is rejected.

Resolution audit: member-pointer address formation already selects through
`member_func_candidates`, but ordinary `resolve_member_func` probes the joined
token before field fallback. Any canonical emitted-token batch must change this
to consult the registered direct candidate first, then retain joined-token
lookup only as a legacy fallback.

Migration probe: a private emitted-token prefix removes the `B_f` collision,
but `void C::f()` then loses its registered-member definition mapping and is
parsed as a free function (`this` undeclared). The probe was reverted. Include
qualified out-of-class definition binding in the canonical-token batch.

Follow-up (2026-09-14): the current canonical-token migration candidate gets
`pmf4.C` through compilation, but its executable exits `0xC0000005`. The
collision is therefore removed while PMF receiver/virtual-dispatch linkage is
not yet preserved. The former first-party global-name-collision regression is
temporarily absent with this migration; do not retire `pmf4.C` or publish until
that regression is restored and the runnable PMF call path passes.

Binder audit: the later semantic definition binder already calls
`resolve_member_func_by_param_signature`; the prefix probe never reaches it.
The earlier qualified-declarator classifier uses emitted/joined-token presence
to decide whether `C::f` is a member. Preserve that classification from the
class field or overload registry before changing linkage tokens.

Candidate rejection: the canonical emitted-token experiment repaired pmf4's
direct path but failed validated publication (native
`test_derived_memberwise_move_assignment` runtime mismatch and full-suite
collision regression mismatch). It was reverted; root CPC remained preserved.

T3 reassessment: inventory entries `defarg4.C` and `array21.C` are absent from
the current retained corpus. The existing runnable `ttp23.C` reproduces its
own mechanism (`qualified member requires an object of its class`) at
`C<::D,int>::f()`: template-template base ownership is lost during qualified
member-call lowering. It is independent of PMF linkage and is next.

Cycle acceptance (2026-09-14, I1 elaborated new type-id): a `new` expression
now marks its type-id while parsing, so `new class A` reuses an existing
complete tag instead of treating the expression terminator as a block-scope
forward declaration. The serial candidate batch compiled `operators34.C` and
passed the new runnable `test_elaborated_new_existing_class.cpp`, ordinary
placement-new, and private-allocation rejection controls. Validated publication
passed all 28 native regressions; Constructors passed 4/4. `operators34.C` is
retired. Next I1 rows are lifecycle/initializer mechanisms, not this tag-lookup
path.

I1 reassessment (2026-09-14): serial root-CPC reproduction partitions the
remaining initializer/lifecycle rows at their first blockers: C++26 aggregate
parsing (`aggr-init1`), GNU compound literals with class elements
(`complit12`), conversion-operator declaration syntax before new construction
(`new33`), unevaluated-new SFINAE (`pr25811-3`), template copy assignment
(`copy1`), inherited assignment/conversion lookup (`pr24623`), and typedef
functional-expression disambiguation (`crash5`). They are independent; the
next I1 candidate is `new33`'s qualified-reference conversion declaration,
with existing conversion-reference controls as its focused baseline.

Cycle acceptance (2026-09-14, I1 implicit-copy conversion): placement new now
recognizes a class argument's viable non-explicit conversion to the destination
class as an implicit-copy path, evaluates that conversion once, and then uses
the existing memberwise copy construction. Candidate validation compiled
`new33.C` and passed qualified-reference conversion positives, the new runnable
`test_implicit_copy_constructor_conversion.cpp`, and the explicit-conversion
negative. Validated publication passed 28 native regressions and
OperatorOverloads passed 4/4. `new33.C` is retired. The remaining I1 rows stay
partitioned; continue with their next independently scoped mechanism.

I1 follow-up reassessment (2026-09-14): fresh candidate reproduction after
`new33` retirement confirms no mergeable residual cluster: `aggr-init1` stops
at constexpr aggregate initialization, `complit12` at GNU class compound
literals, `pr25811-3` at unevaluated-new default-initialization viability,
`copy1` at mutable assignment through const, `pr24623` at inherited assignment
candidate selection, and `crash5` at typedef functional-expression parsing.
Start with `pr25811-3` only after retaining its compile-time positive and
negative controls; do not couple it to conversion construction.

Cycle acceptance (2026-09-14, I1 unevaluated-new viability): default
constructibility now rejects omitted const and reference data members and the
new-expression path consumes that same predicate. Candidate validation passed
`pr25811-3`, scalar and array allocation positives, and invalid-constructor
negatives; the new compile-only member-viability control covers both invalid
members and an ordinary valid member. Publication passed 28 native regressions
and Constructors 5/5. `pr25811-3.C` is retired.

I1 reassessment (2026-09-14, post-pr25811): fresh candidate reproduction
keeps the five remaining rows separate: constexpr aggregate evaluation
(`aggr-init1`), GNU class compound-literal parsing (`complit12`), const/template
copy-assignment suppression (`copy1`), inherited smart-pointer assignment
selection (`pr24623`), and typedef functional-expression disambiguation
(`crash5`). The two assignment rows are not a shared repair: existing
assignment-from-conversion coverage belongs to `pr24623`, while `copy1` must
preserve its const member-template overload and implicit-copy fallback. Start
with `copy1` and its mutable/implicit-copy controls.

Cycle acceptance (2026-09-14, T3 template-template base rebinding): when a
concrete class specialization inherits a template-id whose saved arguments
still name its enclosing parameters, base registration now rebinds those
arguments through the concrete owner before recording the inheritance edge.
The runnable template-template base regression, retained `ttp23.C`, existing
member-template controls, and rejecting `spec18.C` passed in one serial
candidate batch. Validated publication passed 28 native regressions and
Templates passed 41/41. `ttp23.C` is retired. Remaining T3 work is independent
overload conversion ranking (`overload12.C`); T4 PMF linkage remains separate.

Cycle acceptance (2026-09-14, T3 derived-reference ranking): standard
derived-to-base reference conversions now carry their actual inheritance
distance, so the nearer base wins while sibling conversions remain tied. The
retained assemble-only `overload12.C`, runnable direct and conversion-operator
controls, existing reference-ranking coverage, and a rejecting sibling-base
ambiguity control passed in one serial candidate batch. Validated publication
passed 28 native regressions and OperatorOverloads passed 6/6. `overload12.C`
is retired. T3 has no remaining retained row; continue with the independent
T4 PMF linkage collision (`pmf4.C`).

T4 handoff (2026-09-14): published-root reproduction confirms `pmf4.C` still
fails at the source global `B_f`: the lowered non-static member `B::f` uses
the identical joined global token, so the PMF object is parsed as an
incompatible redefinition. This is a canonical member-linkage namespace
collision, not a member-pointer conversion issue. A repair must migrate the
emitted member token and preserve source-name lookup through declaration,
qualified out-of-class definition binding, ordinary calls, virtual dispatch,
and member-address formation as one coherent batch; do not apply a PMF-local
rename.

PMF migration probe (2026-09-14): a canonical non-static member token removes
the `B_f` redefinition, but `C::f` is then lowered without its implicit object
and rejects `this`. The later signature binder is not sufficient: an earlier
scoped-definition classifier still mistakes the raw source-global `C_f` PMF
for a static member. The probe was reverted. The next PMF batch must make that
classifier consult the member declaration/overload registry before any raw
static-token lookup, then reapply the token migration with PMF call controls.

PMF follow-up (2026-09-14): the combined registry-classifier and canonical
token candidate compiles `pmf4.C`, but its runnable PMF dispatch exits with
`0xC0000005`. The linkage collision and out-of-class binding are therefore
resolved while member-pointer virtual receiver/slot lowering is not. Do not
publish this candidate; trace virtual PMF address formation and invocation
against the non-primary `B` vtable next.

PMF runtime isolation (2026-09-14): the candidate's crash occurs already on
the converted `&B::f` call; `&C::f` is not required to reproduce it. The
stored virtual member-pointer path reaches the non-primary virtual dispatch
lowering but does not invoke the override safely. The retained first-party
control must exercise both `&secondary::call` converted to `derived::*` and
`&derived::call`; do not weaken it to a compile-only or one-call check.

I1 assignment/declarator follow-up (2026-09-14): `copy1.C` now passes with a
const-qualified member-template assignment selected before the ordinary
read-only diagnostic, while a mutable same-class assignment still takes the
implicit copy path. The new runnable Classes control covers both outcomes,
the existing copy/conversion controls pass, and a plain const assignment is
still rejected. `crash5.C` also now assembles: local-declaration
disambiguation recognizes a qualified non-static data member as a direct
initializer, with a runnable instance-member control passing. Do not retire
either row yet: the required publishing builds were blocked by an unrelated
candidate compiler access violation during packaged-runtime generation (and,
on the prior attempt, five unrelated template-instantiation regression
compilation failures). Preserve the two focused repairs and resume
publication only after that concurrent compiler instability is resolved.

PMF publication reassessment (2026-09-14): folding a non-virtual base offset
into a virtual PMF's constant adjustment word repairs the actual `pmf4.C`
crash: the prior initializer stored a null PMF because its conversion required
runtime code. The retained row, non-primary virtual PMF regression, virtual-
base conversion control, unrelated-owner rejection, and existing static/member
PMF controls pass with the diagnostic candidate. Publication remains blocked:
the canonical non-static member-token migration independently rejects five
first-party template/constructor regression compiles
(`parenthesized_functional_construction`, `implicit_derived_copy_with_base_constructors`,
`initializer_list_backing_lifetime`, `derived_memberwise_move_assignment`, and
`template_braced_reference_overload`). Reverting the separate qualified-field
direct-initializer probe does not change that cluster. Do not retire `pmf4.C`
or publish until the migration's template/implicit-member lookup path is
traced against the first representative, then rerun this PMF batch.

Cycle acceptance (2026-09-14, T4 PMF linkage and namespace migration):
non-static member emission remains in its private canonical namespace while
namespace qualification now uses the static-name constructor, so qualified
types do not become member symbols. Virtual PMF base-to-derived conversions
fold their fixed receiver offset even when the low bit marks virtual dispatch,
allowing global PMF initializers to remain constant. The retained `pmf4.C`,
five migration regressions, PMF positive controls and unrelated-owner negative
passed in the serial candidate batch; validated publication passed 28 native
regressions and OperatorOverloads passed 7/7. `pmf4.C` is retired. Next:
reassess the I1 copy-assignment/declarator rows independently; the attempted
qualified-field declarator probe remains unaccepted until it no longer changes
functional-expression parsing.

T3 `spec18.C` reassessment (2026-09-14): root CPC reaches the third template
header and rejects `template<class A*, class B>` before any explicit
specialization is parsed (`'>' expected (got '*')`). This is the corpus's only
remaining or first-party occurrence of that obsolete function-template partial
header spelling, so it is independent of ordinary specialization ordering.
Any repair must represent the pointer pattern for deduction and ordering; do
not merely consume `*`, which would turn the pointer candidate into a duplicate
generic overload.

T3 pointer-pattern follow-up (2026-09-14): a retained pointer-pattern bit now
lets `spec18.C` compile and run, and the first-party pointer-versus-generic
selection control passes. Validated publication passed 28 native regressions,
but the Templates broad gate exposed independent failures in
`test_nested_enum_non_type_specialization.cpp` and
`test_template_member_global_operator_new.cpp` (40/42). Keep `spec18.C`
retained until those template regressions are traced and the coherent T3 gate
is clean.

Cycle acceptance (2026-09-14, T3 pointer-pattern and nested specialization
lookup): function-template headers retain a pointer pattern per type parameter,
so deduction compares the pointed argument type without collapsing the pointer
candidate into the generic overload. Object-member static dispatch now consults
the static overload registry even when the instance-member table has no entry;
this makes an explicit nested class-template specialization's static member
callable through an object. `spec18.C` compiled and ran, and the retained
pointer-pattern, nested-specialization, static positive, and rejecting static
signature controls passed in one serial candidate batch. Validated publication
passed 28 native regressions; Templates passed 41/42, with only the independent
global `operator new` row still failing. `spec18.C` is retired with its corpus
manifest and remaining-work row removed. Next cluster: global operator-new
lookup in template members.

Cycle acceptance (2026-09-14, Templates runner header context): the remaining
global-operator-new row was a test-runner include-path failure, not lookup or
allocation semantics: root CPC compiled and ran it with the repository SDK
headers. Its explicit SDK dependency is now declared in the test metadata.
The non-publishing candidate batch passed that row with the nested-specialization
and static positive/negative controls. A second validated publication passed
28 native regressions and Templates passed 42/42. Next retained-work cluster:
reassess the O1 unresolved-sentinel/reachability rows together.

O1 reassessment (2026-09-14): serial root-CPC reproduction partitions the four
unresolved-sentinel rows: `pr71654.C` is unsigned range/bit contradiction
folding, `inline11.C` is GNU-inline constant-only body elimination, and
`comdat1.C`/`attrib6.C` enter catch reachability. The two catch rows are not
one repair: `attrib6.C` first needs `nothrow` preserved from declaration to
definition; `comdat1.C` additionally needs inline/template call-graph
no-throw proof. `cpp_eh_parse_try` currently emits handlers unconditionally.
Start the next candidate with the narrower `attrib6.C` attribute-preservation
path and preserve an actually throwing catch control.

I1 inherited assignment/null conversion probe (2026-09-14): `pr24623.C`
isolates a third assignment mechanism: the assignment fallback was not
carrying the null-pointer marker into converting-constructor resolution. The
marker is now preserved through that probe and the retained compile-only row
passes. A focused first-party null-converting-assignment runtime regression
was added, but it cannot be executed until the failed publishing attempt's
candidate-runtime state is repaired (`build/compiler/cpc.exe` currently
reports missing `libcprime1.a`); this is the same unpublished candidate state,
not a row failure. Do not retire `pr24623.C` before a clean candidate batch
and normal publication.

Cycle acceptance (2026-09-14, I1 assignment and direct initialization):
assignment overload resolution now gives a const-qualified member-template a
chance before rejecting a const receiver, while mutable same-class assignment
continues through implicit copy; null-pointer arguments retain their marker
through the assignment converting-constructor fallback. Local declarator
disambiguation also recognizes a qualified non-static member expression as a
direct initializer. Retained `copy1.C`, `pr24623.C`, and assemble-only
`crash5.C` passed together with the focused runnable copy/null/qualified-member
controls and a rejecting const-assignment control. Validated publication passed
the 28-test native regression gate and Classes passed 11/11. All three rows
are retired. The remaining I1 rows are independent constexpr aggregate
evaluation (`aggr-init1`) and GNU class compound literals (`complit12`); take
the larger language mechanism only after fresh reproduction.

Cycle acceptance (2026-09-14, I1 GNU class compound literals): typed class
and class-array compound literals now use the normal destination-aware braced
construction path, and saved array-list splitting unwraps their type wrapper.
This preserves constructor/destructor counts rather than materializing an
extra class temporary. Retained `complit12.C` compiled and ran successfully;
GNU vector compound-literal and functional-construction positives passed, and
the independent constexpr aggregate row still rejects as expected. Validated
publication passed 28 native regressions and Expressions passed 2/2.
`complit12.C` is retired. The sole remaining I1 row is `aggr-init1.C`'s
constexpr aggregate/default-member evaluation.

Cycle acceptance (2026-09-14, O1 no-throw catch reachability): GNU
`__attribute__((nothrow))` now records the existing no-throw function flag.
Backend calls to no-throw functions do not establish an EH state, and a try
body with neither a potentially throwing call nor a direct throw parses its
handlers without emitting or linking them. Direct throws remain tracked, so
throwing catch handlers remain live. Retained `attrib6.C` and `comdat1.C`, a
runnable GNU-nothrow positive, a throwing-catch unresolved-link negative, and
the typed-catch runtime control passed in one serial candidate batch.
Validated publication passed 28 native regressions and Exceptions passed 3/3.
Both corpus rows are retired with their manifest and worklist entries removed.
Next O1 reassessment: `pr71654.C` unsigned contradiction versus `inline11.C`
GNU-inline constant-body reachability are independent mechanisms.

O1 `pr71654.C` reassessment (2026-09-14): the outer unsigned comparison is
lowered to CPU flags and consumed before its nested body is parsed; the local
`j` retains no alias or range relation to `i0`. Eliminating the nested bit-test
call therefore requires a general CFG value-range/alias analysis, not a safe
local expression rewrite. This is independent of `inline11.C`'s GNU-inline
constant-body reachability and the repaired EH no-throw path. Next shared
candidate: reassess the remaining T4 virtual/member-pointer rows together.

T4 reassessment (2026-09-14): `ptrmem2.C` now first rejects construction of
`D` for missing implicit default-constructor viability, before pointer-to-
member conversion; `pr60640-3.C` independently reaches covariant virtual
receiver lookup and rejects `A::foo` through the adjusted local class. Do not
batch these under member-pointer representation. Next smallest repairable
cluster must be selected from the remaining constructor/lifecycle rows or the
single covariant-virtual row after focused reproduction.

Cycle acceptance (2026-09-14, T4 qualified virtual member address): qualified
class-member lookup now uses the resolved class scope, and an address-forming
expression preserves a member-function spelling even when an alias record has
the same name. This restores `&D::f` as a contextual base member-pointer
address before its derived conversion. The retained runnable `ptrmem2.C`, the
cast-bound member-call regression, non-primary virtual-base and inheritance
ranking positives, an unrelated-owner negative, and virtual-constructor
controls passed in one serial candidate batch. Validated publication passed
28 native regressions; OperatorOverloads passed 8/8 and Constructors 6/6.
`ptrmem2.C` is retired. Next T4 row is the independent covariant virtual
receiver path in `pr60640-3.C`. The concurrent aggregate-initializer replay
edit was corrected to pass its source length to `tok_str_add_record`; the
subsequent validated root publication again passed all 28 native regressions.
