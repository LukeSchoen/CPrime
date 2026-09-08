# Fast test policy

Each compiler invocation and test executable must finish in **less than five
seconds**, including process startup. `run.ps1` defaults to five seconds and
rejects larger budgets. Smaller budgets are supported for tight loops and runner
fixtures. Reaching the limit is a failure even if the process returns zero.
Cleanup has separate bounded waits and separate timing; it is never an unlimited
extension of the execution budget.

The runner stops on the first timeout, saves the command and diagnostics in
`results.jsonl`, records the repair item in `slow-failures.json`, and reports
unexecuted cases in `execution.json`. `-ContinueAfterTimeout` permits a serial
inventory sweep with the same hard per-process limit. Neither mode retries slow
tests or increases their budget. Runtime timeouts and crashes have distinct
failure statuses. Empty and probe-only runs cannot pass the checked gate.

Treat unexplained slowness as a CPC defect. Minimize the source, add a fast
regression, improve the algorithm or generated program, and rerun the original
case and surrounding tests. Do not delete a case merely because CPC times out.

To collect initial workload evidence, run `triage-speed.ps1 -Source <file>`.
It compiles with CPC using the five-second ceiling, writing `comparison.json`
under `build/`. Pass `-BaselineCompiler <old-cpc.exe>` to compare two CPC builds
serially with identical source and options. No other compiler is invoked.
This is a single-sample triage measurement, not a conformance comparison or a
statistically stable benchmark. Inspect target defaults, diagnostics, and any
unsupported options before interpreting it.

When source inspection and measurements establish that a test deliberately
requires slow work (for example a timed sleep or a large benchmark), remove the
test from the fast suite and record its source identity, workload reason,
commands, timings, and lost coverage in a reviewed change. Prefer a smaller test
of the same rule. For imported GCC tests, preserve the pinned third-party source
and implement an explicit, evidence-backed inventory exclusion; report it as
removed coverage, never as a pass. No such exclusion is currently justified or
installed. Reference slowness alone is insufficient evidence.

This policy is enforced by the GCC runner, speed triage tool, and local language
runner `Tests/run.ps1`. Standalone runners still require equivalent integration.
For an unpackaged compiler built under `build/`, supply `-RuntimeRoot` pointing
to the same runtime/header installation used by the baseline compiler.
