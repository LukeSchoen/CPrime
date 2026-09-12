# Remaining work

Target: 100% of the retained GCC language rows and the first-party pedantic
corpus, with no missing coverage, weakened expectations, compiler internal
errors or timeout retries.

State (2026-09-13, cycle 25): the retained corpus is 122 rows -- 39
PASS_COMPILE, 83 FAIL_COMPILE -- with 0 fast-tier regressions, and the
pedantic first-party list is empty: every first-party case passes at both
tiers (`Tests/run-all.ps1 -Tier fast`, `Tests/pedantic/run.ps1 -Group
language`). Cycle 19 finished pre-step 1 by repairing the last four first-party
failures. A range-for
over a member read from a const object takes the const element type; the
coroutine syntax case is a runnable case whose `co_yield`/`co_return` promise
lowering is observed; a by-value parameter is read alive inside its own full
expression, matching the pinned temporary-lifetime case; and a named local
returned by a trailing `return <local>;` is now constructed in the caller's
result object instead of being copied out of the frame.

Pre-step 2 has seven landed changes, each measured on this machine against the
compiler it replaced and each codegen-neutral over the first-party sources that
compile standalone. Cycle 19 bucketed saved inline-expansion bodies by callee
name and gated `find_cpp_this_symbol()`'s receiver scan on any
`__cprime_this_` identifier having been interned: the self-compile workload went
from 0.469 s to 0.390 s (median of 7 runs, -16.8%) and `cpp.call.source.lookup`
from 18.640 ms to 15.055 ms. Cycle 20 cached the `CPRIME_TRACE_INCOMPLETE`,
`CPRIME_DUMP_AUTORET` and `CPC_TRACE_RETURN` gates: each gate sits in a
per-instantiation path and rescanned the whole environment block per call, which
the sampling profiler had charged 9.4% of the self-compile to. Against the
compiler it replaced the self-compile workload went from 0.411 s to 0.358 s
(-13.0%), `cpp.template.static.lookup` from 25.958 ms to 23.228 ms (-10.5%), the
`-E` control is unchanged, and the sampler shows no unmapped DLL bucket above
1.3%. Cycle 22 turned the cached `is_cpp_translation_unit()` check into a macro
with a one-time `_slow` filename probe, removing an out-of-line call from a
pervasive frontend path; the 12-compilation in-process self-compile workload
went from a 4118 ms median to 3953 ms (-4.0%), the single-compile median from
344 ms to 328 ms, and the compiled object is byte-identical. Numbers live in
`BuildProfile/README.txt`. Cycle 23 remembered the fixed spellings the C++
front end re-interns while resolving names (`this`, `operator=` and the other
operator spellings, `<no name>`, `__cpc_lexical_types`, `__cprime_vptr`): 217k
`tok_alloc_const()` calls per compilation, 92% of the calls the profiler
charged `tok_alloc`, became a compare. The 12-compilation in-process
self-compile workload went from a 4047 ms median to 3906 ms (-3.5%), the
single-compile `-bench` median from 336 ms to 313 ms (-6.8%), and the cache is
still filled at the first use of each spelling, so all 1534 first-party sources
that compile standalone and the self-compile object stay byte-identical.
Cycle 24 stopped the C front end from paying for C++ name resolution it cannot
use, with gates that read only the state the parser sets for the construct being
skipped: `find_current_namespace_tok_ex()` answers while no binding redirects a
name and no namespace state exists, `find_current_class_nested_type_tok()`
answers while no member class is active and no member declarator is being
parsed, `find_class_template_def()` and `find_function_template_def()` return
NULL while no template has been registered, and `gexpr()` skips the
comma-operator lookups until an `operator<op>` spelling has been parsed. The
12-compilation in-process self-compile workload went from a 3890 ms median to
3735 ms (-4.0%), the wall median from 3897 ms to 3726 ms (-4.4%), the
single-compile `-bench` median is unchanged inside its 15.6 ms tick, and the
1576 first-party tests that compile standalone plus the self object stay
byte-identical. Gates keyed on the source filename were tried first and
rejected: this parser also accepts `namespace`, `using`, `class` and operator
declarations in a `.c` file.
Cycle 25 removed repeated expression-spelling work from the C front end:
`unary()` derives an identifier's spelling once, directly from `table_ident`,
and gates the `typename`, named-cast and `delete` probes on the spelling's
first character; `cpp_type_trait_name_tok()` rejects names that do not begin
`__i`/`__h` before its ten string comparisons; and `get_tok_str()` no longer
resets its scratch buffer on the identifier fast path. The 12-compilation
in-process self-compile workload went from a 3756 ms wall median to 3690 ms
(-1.8%) and a 3735 ms inner-time median to 3672 ms (-1.7%), 6 interleaved
pairs, and the compiled self object is byte-identical.

