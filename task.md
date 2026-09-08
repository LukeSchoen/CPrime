# CPrime: work required for 100% test success

Assessment date: 2026-09-08. This is an implementation backlog, not a claim that the compiler already passes. The requested investigation and planning are complete when the baseline and tasks below are recorded; compiler implementation remains future work.

## What was downloaded and what success means

The imported tests are from GCC, managed by `Tests/gcc/run.ps1`, rather than a Clang compatibility suite. The checkout is `build/gcc-upstream`, pinned to revision `5f6257c26b814de1a14c71b2d3a49291765b6577`. It contains `g++.dg`, `g++.old-deja`, `c-c++-common`, and test support. Bundled Clang is also available as a compiler used to build CPC.

There are 26,451 inventoried source files, not 26,451 independently verified test cases. The current adapter uses CPC's default C++ mode, inventories helper sources, and cannot yet check many upstream expectations. A successful object compilation does not establish diagnostic, standard-version, ABI, or runtime correctness.

- **First milestone:** every currently checked, applicable case passes, with no compiler crashes or unexplained timeouts. This is a restricted adapter milestone.
- **Final compatibility milestone:** every applicable test in a declared target/language/option matrix has its actual expectations checked and passes; no applicable case remains an unverified probe or unsupported harness feature. All local tests also pass.
- **Literal entire GCC inventory:** includes GCC-specific internals, extensions, specialized drivers, and other target assumptions. Passing this literally requires implementing those capabilities or providing the necessary target environments. Any justified non-applicable cases must remain separately counted with evidence; they are never passes. Do not silently narrow the goal to obtain 100%.

Invalid-program tests must reject for the expected reason. `PROBE_ACCEPTED`, `PROBE_REJECTED`, `UNSUPPORTED`, crashes, and timeouts must never be included in the pass numerator. Passing this finite suite is also not proof of complete ISO C++ conformance.

## Evidence and baseline

Fresh results are in `build/gcc-task-baseline/{metadata.json,summary.json,results.jsonl}`. The run uses root `cpc.exe`, default language mode, no probes, and a five-second limit separately for compilation and execution. Compilation is serial, one compiler process at a time. No compiler rebuild was performed for this assessment.

Compiler SHA-256: `ea36f1ad84e019cefc809eb1e2c3eb30417c7fac33df0ee14006c6b53491e7bd`. Repository HEAD: `c067b5e`; the working tree was clean before this document was added. The runner completed with exit code 1 because checked cases failed.

| Status | All three source trees | C++ trees only (`g++.dg`, `g++.old-deja`) |
| --- | ---: | ---: |
| PASS_COMPILE | 1,329 | 1,293 |
| PASS_ASSEMBLE | 816 | 816 |
| PASS_LINK | 69 | 69 |
| PASS_RUN | 580 | 570 |
| PASS_PREPROCESS | 7 | 1 |
| FAIL_COMPILE (includes link failures) | 1,342 | 1,313 |
| FAIL_RUN (includes runtime crashes/timeouts) | 108 | 105 |
| Compiler CRASH | 6 | 6 |
| Compiler TIMEOUT | 3 | 3 |
| UNSUPPORTED, not executed | 22,191 | 18,859 |
| Total inventoried sources | 26,451 | 23,035 |

There are **2,801 passes out of 4,260 executed checks (65.75%)**, with only **16.11% of inventoried sources executed**. These are adapter measurements, not a C++ conformance percentage. The 1,459 non-passing executed checks still need triage for compiler defects versus driver, helper-source, or target assumptions.

Largest failing directory groups, counting compile/run failures, compiler crashes, and timeouts: `g++.dg/template` 333; `g++.old-deja/g++.pt` 203; `g++.dg/ext` 95; `g++.old-deja/g++.other` 88; `g++.dg/lookup` 71; `g++.dg/init` 55. Templates and lookup are therefore a high-value starting area after harness validation and crash triage. These counts are failing files, not independent root causes.

The six compiler crash cases are `g++.dg/eh/ctor3.C`, `g++.dg/lookup/koenig12.C`, `g++.dg/template/deduce2.C`, `g++.dg/template/friend7.C`, `g++.old-deja/g++.pt/explicit78.C`, and `g++.old-deja/g++.pt/expr1.C`. The three compile timeouts are `g++.dg/eh/cleanup1.C`, `g++.dg/opt/pr117439.C`, and `g++.dg/other/pr117516.C`; a five-second timeout alone does not establish an infinite loop.

