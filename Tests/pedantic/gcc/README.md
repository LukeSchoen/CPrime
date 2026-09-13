# Retained GCC compatibility work

## Scope and evidence

5 unresolved rows remain at upstream revision
`5f6257c26b814de1a14c71b2d3a49291765b6577`. The 2026-09-13 root-CPC audit
reported compile/link failure for every row, with no timeout. Compiler identity
and exact commands are in `build/gcc-preparation-audit/metadata.json` and
`results.jsonl`; task.md records the compiler hash. The manifest hashes every
retained source and its required support header. Passing behavior must move to
minimal first-party coverage before deleting a repaired external row.

C++17 is the cutoff. `g++.dg/coroutines/pr113457.C` is excluded for concepts,
ranges and coroutine requirements; do not resume its old repair queue.
Old GNU syntax and optimizer link sentinels are labelled
separately below; acceptance is not a claim of standard conformance.

## Package mechanisms

| ID | Mechanism and likely implementation | Package validation |
| --- | --- | --- |
| T1 | Declaration ownership, specialization identity and linkage: cprimegen_templates.inc, function_specializations.inc, microsoft_mangle.inc. Register the owner and declaration before replay; canonicalize once. | Member templates under non-template owners, friend template-ids, enum arguments, declarations/definitions and separate-TU symbol identity. |
| T2 | Dependent scope and completion: templates, substitution, alias_templates, cpp_member_info. Keep lexical lookup separate from receiver lookup; materialize layout only when semantically required. | Alias/class chains, explicit-instantiation parameter scope, free names inside members, deferred nested layout; include access rejection controls. |
| T3 | Deduction, defaults and ordering: template_ordering, substitution, cpp_names_overload. Substitute explicit arguments before deduction and preserve candidate order/ownership. | Runtime selected-overload values, all argument spellings, ambiguity controls, SFINAE versus hard errors. |
| T4 | Member-pointer identity/calls: member_pointers, microsoft_mangle, cpp_member_info and backend receiver adjustment. Use a consistent owner/type representation through substitution and emission. | Non-primary base, virtual dispatch, member-template addresses, NTTPs and callable results; compile-only success is insufficient for runnable originals. |
| I1 | Initialization and lifecycle: initializers, constexpr, lifecycle, temporaries. Separate parsing, viability, constant materialization and runtime cleanup. | Static assertions plus runtime values/counts, new/class conversions, mutable assignment, compound-literal lifetime and the pending reductions in task.md. |
| E1 | Function-try-block parsing and cleanup: statements, exceptions, lifecycle. Save the function scope and constructed-subobject state for handlers. | Constructor base destruction before handler, ordinary member handler scope and propagation. |
| X1 | GNU/C compatibility: cprimegen.c, initializers, cprimeasm.c, cprimepp.c and linker symbol naming. Keep extensions explicit; do not weaken standard diagnostics globally. | Generic atomic size, asm tied operands/names, VLA scope, alignment and system-header-only permissive behavior. |
| O1 | Required optimization/linkage: frontend reachability, exception attributes and x86_64-fastopt.inc. Optimize proven unreachable calls while preserving side effects and inline linkage. | Keep undefined sentinel functions undefined; both the original link check and a local semantic opposite must behave correctly. |

These are investigation plans based on source and first diagnostics, not claims
that every row has a proven common root cause. Process T1 before dependent T2/T3
and T4 fixes; initialization and exception work then feed O1. Reassess all rows
in the affected package after a coherent repair, rather than starting another
one-row cycle. The general order and non-GCC issues are in [task.md](../../../task.md).

## Complete retained inventory

All rows currently have status FAIL_COMPILE (the adapter uses this label for
link failures too). Paths are relative to corpus/. The error column is the
first observed blocker; the planned check includes behavior beyond that blocker.

| Row | Package | Current blocker | Planned repair and proof |
| --- | --- | --- | --- |
| `c-c++-common/pr71654.c` | O1 | undefined symbol 'foo' | Fold the proven unsigned-byte condition before emitting a reference to foo; retain the undefined sentinel. |
| `g++.dg/eh/dtor1.C` | E1 | function definition expected | Lower an out-of-class destructor function-try-block without invalid destructor cleanup state; observe base destruction before handler entry. |

## Commands and retirement

Run an exact row with
`powershell -NoProfile -ExecutionPolicy Bypass -File Tests/pedantic/gcc/run.ps1 -Select <path> -Out build/gcc-package`.
Prefix selections may select a package directory; unknown selections fail.
Use root CPC only and one compiler process at a time. Each compile/program has
a five-second ceiling; crashes/timeouts fail without retries. The adapter uses
CPC's default language mode, so content inspection/reduction is required for
the C++17 cutoff; the runner does not enforce a standard-version matrix.

The retained suite deliberately exits nonzero while positive cases fail; it is
not an expected-failure green gate. Fast selects all retained rows, pedantic
selects none, and `-Tier all` selects the complete manifest. Before final
retirement, fix empty-manifest handling in corpus.ps1 and run.ps1; both currently
reject an empty corpus instead of reporting a valid zero-case assessment.
Implement new/replacement tooling in native C under src/, built by root CPC;
avoid expanding the PowerShell runner implementation.

For each repaired row, preserve its distinct observable behavior in a combined
first-party test where flags and scope permit, verify original and reduction,
then delete the source and manifest entry. Remove unused support entries only
after checking remaining include dependencies. Keep auxiliary multi-TU cases
open until their complete linkage scenario is represented locally.

Corpus/tier edits require `test_corpus.ps1` and discovery checks. Harness
changes also need runner, assessment, provenance and progress fixture checks.
Raw logs remain in build/; this file contains only current decisions and work.
