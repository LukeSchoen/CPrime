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

The adapter passes `-fcoroutines` for `g++.dg/coroutines`, matching GCC's
`coroutines.exp` default options.

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

The repair order and row buckets live in [task.md](../../../task.md) under the
wave plan; repaired rows are deleted rather than promoted. The narrowed leads
for the remaining clusters are below.

## Narrowed leads

Each lead is already reduced to one site and is the shortest path for the
matching cluster:

- `conv20.C`: an object declaration materializes its class template
  specialization even where completeness is not required, so `extern B<int> b;`
  reports `base class 'A__int' is incomplete` for a `B` derived from an
  incomplete `A` and the arity short-circuit is never reached.
- Dependent template template argument: `T::template AA<U>::template B` names
  the nested template but loses the enclosing argument, so
  `chain<outer<char>, char>` still builds `middle<int>::inner` (`sizeof` stays 4
  where the `typename` spelling gives 1).
- Template parameter hiding: cycle 24 covers plain non-dependent bases; a
  dependent base can still substitute the argument and probing it must avoid
  recursively instantiating the current specialization (`access28.C`).
- Member function template of a member class template specialization:
  `template<> template<> template<class V> void A<int>::B<char>::g(V) { }` is
  still a namespace-scope template, so a linked call to `g` is undefined
  (`spec7.C` is compile-only; probe a call on `A<int>::B<char>`).
- Indirect virtual base mem-initializer is dropped (`skip_initializer_emit`) and
  default-initialized instead; cycle 17 only split base-object/complete
  (`*__base`) variants.
- Array decay: `return X::p == X::c ? 0 : 1;` reports `invalid operand types for
  binary operation` where the comparison opens a statement or `?:`;
  `if (!(...))` works.