`Tests/gcc/test_runner.ps1` passed its directive-classifier and process tests during this investigation. A fresh `features/Classes` local-suite run also passed **163/163**, recorded in `build/gcc-task-baseline/local-classes.log`. The complete local suite and full probe survey were not rerun for this planning task.

Historical context: `Tests/gcc/results-2026-09-08.json` records an earlier full probe survey. Its post-fix all-source totals include 148 compiler crashes, 5 timeouts, 1,450 compile failures, and 109 runtime failures. Its 9,694 accepted and 12,034 rejected probes were **not verified passes**. It also records 1,164 local tests passing on each of two compiler hosts and 23 repaired GCC cases. Those figures refer to older binaries and must not be presented as today's full local-suite result or directly compared with this non-probe run.

## Ordered implementation backlog

### Resumed verification (2026-09-08)

`build/gcc-checked-resume` is a fresh, completed run of all 4,260 original
checked paths against `build/gcc-fast-initial/cpc-work.exe`, using the explicit
portable runtime and unchanged five-second process deadline. Its comparison
with `build/gcc-task-baseline` records **3,001/4,260 passes (70.45%)**, **200 new
passes**, **zero regressions**, and **zero missing cases**. There were no compiler
crashes or compiler/runtime timeouts. Remaining failures are 1,146 compile/link
failures, 65 runtime crashes, and 48 other runtime failures. This confirms the
two later performance fixes in a complete comparison; compatibility remains
incomplete.

The GCC runner now writes a content-hashed `inputs.json` covering selected
sources and the explicit runtime's `include/` and `lib/` files. Metadata includes
the manifest and provenance-helper hashes, language, and unresolved implicit
runtime status. Target and standard-version matrix provenance remain unfinished.
`test_provenance.ps1`, existing runner tests, and comparison tests pass. An
end-to-end run in `build/gcc-provenance-check` passes `pr117516.C` and validates
the manifest hash and nonempty runtime/header identity. No compiler source was
changed during this resumed verification.

### Implementation progress (2026-09-08, stopped at user request)

Work is paused at the user's explicit stop request. The 90% target and the full
backlog remain incomplete. No slow tests were deleted, excluded, or relabeled
as expected failures. All development builds and validation used CPC after the
user's instruction; compilation remained serial. Root `cpc.exe` and `lib/` were
preserved. The root compiler still has the baseline SHA-256 recorded above.

**Latest completed full comparison:** `build/gcc-checked-progress-3` contains
**2,999/4,260 passes (70.40%)**, versus **2,801/4,260 (65.75%)** at baseline:
**198 new passes, zero regressions, zero missing cases, and zero compiler crashes**.
These are restricted adapter results, not a C++ conformance percentage.
The remaining statuses in that completed run are 1,146 compile failures,
65 runtime crashes, 48 other runtime failures, and two compiler timeouts.
`comparison.json` preserves the fixed baseline denominator.

**Later validated performance fixes:** both remaining timeout reproducers now
compile successfully in focused runs, without raising the five-second limit:

- `g++.dg/other/pr117516.C`: 0.0446 seconds, recorded in
  `build/gcc-trivial-graph4/results.jsonl`. Cache completed types with trivial
  construction/destruction and skip repeated member-lifetime traversal.
- `g++.dg/opt/pr117439.C`: 0.0451 seconds, recorded in
  `build/gcc-array-loop/results.jsonl`. Generate a runtime constructor loop for
  trivially destructible member arrays instead of unrolling millions of elements.
  Nontrivial element destruction retains the existing unwind handling.
- These two new passes are **not folded into the completed full-run percentage**.
  The follow-up full run, `build/gcc-checked-progress-4`, was terminated at the
  user's request after **1,702 recorded cases**, leaving **2,558 unexecuted**.
  `interrupted.json` records this explicitly; this directory is not a full result.

**Implemented compiler fixes:** pointer/scalar conversion operators, conditional
conversion, class-copy initialization through conversion operators, overloaded
comma expressions, GNU `__null`, explicit template instantiation emission,
base-class using declarations, qualified nested-class definitions, implicit
member construction and exception cleanup, lazy self-template identity,
constant-expression template arguments, and typedefs naming other specializations.
All six baseline compiler-crash reproducers have passed their individual checks.
Remaining language and runtime failures still require work.