The remaining work is the rest of pre-step 2, then Wave A item 3. The profile
leads for the next speed session: `next_nomacro` is the largest reliable leaf at
about 13%; `unary` (most of it beyond the sampler's 64 KB symbol window) is
still about 5%, `g` 3.0%, `next` 2.8%, `vswap` 2.2% and `parse_btype` 2.4%; the
rest is a long tail (`tok_str_add2`, `gen_cast`, `preprocess_skip`, `load`,
`macro_subst`, `tal_realloc_impl`, the statement/decl path) over roughly 12% of
unmapped leaf samples in DLL routines. The repeated `get_tok_str()` calls and
spelling probes that fed those checks are off the list.
`find_expand_inline_function`, `find_cpp_this_symbol`, the environment gates,
the `is_cpp_translation_unit` call, the literal interning behind `tok_alloc` and
the C++ name-resolution helpers gated in cycle 24 are off the list.

Three failures sit outside the target, for whoever owns those gates.
`Tests/run-checks.ps1` reports `test_MsvcRecordReturn` failing on
`native_make<Defaulted4>` (exit 8), which matches cycle 18's "a class whose
only constructor is defaulted stays an aggregate" change against MSVC's
trivial-record rule. `test_AsmOutput` fails in every recorded check run,
including runs before cycle 18. Neither is part of the retained corpus or the
pedantic first-party list, and cross-compiler ABI gates need explicit
authorization. `Tests/pedantic/performance/pass/test_heap_list_push_clear_perf.cpp`
does not compile in any compiler state: it calls `GetTickCount64`, which the
vendored `third-party/win32-sdk/include/winapi/winbase.h` never declares, so
the pre-step 2 runtime workload is unavailable until CPrime supplies that
prototype from first-party code.

## Pre-steps

Do these first, in order. The target does not move; the order does. While a
pre-step is unfinished, do not open a new retained GCC row.

1. Green internal baseline. Every existing first-party test passes at both
   tiers -- `Tests/run-all.ps1 -Tier fast` and `Tests/run-all.ps1 -Tier
   pedantic` -- and `Tests/progress/first-party-failures.txt` is empty. Repair
   each listed failure as a normal cycle (reproduce, minimal first-party
   regression, fix, re-run the tier). The retained external GCC rows are the
   only cases allowed to fail by design: no skips, no `EXPECT_COMPILE_FAIL`
   rewrites, no relaxed timeouts and no missing coverage.
2. Compilation-speed wave. Make root `cpc.exe` compile the existing workloads
   measurably faster with no behavior change: no new passes, no new failures
   and no weakened expectations. Improve algorithms and data handling, keep one
   thread and one compiler process at a time, and measure the identical
   workload before and after on this machine with `cpc.exe` alone. Record both
   numbers; keep a change only when it is a repeatable win, and revert it
   otherwise. Use `Tests/benchmarks/compile/` and
   `Tests/pedantic/performance/`; add first-party benchmark inputs there rather
   than a second harness. The wave ends when the benchmark set stops yielding a
   repeatable win without regressing the fast or pedantic tiers.
3. Wider GCC retarget. Only after 1 and 2, continue the wave plan below
   (Wave A item 3 onward) and re-assess the retained corpus against the wider
   remaining GCC language rows, adding rows as the wave budget allows.

## Cycle contract

One session = one cluster; a fresh launch beats a long session that runs out of
context.

1. Pick the next unfinished pre-step above, or the next item from the wave plan
   below once the pre-steps are done. Reproduce one row:
   `Tests/pedantic/gcc/run.ps1 -Select <path> -Out build/<name>`.
2. Reduce it to a first-party regression in the nearest `Tests/features/...`
   suite, fix the compiler, and re-run the selection plus the regression until
   both pass.
3. Retire the row: delete the corpus file, remove its `corpus.json` case entry
   and drop any `Tests/tiers.json` entry for it. Repaired rows are deleted, not
   promoted into a permanent external corpus.
4. Re-run `Tests/run-all.ps1 -Tier fast` once (about 20 s) to confirm zero
   regressions, update the State line, then stop. Repairing one of the
   first-party failures below also means deleting its line from
   `Tests/progress/first-party-failures.txt`; the worker counts the retained
   corpus plus that list, records the rate in `Tests/progress/log.tsv`, and
   stops only when both are empty.

Do not start a second cluster. If a fix has not landed after roughly half the
budget, revert it, record the narrowed lead in the retained GCC README, and
stop. Run focused selections, print only the tail of a run, and never open whole
test files or inventories when a path plus first diagnostic is enough. Do not
paste inventories into this file. Create `done.x` and stop the moment every
retained row and pedantic test passes.

Measurements must name the compiler explicitly: on this host a bare `cpc.exe`
resolves through the PATH to an unrelated archived compiler, which reports
different predefined macros and emits different objects.

## Wave plan

Waves A to C run after the pre-steps above are done.

139 rows = 40 coroutines + 17 `_Complex` + 82 general language long tail.
Without Waves B and C the ceiling is ~82/139, so both features are required.

### Wave A - general language long tail

Batch two or three independent repairs per session, in this order:

1. Overload and deduction singletons: the remaining `no matching function
   template` / `no matching overloaded function` rows. `mem_fun` and
   `defaultHandler`, `call7.C`, `ttp58.C` and `spec4.C` are retired, and cycle
   10 retired `template26.C` and `spec28.C`; cycle 13 retired `overload6.C` and
   `friend28.C`, so this item is empty apart from
   `g++.dg/coroutines/pr113457.C`, which belongs to Wave C.
2. Const and conversion cluster: empty; cycle 15 retired the last
   `cannot convert` external rows (`new5.C`, `cvt21.C`, `ptrmem5.C`).
3. The narrowed leads recorded under
   [retained GCC checks](Tests/pedantic/gcc/README.md#narrowed-leads), one site
   each: `access28.C`, `spec7.C`, the dependent template template
   chain, the indirect virtual base mem-initializer, array decay in a `?:`
   condition, then `conv1.C` (current lead), `synth7.C`, `template25.C`,
   `canon-type-3.C`, `syshdr1.C` and `anon3.C`.

### Wave B - `_Complex` (17 rows, 3 sessions)

1. Type/declarator parsing and imaginary literals for `_Complex` and
   `__complex__`, retiring the `invalid number` and `';' expected (got
   'double'/'__complex__')` clusters.
2. Arithmetic, conversions and the `__real__` / `__imag__` / `conj` builtins.
3. Runtime layout, ABI and the `g++.dg/opt` optimization rows.

### Wave C - coroutines (40 rows, 4 sessions)

1. Ship `<coroutine>` and `<experimental/coroutine>` and accept `-fcoroutines`;
   this single step unblocks 28 rows.
2. Parse `co_await`, `co_yield`, `co_return` and awaitables.
3. Promise machinery and coroutine frame lowering.
4. ABI, symmetric transfer, exceptions and destruction.

## Known first-party pedantic failures

None. `Tests/progress/first-party-failures.txt` is the authoritative path list.
CPrime tests that reproduce failures in the sibling CL/OTServer project must
stay standalone; do not include `CommonLib` or other CL headers.

The named return value optimization is deliberately partial: it applies to a
body whose single, trailing `return <identifier>;` names an automatic object
of the caller-owned result type, declared directly in the function body's own
block, and whose declaration is constructor-shaped. Other named returns keep
the byte-copy elision that was already in place.

Resolved work, per-cycle history and yield analysis live in git history and
[the development loop](Tests/DEVELOPMENT.md).
