# GCC C++ assessment

The serial adapter fetches unchanged upstream sources from
https://github.com/gcc-mirror/gcc at revision
`5f6257c26b814de1a14c71b2d3a49291765b6577` into `build/gcc-upstream`.
It inventories `g++.dg`, `g++.old-deja`, and `c-c++-common`.

```powershell
./Tests/gcc/run.ps1 -Fetch -Out build/gcc-baseline
./Tests/gcc/run.ps1 -Select g++.dg/init/ -Out build/gcc-init
./Tests/gcc/run.ps1 -Baseline build/gcc-baseline -Out build/gcc-current
./Tests/gcc/compare.ps1 -Baseline build/gcc-baseline -Current build/gcc-current
```

`-Select` accepts relative path prefixes or filenames; `-Limit` bounds selection.
`-Compiler` selects CPC; `-RuntimeRoot` selects an explicit matching runtime.
Use a fresh output directory for each run.

## Result contract

This is a restricted assessment, not full GCC/DejaGnu or C++ conformance.
Supported unconditional actions are preprocess, compile/assemble to an object,
link, and run with exit zero. Supported options include optimization, debug info,
and inline/builtin opt-outs. CPC's default C++ mode is used.

Target selectors, language-standard flags, diagnostic expectations, assembly/tree
scans, extra sources, and special drivers are `UNSUPPORTED`, never passes.
`-Probe` attempts unsupported sources and records acceptance/rejection without
verifying their expected behavior. Empty and probe-only runs cannot pass the gate.
Add adapter regression coverage before extending directive support. Preserve
upstream sources and expectations.

## Timing and artifacts

Compilation and execution each have a five-second ceiling including startup;
`-Timeout` may lower it. Crashes/timeouts fail. Timed-out processes are killed and
waited for before continuing. By default the runner stops on timeout;
`-ContinueAfterTimeout` completes a serial inventory with the same ceiling.
Cleanup has separate bounded waits. Do not retry with longer budgets.

| Output | Contents |
| --- | --- |
| `metadata.json`, `inputs.json` | Revision, settings, compiler/runner and source hashes; explicit runtime hashes |
| `results.jsonl`, `summary.json` | Per-source commands, diagnostics, statuses, timings, and totals |
| `progress.json` | Live deltas and phase timings; console updates every 15 seconds |
| `slow-failures.json`, `execution.json` | Timeout repair items and unexecuted coverage |

Implicit runtime lookup is marked unresolved in provenance. Compare identical
inputs and use `compare.ps1` to detect regressions and missing coverage.

For slow reproducers, use `triage-speed.ps1 -Source <file>` with optional
`-BaselineCompiler <old-cpc.exe>`. This measures identical inputs serially under
the same ceiling and writes `comparison.json` under `build/`. Minimize defects
and add fast regression coverage. Deliberately slow workloads need an explicit,
evidence-backed exclusion with lost coverage reported; preserve imported sources.

## Adapter checks

Run `test_runner.ps1`, `test_compare.ps1`, `test_provenance.ps1`, and
`test_progress.ps1` from this directory using PowerShell. These cover classification,
quoting, capture, timeouts/crashes, comparison, hashes, and incremental reporting.
See [the development loop](../DEVELOPMENT.md).
