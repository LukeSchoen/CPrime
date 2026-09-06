# Agent Instructions

## Serial compiler execution

- Cprime compilation and Cprime project builds must run on one thread, with
  one compiler process at a time. Do not enable compiler threads, parallel
  translation-unit builds, worker pools, or concurrent compiler invocations.
- This rule applies especially when the user asks to make Cprime faster.
  Optimize the compiler's algorithms, data structures, lookup, parsing,
  instantiation, and memory use. Do not substitute parallel execution for
  improving how the compiler handles its data.
- Keep Cprime serial in performance comparisons. MSVC may use threaded and
  parallel builds, including /MP and parallel MSBuild workers. The serial
  restriction applies to Cprime, not MSVC.
- Use the same source inputs and record each compiler's settings and concurrency.
  Report compiler work separately from build-driver overhead. Do not re-enable
  Cprime parallel compilation without an explicit user instruction to do so.
- This concerns execution of the compiler and its build tools; it does not
  remove C++ language/runtime support for programs that use threads.

When working on the cpc compiler, stay with the task for as long as useful
progress is possible. Prefer sustained, careful investigation and implementation
over stopping early. Keep working through diagnosis, reproduction, fixing, and
verification unless the task is complete or there is a concrete blocker that
requires user input.

Compiler compatibility work is an extended implementation task, not a short
blocker-reporting exercise. Do not stop merely because one failure was moved to
the next failure, because an initial fix became complicated, or because a few
minutes have elapsed. Continue for a substantial work session while diagnosis,
implementation, testing, reduction, or project-build progress remains possible.

The requested deliverable is forward progress in coherent, reviewable changes:

- Prefer a substantial, integrated fix and its regression coverage over small,
  speculative edits that only move diagnostics around.
- Do not leave experimental, name-specific, or knowingly incomplete compiler
  hacks in the working tree as if they were progress.
- Do not convert a newly added reproducer into an expected-failure test merely
  to make the test layout look clean. A reproducer added for the active task
  must be fixed and promoted to passing coverage before the work is presented
  as complete, unless a genuine external blocker requires user input.
- Revert failed experiments before pursuing the next approach. Keep the
  working tree buildable and make each retained change defensible.
- Exercise the real measurement project repeatedly. Passing a synthetic test
  is necessary but is not sufficient when Racer is the stated goal.
- When asked for a testable Racer build, do not hand back a progress report as
  the deliverable. Continue until the Racer executable is actually produced
  and the relevant compiler tests pass, or until a concrete blocker makes
  further work impossible without user input.
- Once a coherent wave is verified, preserve it as a meaningful commit-sized
  unit with the implementation, regression tests, and task notes together.
  Avoid a trail of tiny checkpoint-style or diagnostic-only changes.

Follow this workflow:

1. Check whether each reported symptom is a real issue.
2. Add tests that reproduce the limitation or bug.
3. If the issue reproduces, fix it.
4. Verify the fix against the tests.
5. Continue into related failing cases when they are part of the same compiler
   behavior, and keep iterating until the issue is genuinely handled.
6. Avoid unnecessary churn, but do the hard work needed to reach a complete,
   tested result.
7. Rebuild CPC, run the focused regression tests, run relevant surrounding test
   suites, copy the verified compiler into the measurement project, and run the
   real Racer build through compile and link.
8. Before saying a build is ready to test, verify that the expected Racer
   executable exists and report its exact path. Warnings or known limitations
   that could affect runtime behavior must be stated explicitly.