**Fast-test framework:** GCC and local language runners enforce a maximum
five-second process deadline, bounded output cleanup, and child-process tree
termination. Timeouts and incomplete capture fail. Empty, unsupported-only,
and probe-only gates fail. Exact-file selection avoids full-tree scans in focused
loops. `Tests/gcc/compare.ps1` rejects regressions and removed checks, reports
promoted coverage separately, and never improves the score by dropping cases.
`Tests/gcc/triage-speed.ps1` compares CPC builds serially with recorded commands,
hashes, and timings. Standalone runners still need broader deadline integration.

**Latest focused local validations:** Templates 463/463, Constructors 129/129,
Classes 167/167, OperatorOverloads 74/74, Exceptions 37/37, Destructors 22/22,
and c_compat 23/23. These were completed around their relevant changes, not as
one exhaustive repository gate. GCC runner, comparison, local runner fixture,
and linker-map tests also passed. New lifetime performance regressions exercise
a shared nested-member graph and a 32,768-element array's constructor count/order.

**Resume here:** rerun the original 4,260 checked paths against the current
compiler into a fresh results directory, then compare with `build/gcc-task-baseline`.
Do not resume by treating the interrupted run as complete. The current development
binary is `build/gcc-fast-initial/cpc-work.exe`; runners need
`-RuntimeRoot C:/Users/Luke/AppData/Local/cpc/1.4` to use the same portable headers
and runtime as baseline. Native debugging uses the CPC-built
`Tests/tools/trace_compile.c` and `-Wl,-Map=<path>`; the tracer now captures a bounded
timeout stack as well as crash stacks. Latest source edits concern lifetime
caching, member-array loops, their two regressions, and timeout tracing.
Continue with the remaining compile/runtime failure groups after confirming that
these final performance changes preserve the full baseline's existing passes.

### 1. Establish a trustworthy test inventory and gate

- [ ] Add a complete serial test entry point under `Tests/` that discovers all local language suites and explicitly registers standalone ABI, runtime, packaging, driver, and runner tests. `tests.cmd` currently calls only a subset; it is not an exhaustive repository gate.
- [ ] Record source commit and working-tree state, compiler hash, runtime/header identity, target, language mode, options, timeout, selected paths, runner version, and concurrency. Timeout, concurrency, runner hashes, source state, and runtime path are now recorded; complete runtime/header identity and target/language-matrix provenance remain to be finished.
- [x] Preserve per-case commands, diagnostics, run output, and timings in `build/`; add machine-readable comparison reports for new passes, regressions, crashes, timeouts, and coverage changes.
- [ ] Build a case inventory from upstream driver semantics: distinguish independent tests from helpers and group multi-source/link tests correctly. For example, the current scan attempts `g++.dg/DRs/dr2387-aux.cc` independently, whereas `g++.dg/dg.exp` discovers `.C` tests.
- [ ] Track compiler failure, link failure, wrong runtime result, runtime crash, and runtime timeout separately. Runtime crashes and timeouts now have distinct statuses; link errors still need separation from `FAIL_COMPILE`.
- [ ] Publish two measurements: verified pass rate among applicable cases and expectation-checking coverage. Adding unsupported cases or changing scope must not improve the reported score without an explicit explanation.

Completion: reproducible per-case inventory, stable denominators, and a gate that cannot report success merely because it executed no relevant checks.

### 2. Expand the GCC adapter so the hidden backlog becomes measurable

Primary files: `Tests/gcc/assessment.ps1`, `Tests/gcc/run.ps1`, and `Tests/gcc/test_runner.ps1`.

- [ ] Implement target/effective-target conditions, language-version selectors, conditional actions/options, and genuine target exclusions. Missing CPC language features remain implementation gaps, not convenient target exclusions.
- [ ] Honor the upstream standard and option matrix, including defaults established in the `.exp` drivers. Merely accepting or dropping `-std=` is insufficient.
- [ ] Check `dg-error`, warnings, notes/messages, forbidden diagnostics, locations, and excess diagnostics. Define and test any diagnostic-pattern adaptation explicitly; a generic compiler error or crash is not a successful negative test.
- [ ] Implement additional sources, include/support paths, link settings, expected runtime output, and expected abnormal termination where specified.
- [ ] Audit specialized drivers and support their test grouping and configuration. Do not infer all required settings from directives in an isolated source file.
- [ ] Preserve the distinction between preprocess, compilation to assembly, assembly to object, linking, and running. Current `PASS_COMPILE` only checks object production; preprocessing currently does not compare expected output unless an expectation is implemented separately.
- [ ] Inventory assembly scans, tree dumps, GCC ABI tests, plugins, OpenMP/offload, and other compiler-specific requirements. Implement those needed for the declared goal and separately document genuinely non-applicable target/compiler-internal checks without claiming them as passed.
- [ ] Add adapter regressions for each new directive, nested/conditional syntax, malformed input, stale artifacts, missing dependencies, and process cleanup before promoting unsupported cases to checked cases.

