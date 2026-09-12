# Retained GCC failure corpus

This directory holds only the unresolved rows from upstream GCC tests at
revision `5f6257c26b814de1a14c71b2d3a49291765b6577`
(https://github.com/gcc-mirror/gcc). Every retained case is expected to fail
until it is repaired; established passes were deleted once their behavior moved
into first-party coverage, and git history keeps the removed sources.

`corpus.json` lists and hashes the retained cases; supporting headers live
beside them and are hashed separately so they are never discovered as tests.

## Commands

```powershell
./Tests/pedantic/gcc/run.ps1 -List
./Tests/pedantic/gcc/run.ps1 -Select g++.dg/template/access27.C -Out build/gcc-exact
./Tests/pedantic/gcc/run.ps1 -ContinueAfterTimeout -Out build/gcc-retained
```

The default fast tier selects every retained case. The tier partition in
`Tests/tiers.json` is still consulted so a repaired row can be removed from the
corpus without disturbing the rest; there is no pedantic GCC partition while
every retained row is unresolved, and `-Tier pedantic` is a no-op.

`-CompilerPath` and `-RuntimeRoot` select the compiler and runtime. Each
compiler and program invocation has a five-second ceiling; crashes and timeouts
fail and are never retried with a larger budget. An unknown `-Select` fails.
`compare.ps1 -Baseline <old> -Current <new>` reports regressions and lost
coverage within the same corpus.

`summary.json`, `progress.json`, `metadata.json`, `inputs.json` and
`results.jsonl` under the output directory record results and provenance. The
score is verified passes over the selected retained cases; it is not GCC or
C++ conformance.

Focused adapter checks: `test_runner.ps1`, `test_compare.ps1`,
`test_progress.ps1`, `test_provenance.ps1`, `test_corpus.ps1`.