Completion: zero unimplemented harness expectations for applicable cases. Until then, full compatibility progress cannot be quantified by the checked pass rate alone.

### 3. Eliminate compiler crashes and pathological compile times

- [ ] Reproduce every crash from the fresh run and refresh the full `-Probe` robustness survey; today's ordinary run does not exercise unsupported cases.
- [x] Repair `g++.dg/eh/ctor3.C` and `g++.dg/lookup/koenig12.C`; both pass, as do the other four baseline compiler-crash reproducers.
- [ ] Reduce failures into general-purpose local regressions. Investigate error-path cleanup, symbol/type lifetime, recursive instantiation, and worklist termination; confirm causes before selecting a fix.
- [ ] Triage timeout cases within a hard five-second ceiling using `Tests/gcc/triage-speed.ps1`; do not raise the limit. Treat unexplained slowness as a CPC defect. Remove a case from the fast suite only with evidence that its intended workload requires slow execution, recording the coverage loss. See `Tests/gcc/FAST_TESTS.md`. Start with `g++.dg/eh/cleanup1.C`, a large constructor-cleanup case.
- [ ] Improve algorithms, caching, and data handling where needed. Do not solve compiler latency by adding compilation threads or simply raising the baseline timeout.

Completion: no crashes on valid or invalid inputs; bounded completion on stress inputs; no unexplained compiler or runtime timeouts in checked tests or the probe survey.

### 4. Repair core C++ semantics in coherent feature groups

The examples below are observed failures or investigation leads, not established root-cause diagnoses. Group all failures by minimized cause before counting independent bugs.

| Work area | Starting evidence | Likely implementation and surrounding tests |
| --- | --- | --- |
| Lookup, scopes, ADL, dependent names | `g++.dg/lookup/anon3.C`, `anon4.C`, `koenig12.C` | `cprimegen.c`, `cprimegen_cpp_names_overload.inc`, template files; Namespaces, Classes, Templates |
| Templates, deduction, substitution, specialization and ordering | Template-related failures in lookup/parse; `g++.dg/parse/access1.C` | `cprimegen_templates.inc`, `cprimegen_substitution.inc`, `cprimegen_template_ordering.inc`, alias/function-specialization files; Templates |
| User-defined conversions, references, overloads, member pointers | `g++.dg/conversion/op6.C` rejects a valid class-to-pointer conversion used by `delete`; `cast1.C`, `cond6.C` also fail | Names/overload, member-pointer and expression handling; OperatorOverloads, References, Expressions |
| Initialization, aggregates, special members, object lifetime | `g++.dg/init/aggr14.C`; `aggr7-eh.C` and `aggr7-eh2.C` have unresolved constructor symbols | Initializers, lifecycle, temporaries; Constructors, Destructors, InlineLifecycle |
| Inheritance, virtual dispatch, covariant results, RTTI | `g++.dg/abi/covariant3.C`, `covariant4.C`, `covariant5.C` compile but fail at runtime; `covariant4.C` access-violates | Class layout/dispatch, RTTI, Microsoft mangling, backend; Classes and native ABI fixtures |
| Exception semantics and cleanup | `g++.dg/eh/ctor3.C`, `cleanup1.C`, aggregate exception cases above | `cprimegen_exceptions.inc`, lifecycle and `src/runtime/`; Exceptions and `test_RunExceptions.ps1` |
| Parsing, declarations, constant evaluation | `g++.dg/parse/access12.C`, `ambig2.C`; constant-initializer failures | Parser, constexpr, local types; Declarations, Expressions, Statements, Templates |
| GNU builtins, attributes, preprocessing and extensions | `c-c++-common/cpp/has-builtin-2.c`, `has-builtin-4.c`, `g++.dg/ext/align2.C`, `c-c++-common/asmgoto-1.c` | `cprimepp.c`, parser and backend; Includes, C compatibility, extension regressions |

- [ ] For each group, reproduce, reduce, add meaningful positive and negative coverage, fix the underlying rule, and re-run the original upstream cases plus relevant local suites.
- [ ] Audit generated code for all runtime failures, including optimization-sensitive cases. Compile acceptance is only the first step.
- [ ] Validate portable semantics separately from target ABI assumptions. For example, missing `cxxabi.h` in `g++.dg/abi/rtti2.C` needs a GNU-runtime applicability decision, while the covariant-call cases test meaningful C++ behavior on Windows too.
- [ ] Retest fixes in appropriate optimization modes and with both a native-host-built CPC and a CPC self-hosted compiler.

Completion: every applicable checked failure is resolved by a compiler/runtime fix or a demonstrated harness correction, with per-case evidence and no regressions.

### 5. Complete language modes, newer C++, and runtime/header coverage

- [ ] Declare and implement the required C++ standard versions, version macros, feature-test macros, extension policy, and strict diagnostic behavior. Test that rules actually change with the selected mode.
- [ ] Once selectors and diagnostics work, inventory missing capabilities by language generation: constexpr rules, deduction/CTAD, folds, structured bindings, concepts/requires, coroutines, modules, and later-standard features present at this GCC pin. These are audit categories, not a claim that every feature is entirely absent.
- [ ] Address the coroutine gap demonstrated by `g++.dg/coroutines/coro-function-decl.C` (missing `<coroutine>`) and related coroutine feature/configuration failures; header presence alone will not supply lowering and runtime semantics.
- [ ] Inventory required standard headers and runtime behavior, then implement missing portable facilities in `include/` and `src/runtime/`. Keep GNU-specific runtime APIs distinct from CPC's Windows ABI requirements.
- [ ] Retain C compatibility while modifying shared parser, preprocessor, code-generation, and runtime paths. Shared GCC C/C++ cases in this assessment were compiled as C++, not independently validated as C.

Completion: all declared modes have executable conformance checks; missing applicable features no longer hide behind unverified options or probes.

### 6. Make 100% reproducible and keep it green

- [ ] Run all local language suites, including those omitted by `tests.cmd`, without relying on stale shared binaries.
- [ ] Run standalone runner/manifest fixtures, include search, batch/incremental build, code-generation checks, exception/object-link tests, native COFF/TLS/MSVC ABI tests, UCRT interoperability/module exit, and portable packaging checks. Record required native toolchains as test prerequisites.
- [ ] Build CPC with the native host compiler and self-host serially into `build/`; validate both against identical inputs and settings. Preserve root `cpc.exe` and `lib/` bootstrap/portable inputs during development.
- [ ] Re-run the full pinned GCC matrix with all applicable expectations checked, plus the full crash/timeout survey. Review per-case differences, not just aggregate counts.
- [ ] Save a final report with zero applicable failures, zero applicable unsupported/unverified cases, zero crashes/unexplained timeouts, explicit non-applicable counts and reasons, and 100% local-suite success for each tested compiler host.
- [ ] Add the complete serial gate to ongoing validation, with focused suites for each change and the complete matrix for release acceptance.

## Commands for the next implementation session

Run these separately and serially from the repository root. Use a new output directory for each comparison run.

```powershell
# Adapter self-tests.
powershell -NoProfile -ExecutionPolicy Bypass -File Tests/gcc/test_runner.ps1

# Reproduce today's checked inventory (an exit of 1 is expected while failures remain).
powershell -NoProfile -ExecutionPolicy Bypass -File Tests/gcc/run.ps1 -Compiler ./cpc.exe -Timeout 5 -Out build/gcc-next-checked

# Full robustness survey; probe outcomes are not verified passes.
powershell -NoProfile -ExecutionPolicy Bypass -File Tests/gcc/run.ps1 -Compiler ./cpc.exe -Probe -Timeout 5 -Out build/gcc-next-probe

# Small semantic/robustness starting set, invoked inside PowerShell for array arguments.
./Tests/gcc/run.ps1 -Compiler ./cpc.exe -Select 'g++.dg/lookup/koenig12.C', 'g++.dg/eh/ctor3.C', 'g++.dg/conversion/op6.C', 'g++.dg/abi/covariant4.C' -Timeout 5 -Out build/gcc-first-fixes

# Example surrounding local suite; the final gate must discover and run all suites.
powershell -NoProfile -ExecutionPolicy Bypass -File Tests/run.ps1 -Suite features/Classes -CompilerPath ./cpc.exe
```

Keep first-party implementation in C/C++/assembly, batch, or PowerShell. Keep tests and benchmark inputs under `Tests/`, build scripts under `scripts/`, and generated evidence under `build/`. Leave upstream sources unchanged, avoid application-specific dependencies and name-specific hacks, revert failed experiments, and never relabel active failures as expected failures to improve totals.
