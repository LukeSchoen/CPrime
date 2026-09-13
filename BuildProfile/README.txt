BuildProfile

Purpose:
- Measure how quickly CPC builds representative inputs.
- Keep this separate from correctness tests so timing data can be audited without
  changing pass/fail language coverage.

Cases:
- Put compile-time profile inputs in Tests\benchmarks\compile.
- Files named test_*.c or test_*.cpp are compiled once per sample.
- Optional metadata in the first 12 lines:
  // PROFILE_NAME: readable.case.name
  // PROFILE_ARGS: extra cpc arguments

Run:
- BuildProfile.cmd
- BuildProfile.cmd -Iterations 10 -Warmups 2
- BuildProfile.cmd -CompilerPath path\to\cpc.exe
- BuildProfile\build-cpc-clang.cmd
- BuildProfile\compare-tcc.cmd -BuildTcc -Iterations 20 -Warmups 3
- BuildProfile\compare-tcc.cmd -BuildTcc -TccBuildCompiler path\to\clang.exe
- BuildProfile\compare-tcc.cmd -CpcPath build\clang\cpc-clang.exe
- BuildProfile\compare-asm.cmd -Iterations 50 -Warmups 5

Output:
- build\profiles\compile\build-profile.csv
- build\profiles\compile\tcc-compare.csv
- build\profiles\compile\asm-compare-samples.csv
- build\profiles\compile\asm-compare-summary.csv
- build\clang\cpc-clang.exe
- Console summary with average compile time per case.

Raw tcc baseline:
- Upstream tcc is vendored in third-party\tcc.
- BuildProfile\compare-tcc.cmd compiles only C profile cases, first with raw
  tcc and then with CPC, and reports CPC speed as a percentage of raw tcc.

Section-byte assembly serialization comparison:
- Yasm is vendored in third-party\yasm as a small standalone external assembler.
- BuildProfile\compare-asm.cmd emits CPC's current `-Sbytes` section-byte assembly
  serialization once per case, assembles it repeatedly with CPC and Yasm, links
  both objects with CPC, and reports timing plus byte-exactness against the
  direct CPC executable.

Full self-build packaging (2026-09-08)
- Build.cmd, self-built CPC, serial compilation, full embedded SDK/runtime.
- Header processing, payload serialization and compression use a native C helper
  rebuilt by CPC on each invocation. No C#, Add-Type, payload cache or
  incremental-build shortcut is used by packaging.
- Previous interpreted-PowerShell full build: 14.01 seconds. Native C runs:
  4.227, 4.366, 4.410 seconds; median 4.366 seconds. The first run followed
  deletion of build/compiler.
- Timings include command startup, both runtime builds, compiler compile/link,
  helper compilation, full packaging and replacement of the root executable.
- Original/new compressed payloads are byte-identical; cpc.exe is 2,222,398 bytes.
- Validation: test_PortablePackaging.ps1 (including isolated SDK/runtime use),
  c_compat (23 passed), features/Includes (41 passed).

Compile-speed wave (2026-09-13, cycle 19)
- Workload A: `cpc.exe -bench -c src/compiler/driver/cprime.c` with the
  compiler's own include set and defines, 171,853 preprocessed lines, median of
  7 runs on this machine: 0.469 s before, 0.390 s after (-16.8%).
- Workload B: BuildProfile.cmd -Iterations 20 -Warmups 3, average per case,
  before -> after in ms: cpp.call.source.lookup 18.640 -> 15.055 (-19.2%),
  c.integer.compile.stress 10.466 -> 10.491, c.empty.main 10.136 -> 10.049,
  test_conversion_template_owner_lookup 18.783 -> 18.854,
  cpp.template.static.lookup 26.403 -> 26.236,
  cpp.template.member.list 12.981 -> 13.165. Every unchanged case stayed
  within +/-1.5%, which is this host's run-to-run spread.
- `cpp.call.source.lookup` is a new first-party input: it fills the local stack
  and the saved inline-body table, then resolves many unqualified calls.
- Changes kept: bucket the saved inline-expansion bodies by callee name, and
  gate find_cpp_this_symbol()'s receiver scan on any `__cprime_this_`
  identifier having been interned. Both were verified codegen-neutral: the
  1557 first-party C/C++ sources that compile standalone produced
  byte-identical objects before and after.
- Validation for the wave: Tests/run-all.ps1 -Tier fast (0 regressions, the
  retained corpus stays at 39/122) and Tests/pedantic/run.ps1 -Group language
  (25 suites, 0 failures).

Compile-speed wave (2026-09-13, cycle 20)
- Workload A: `cpc.exe -bench -c src/compiler/driver/cprime.c` with the
  compiler's own include set and defines, 171,942 preprocessed lines, median of
  7 runs on this machine, two rounds per compiler: 0.412 s and 0.411 s before,
  0.364 s and 0.352 s after (-13.0% on the round medians).
- Workload A in process (same include set, 12 compilations per invocation, so
  process startup is amortized): median per compilation 391 ms before, 343 ms
  after (-12.3%).
- Preprocess-only control (`-E`, 20 compilations per invocation): 187 ms
  before and after, which is the expected result because the change is in the
  parser rather than the tokenizer.
- Workload B: BuildProfile.cmd -Iterations 20 -Warmups 3, average of two
  rounds per compiler, before -> after in ms: cpp.template.static.lookup
  25.958 -> 23.228 (-10.5%), c.integer.compile.stress 10.333 -> 10.104,
  cpp.template.member.list 13.304 -> 13.089, c.empty.main 9.998 -> 10.071,
  cpp.call.source.lookup 14.612 -> 14.823, test_conversion_template_owner_lookup
  18.531 -> 18.783. The unchanged cases sit inside this host's +/-3% small-case
  spread and are dominated by process startup.
- Profiler evidence: `build\profile-process.exe` sampled against
  `build\compiler\cpc.map` charged about 9.4% of the self-compile to a single
  ucrtbase address, which the command-line probes in the same tool identified
  as the `getenv` block. The callers were the `CPRIME_TRACE_INCOMPLETE`,
  `CPRIME_DUMP_AUTORET` and `CPC_TRACE_RETURN` gates, which sit in the
  per-instantiation paths and rescanned the whole environment block per queued
  specialization. After caching those gates per compilation the same profile
  shows no unmapped DLL bucket above 1.3%.
- Changes kept: resolve the three environment-driven trace gates once per
  compilation state instead of per call. The gates and their output are
  unchanged when the variables are set: with all three set, the base and new
  compilers print byte-identical trace output for
  Tests/features/Templates/pass/test_member_template_deduction_scaling.cpp
  (11816 lines each), and 1505 first-party C/C++ sources that compile
  standalone produced byte-identical objects before and after.
- Validation for the wave: Build.cmd self-host and publish, Tests/run-all.ps1
  -Tier fast (0 regressions, the retained corpus stays at 39/122),
  Tests/run-all.ps1 -Tier pedantic (577 passed, 0 failed) and
  Tests/pedantic/run.ps1 -Group language (25 suites, 0 failures).

Compile-speed wave (2026-09-13, cycle 22)
- Workload A in process (same include set, 12 compilations per invocation):
  four rounds per compiler, inner sum before 4172/4110/4109/4125 ms -> after
  4000/3984/3922/3906 ms, median 4118 -> 3953 ms (-4.0%).
- Workload A: `cpc.exe -bench -c src/compiler/driver/cprime.c` with the
  compiler's own include set and defines, 17 interleaved pairs: inner-time
  median 344 ms before, 328 ms after (-4.7%), mean 341.8 -> 326.4 ms (-4.5%).
  The compiled self object is byte-identical before and after.
- Workload B: BuildProfile.cmd -Iterations 20 -Warmups 3, average per case,
  before -> after in ms: c.integer.compile.stress 10.373 -> 10.447,
  c.empty.main 10.134 -> 9.936, cpp.call.source.lookup 14.813 -> 14.878,
  test_conversion_template_owner_lookup 18.481 -> 18.601,
  cpp.template.static.lookup 23.427 -> 23.341,
  cpp.template.member.list 13.123 -> 13.332. The small cases are dominated by
  process startup and stayed inside this host's +/-2% spread.
- Change kept: replace the out-of-line `is_cpp_translation_unit()` call in the
  pervasive frontend paths with a cached-value macro and a one-time `_slow`
  filename probe. The profiler had charged 1.75% of the self-compile to the
  function's leaf.
- Validation for the wave: Build.cmd self-host and publish, Tests/run-all.ps1
  -Tier fast (0 regressions, the retained corpus stays at 39/122) and
  Tests/pedantic/run.ps1 -Group language (25 suites, 577 passed, 0 failures).

Compile-speed wave (2026-09-13, cycle 26)
- Workload A: `cpc.exe @build/prof-self.rsp` with the compiler's own include
  set, 12 compilations per invocation, 8 interleaved pairs with the packed
  compilers: wall median 3715.2 ms before (published cycle 25 root) and
  3638.0 ms after (-2.08%); every pair favoured the new compiler. The same
  pair measured with the unpacked build (identical sources and flags, so the
  only difference is the change under test) went from a 3605.7 ms to a
  3511.3 ms median, -2.62%. The published root compiler produces the same self
  object as the profiled candidate build.
- Workload B: BuildProfile.cmd -CompilerPath <compiler> -Iterations 20
  -Warmups 3, one round per compiler, before -> after in ms:
  c.integer.compile.stress 10.247 -> 10.450, c.empty.main 10.217 -> 10.110,
  cpp.call.source.lookup 14.538 -> 14.301,
  test_conversion_template_owner_lookup 17.951 -> 18.376,
  cpp.template.static.lookup 23.012 -> 22.452,
  cpp.template.member.list 12.905 -> 13.358. These inputs compile in 1-14 ms
  and are dominated by the ~10 ms process start, so the in-process
  self-compile workload carries the resolution.
- Profiler evidence: `find_cpp_this_symbol()` was charged 0.7-1.3% of the
  self-compile; it is called for every call expression, and its two
  `sym_find()` probes each cost a random `table_ident` load plus a TokenSym
  deref for a result that can only exist when a 'this'-named binding is live.
  `next()` was charged about 1% in the `define_find()` probe at 0x40d48a,
  which re-read the TokenSym the tokenizer had just touched. `preprocess_skip()`
  dispatched its switch once per skipped byte (1.88% leaf before, 1.33%
  after). The identifier scan-plus-lookup region of `next_nomacro` is still
  the largest block, about 7% of the run, of which the scan/hash loop is 3.4%,
  the bucket load 2.0% and the binding checks 1.5%.
- Changes kept: `find_cpp_this_symbol()` returns NULL while no 'this' or
  lambda-'this' identifier binding has been linked (the flag is set by the two
  writers of a token's `sym_identifier` and reset per translation unit, and
  the two spellings stay interned at their first use so identifier numbering
  is unchanged); `sym_free()` no longer forwards its almost-always-NULL
  `field_index` through the reallocator hook; `next()` takes the macro binding
  from the TokenSym `next_nomacro()` just interned when its token matches;
  `preprocess_skip()` advances over runs of bytes that its switch does not
  handle specially, with a separate stop set for the #warning/#error mode
  where quotes and '/' are ordinary text.
- Rejected: replacing the identifier comparison's `memcmp` with an inline byte
  loop measured 0.5% slower (this codegen keeps loop variables in memory, and
  ucrtbase's `memcmp` is already cheap for short strings), so it was reverted.
- Verified codegen-neutral: all 1601 first-party pass-tier sources were
  compiled with the previous and the new compiler; the 1599 that both compiled
  produced byte-identical objects and byte-identical diagnostic text, and the
  remaining 2 failed for both. The run includes the new
  Tests/c_compat/pass/test_skipped_block_text_scan.c, which pins the bytes the
  restructured skip scan walks past (strings and character constants spelling
  directives, comments holding '#' and quotes, a line continuation, nested
  conditionals and a warning line whose unbalanced quote and slash are
  ordinary text).
- Validation for the wave: Build.cmd self-host and publish, Tests/run-all.ps1
  -Tier fast (0 regressions, the retained corpus stays at 39/122) and
  Tests/pedantic/run.ps1 -Group language (25 suites, 577 passed, 0 failures).

Compile-speed wave (2026-09-13, cycle 23)
- Workload A in process: `cpc.exe @build/prof-self.rsp` with the compiler's own
  include set, 12 compilations per invocation, 5 interleaved pairs: inner-time
  median 4047 ms before, 3906 ms after (-3.5%); wall median 4063 -> 3920 ms.
- Workload A single compile: `cpc.exe -bench -c src/compiler/driver/cprime.c`,
  6 interleaved pairs, two samples per tick value and one slow pair: median
  336 ms before (328/328/328/344/344/344), 313 ms after
  (297/312/312/313/313/344), -6.8%. The bench clock counts 15.6 ms ticks, so
  this is the coarser of the two measurements.
- Workload B: BuildProfile.cmd -Iterations 20 -Warmups 3, average of two
  rounds per compiler, before -> after in ms: c.integer.compile.stress
  10.567 -> 10.182, c.empty.main 10.085 -> 10.028, cpp.call.source.lookup
  14.442 -> 14.450, test_conversion_template_owner_lookup 18.240 -> 18.312,
  cpp.template.static.lookup 23.037 -> 23.045, cpp.template.member.list
  13.023 -> 13.138. These inputs compile to objects in 1-14 ms and so are
  dominated by the ~9 ms process start; every case sits inside this host's
  +/-3% spread, which is why the in-process workload carries the resolution.
- Profiler evidence: the sampler charged 2.7% of the self-compile to
  `tok_alloc`'s leaf, and 2.8% of the stack share to `tok_alloc_const`. Counting
  the callers showed 217k `tok_alloc_const()` calls per compilation, 92% of all
  `tok_alloc` calls, of which 112k re-interned "this" from the identifier lookup
  in `unary`. The next sites were
  `find_class_template_def_for_class_tok` (21.3k, "<no name>"),
  `try_call_cpp_bool_conversion_operator` (18.9k), `try_call_cpp_assignment_operator`
  (14.7k), `get_cpp_binary_operator_method_tok` (14.2k),
  `make_class_lexical_type_tok` (11.1k), `virtual_vptr_field_tok` (10.4k) and
  `try_call_cpp_index_operator` (4.5k).
- Change kept: remember the token for each fixed front-end spelling once per
  translation unit (`CPP_TOK_*` over `CPC_CACHED_TOK` in cprimegen.c) and fold
  the operator spellings `get_cpp_binary_operator_method_tok` derives from a
  token into a per-operator table. The caches are filled at the first use of
  the spelling, which is where the literal used to be interned, so identifier
  numbering is unchanged: all 1534 first-party sources that compile standalone
  produced byte-identical objects before and after, and the compiled self
  object is byte-identical. Interning the same spellings eagerly in
  cprimegen_init was tried first and rejected: it shifted token ids and with
  them the synthesized `__cpc_local_class_<tok>_<n>` names in 26 of those
  objects.
- Validation for the wave: Build.cmd self-host and publish, Tests/run-all.ps1
  -Tier fast (0 regressions, the retained corpus stays at 39/122) and
  Tests/pedantic/run.ps1 -Group language (25 suites, 577 passed, 0 failures).

Compile-speed wave (2026-09-13, cycle 24)
- Workload A in process: `cpc.exe @build/prof-self.rsp` with the compiler's own
  include set, 12 compilations per invocation, 6 interleaved pairs: inner-time
  sum before 3922/3875/3860/3890/3891/3875 ms (median 3890), after
  3703/3735/3735/3750/3735/3734 ms (median 3735), -3.98%. Wall median
  3897 -> 3726 ms (-4.39%) over 5 interleaved pairs.
- Workload A single compile: `cpc.exe -bench -c src/compiler/driver/cprime.c`
  with the compiler's own include set, 6 interleaved pairs: both compilers
  report a 328 ms median (312-344 before, 312-329 after), which is inside the
  bench clock's 15.6 ms tick, so the in-process workload carries the
  resolution.
- Small-case control: 40 copies of `test_c_integer_compile_stress.c` and 40 of
  `test_empty_main.c` compiled in one process, inner-time sum 390 ms before and
  391 ms after (5 interleaved pairs), so the process-startup-dominated cases
  below are unchanged.
- Workload B: BuildProfile.cmd -Iterations 20 -Warmups 3, average of two rounds
  per compiler, before -> after in ms: c.empty.main 10.007 -> 9.909,
  c.integer.compile.stress 10.265 -> 11.031 (one 11.7 ms outlier; the paired
  round was 10.333, and the in-process control above shows the case itself is
  unchanged), cpp.call.source.lookup 14.629 -> 14.166,
  cpp.template.member.list 13.159 -> 13.050, cpp.template.static.lookup
  23.029 -> 22.816, test_conversion_template_owner_lookup 18.557 -> 18.514.
  These inputs compile in 1-14 ms and so are dominated by the ~10 ms process
  start; every case is inside this host's +/-4% spread.
- Profiler evidence: the sampler charged about 5% of the self-compile to C++
  name-resolution work a C translation unit cannot use:
  find_current_namespace_tok_ex (1.57% leaf, 2.21% stack),
  find_current_class_nested_type_tok (0.74% leaf, 1.91% stack),
  find_class_template_def (1.72% leaf), the
  try_call_cpp_free_binary_operator/cpp_resolve_free_operator/
  cpp_resolve_associated_function chain the comma operator entered from
  gexpr() (1.33% stack), and cpp_unnamed_tag_typedef_tok (1.03% leaf), which
  those paths reached through the overload registry. None of them reaches the
  report's 80-row ranking afterwards.
- Changes kept: four gates, each reading only state the parser sets for the
  construct it skips, so a translation unit that uses the construct keeps the
  old path: (1) find_current_namespace_tok_ex() answers immediately while no
  binding redirects a name (no using-declaration or namespace alias has been
  recorded), no namespace is in scope, no unnamed namespace exists, no
  using-directive is active and no inline namespace exists; (2)
  find_current_class_nested_type_tok() answers immediately while no member
  class is active, no enclosing class is on the definition stack and no member
  declarator is being parsed; (3) find_class_template_def() and
  find_function_template_def() return NULL while no template has been
  registered, because index_template_def() is the only writer of those buckets
  and runs with the same counter; (4) gexpr() skips both comma-operator lookups
  until some overload has been registered or an `operator<op>` spelling has
  been parsed. A filename-based gate was tried first and rejected: this parser
  also accepts `namespace`, `using`, `class` and operator declarations in a
  `.c` file, and two such inputs compiled differently under it.
- Verified codegen-neutral: 1576 of the 1738 first-party test sources compile
  standalone with both compilers and produced byte-identical objects, and the
  compiled self object is byte-identical. 16 hand-written probes cover the
  C++ spellings the parser accepts in a `.c` file (namespace, namespace alias,
  using-directive, using-declaration, class-scope using-declaration, class,
  nested class, member function, member comma operator, free comma operator,
  other free operators, class template, template inside a namespace) and
  produce identical exit codes and byte-identical objects.
- Validation for the wave: Build.cmd self-host and publish (the rebuilt
  cpc.exe is byte-identical to the profiled candidate), Tests/run-all.ps1
  -Tier fast (0 regressions, the retained corpus stays at 39/122) and
  Tests/pedantic/run.ps1 -Group language (25 suites, 577 passed, 0 failures).

Compile-speed wave (2026-09-13, cycle 25)
- Workload A in process: `cpc.exe @build/prof-self.rsp` with the compiler's own
  include set, 12 compilations per invocation, 6 interleaved pairs: inner-time
  sum before 3750/3765/3735/3765/3735/3735 ms (median 3735, mean 3747.5),
  after 3672/3657/3672/3641/3688/3672 ms (median 3672, mean 3667.0), -1.7%
  median and -2.1% mean; wall median 3756 -> 3690 ms (-1.8%), mean 3760 ->
  3682 ms (-2.1%). The compiled self object is byte-identical.
- Workload B: BuildProfile.cmd -CompilerPath <compiler> -Iterations 20
  -Warmups 3, one round per compiler, before -> after in ms:
  c.integer.compile.stress 10.256 -> 10.146, c.empty.main 10.154 -> 10.011,
  cpp.call.source.lookup 14.533 -> 14.464,
  test_conversion_template_owner_lookup 18.363 -> 18.034,
  cpp.template.static.lookup 23.083 -> 23.047,
  cpp.template.member.list 12.965 -> 12.682. These small inputs compile in
  1-14 ms and are dominated by the ~10 ms process start, so the in-process
  self-compile workload carries the resolution.
- Profiler evidence: `unary` was charged 5.46% of the self-compile as a leaf and
  re-derived the same identifier spelling with `get_tok_str()` for each of the
  `typename`, four named-cast and `delete` probes, while
  `cpp_type_trait_name_tok()` ran its ten string comparisons for every
  call-shaped expression even though every trait begins `__i` or `__h`. After
  the change the sampler no longer ranks `get_tok_str` and the self-compile
  sample count fell with the wall time.
- Changes kept: `unary()` derives the identifier spelling once, directly from
  `table_ident`, and gates each C++ keyword probe on the spelling's first
  character; `cpp_type_trait_name_tok()` rejects names that do not begin
  `__i`/`__h` before its string comparisons; `get_tok_str()` resets its scratch
  buffer only after the identifier fast path. The direct `table_ident` read is
  the same entry `get_tok_str()` returned, so token numbering and object bytes
  are unchanged.
- Verified codegen-neutral: the compiled self object is byte-identical before
  and after.
- Validation for the wave: Build.cmd self-host and publish, Tests/run-all.ps1
  -Tier fast (0 regressions, the retained corpus stays at 39/122) and
  Tests/pedantic/run.ps1 -Group language (25 suites, 577 passed, 0 failures).

Compile-speed wave (2026-09-13, cycle 27)
- Workload A in process: `cpc.exe @build/prof-self.rsp` with the compiler's own
  include set, 12 compilations per invocation, 16 interleaved pairs with the
  order alternated: inner-time median 3547 ms before and 3516 ms after
  (-0.87%), mean 3542.2 -> 3523.4 ms (-0.53%); wall median 3553.8 -> 3530.0 ms
  (-0.67%). The pairs split 10 wins, 3 losses and 3 ties, with a paired delta
  median of -15 ms and mean of -18.8 ms, so the result is repeatable rather
  than a single fast run.
- Workload B: BuildProfile.cmd -CompilerPath <compiler> -Iterations 20
  -Warmups 3, one round per compiler, before -> after in ms:
  c.integer.compile.stress 10.433 -> 10.007, c.empty.main 10.082 -> 10.169,
  cpp.call.source.lookup 14.505 -> 14.254,
  test_conversion_template_owner_lookup 18.434 -> 18.310,
  cpp.template.static.lookup 22.490 -> 22.415,
  cpp.template.member.list 12.560 -> 12.970. These cases are dominated by the
  ~10 ms process start and stay inside this host's spread.
- Profiler evidence: the hot 32-byte buckets inside `next_nomacro` were the
  per-character hash update. The old expression
  `h + (h << 5) + (h >> 27) + c` compiled to a memory-resident hash with
  several dependent shifts and adds per identifier byte. The replacement
  `h * 31 + c` keeps a full-spelling hash but compiles each byte update to one
  `imul` and one `add`.
- Change kept: use the multiplicative identifier hash in `next_nomacro()` and
  `tok_alloc()`. Both creation and lookup read the same `TOK_HASH_FUNC`, so
  bucket placement changes without changing token allocation order or token
  ids.
- Rejected: hashing identifiers from a fixed sample of bytes removed the
  per-byte chain but was 1.75% slower from extra bucket collisions; a
  shift-xor hash was 45% slower because its low bits stopped depending on the
  prior hash; batching `o()` and the `gen_le*()` byte emitters was 1.8% slower;
  and inlining `get_tok_str()`'s identifier fast path was neutral. All were
  reverted.
- Verified codegen-neutral: 1551 standalone first-party pass sources compiled
  with the previous root and the new root produced byte-identical objects; the
  remaining source failed under both compilers.
- Validation for the wave: Build.cmd self-host and publish, Tests/run-all.ps1
  -Tier fast (0 regressions, the retained corpus stays at 39/122; only the
  expected GCC retained suite fails) and Tests/pedantic/run.ps1 -Group language
  (25 suites, 577 passed, 0 failures).

Compile-speed wave (2026-09-13, cycle 28)
- Workload A in process: `cpc.exe @build/prof-self.rsp` with the compiler's own
  include set, 12 compilations per invocation, 12 interleaved pairs with the
  order alternated, measured with the unpacked base and candidate builds (same
  sources and flags, so the change under test is the only difference): inner
  time median 3516 -> 3500 ms (-0.46%), mean 3521.0 -> 3494.8 ms (-0.74%); wall
  median 3527 -> 3505 ms (-0.62%), mean 3530.9 -> 3507.7 ms (-0.66%); the pairs
  split 9 wins, 2 losses and 1 tie on inner time and 11 wins, 1 loss on wall. A
  first round of 8 pairs that timed one compilation per run measured a -5.4 ms
  mean per compile with an unchanged median, which is inside that round's
  spread; the 12-job sums carry the resolution.
- Workload B: BuildProfile.cmd -CompilerPath <compiler> -Iterations 20
  -Warmups 3, one round per compiler, before -> after in ms:
  c.integer.compile.stress 10.450 -> 10.336, c.empty.main 10.192 -> 9.964,
  cpp.call.source.lookup 14.056 -> 14.514,
  test_conversion_template_owner_lookup 18.444 -> 18.424,
  cpp.template.static.lookup 22.781 -> 22.792,
  cpp.template.member.list 13.098 -> 12.938. These inputs compile in 1-14 ms
  and are dominated by the ~10 ms process start, so they stay inside this
  host's +/-4% spread and the in-process workload carries the resolution.
- Profiler evidence: `next_nomacro` is still the largest reliable leaf at
  12.4-12.6% of the self-compile. Its 32-byte buckets put about 6.5% in the
  identifier scan, hash update and bucket walk, 1.8% at the switch dispatch and
  0.8% on the exit path. Temporary counters in the fast path measured 334,519
  identifier lookups per compilation over 2,580,756 identifier bytes (7.7 bytes
  each), 453,301 hash-chain probes and 302,794 probes that matched hash and
  length and therefore ran memcmp.
- Change kept: `set_idnum()` now derives `ident_cont[256]`, whose entry c holds
  c while c can continue an identifier and 0 otherwise. The scan in
  next_nomacro() reads that byte as both the loop test and the hash addend, so
  each identifier byte costs one classification load instead of a
  flag-masking `isidnum_table` load plus a separate reload of the character.
  `set_idnum()` is the only writer of `isidnum_table`, so the derived table
  cannot go stale, and no identifier character is 0, so the loop stops exactly
  where the old IS_ID|IS_NUM test did.
- Verified codegen-neutral: all 1602 first-party pass-tier sources were
  compiled with the previous and the new compiler, one process per source;
  the 1599 that both accepted produced byte-identical objects and the
  remaining 3 failed for both with byte-identical diagnostics. The compiled
  self object is byte-identical.
- Lead recorded: CPC batch mode (one process, many jobs) is not measurement- or
  sweep-safe. A batch of the first 88 pass-tier sources deterministically makes
  job 88 (Tests/features/Classes/pass/test_anonymous_enum_member_replay.cpp)
  fail with "member function definition requires declared class 'Controls'"
  although that source compiles alone, and a longer sweep stopped producing
  results entirely after 228 jobs. The neutrality sweep above therefore used
  one process per source. Finding the state that leaks between jobs is a
  separate cluster.
- Validation for the wave: Build.cmd self-host and publish (the publication
  gate's regression sentinels pass), Tests/run-all.ps1 -Tier fast (57 passed, 0
  failed; the retained corpus stays at 39/122) and Tests/pedantic/run.ps1
  -Group language (25 suites, 577 passed, 0 failures).

Compile-speed wave (2026-09-13, cycle 31)
- Workload A in process: `cpc.exe @build/prof-self.rsp` with the compiler's own
  include set, 12 compilations per invocation, 20 interleaved pairs with the
  order alternated, measured with the unpacked base and candidate builds (same
  sources and flags, so the change under test is the only difference): inner
  time median 3531 -> 3500 ms (-0.88%), mean 3526.6 -> 3492.1 ms (-0.98%);
  wall median 3525 -> 3501 ms (-0.68%), mean 3533.6 -> 3498.2 ms (-1.00%). The
  pairs split 19 wins, 1 loss and 0 ties, with a paired delta median of -31 ms
  and a mean of -34.5 ms. Two ablated rounds attribute the win: the qualifier
  reorder alone measured -0.90% median over 10 pairs (6 wins, 4 losses) and the
  recorded spelling ids another -0.44% median and mean over 14 pairs against
  that build (9 wins, 4 losses, 1 tie), so both parts contribute.
- Profiler evidence: the workload's counters place 945,882 `get_tok_str()` calls
  in one self-compilation. Two pairs of identical sites tested the spelling of
  the callee name against `std::is_` and `__cpc_ns_std_is_` before testing
  whether the next token was `<`; each of those four probe calls ran 112,376
  times per compilation, 449,504 `get_tok_str()` calls and 449,504 `strncmp()`
  calls that a token test could have answered first. About 310,000 more calls
  were context-spelling tests against `static_assert` (148,296), `using`
  (90,572), `typename` (35,826), `operator[]` (21,086), `template` (8,346),
  `friend` (3,689) and `wchar_t` (2,270), each one a `get_tok_str()` plus a
  `strcmp()` on a path that runs for ordinary name tokens.
- Workload B: the six `Tests\benchmarks\compile\` inputs compiled with the
  unpacked base and candidate builds, 24 samples each with the order
  alternated: base -> candidate in ms were c.integer.compile.stress 8.387 ->
  8.441, c.empty.main 8.246 -> 8.377, cpp.call.source.lookup 12.690 -> 12.877,
  test_conversion_template_owner_lookup 17.073 -> 16.989,
  cpp.template.static.lookup 22.181 -> 21.788 and cpp.template.member.list
  11.683 -> 11.778. These inputs run in 8-22 ms and are dominated by process
  start, so they stay inside this host's +/-2% spread for this shape of case.
- Changes kept: `next_nomacro()` records the token id of each of those seven
  spellings as `tok_alloc_new()` interns it (`note_cpp_probed_spelling()`), so
  the declaration and expression paths compare ids instead of spellings. The
  ids are recorded where the source interns the spelling, which keeps identifier
  numbering exactly as the base compiler produced it -- interning the literal at
  the first probe instead renumbers every identifier interned in between, and
  those numbers appear in synthetic `__cpc_bound_type_<n>` names. The two
  `std::is_` qualifier tests now evaluate `tok == TOK_LT || tok == '<'` first,
  which is the condition that has to hold for the probe to matter; that drops
  the four probe calls from 112,376 to 1,806 each.
- Verified codegen-neutral: all 1602 first-party pass-tier sources were compiled
  with the previous and the new compiler, one process per source and the same
  arguments on both sides; the 1595 that both accepted produced byte-identical
  objects and the other 7 failed for both with byte-identical diagnostics. The
  compiled self object is byte-identical, and the published compiler reproduces
  the candidate's objects exactly.
- Considered and not kept: a spelling test against a cached key of the
  identifier table's hash and length for the literal avoided the intern but kept
  a table load and two compares per probe; two rounds measured -1.12% median
  over 10 pairs and -0.68% median over 12 pairs, i.e. inside this host's spread
  of the id compares, so the id table was kept for removing the lookup as well.
  Interning the literal at the first probe and comparing ids renumbered the
  identifiers interned in between: one first-party source's synthetic symbol
  numbers moved by four, which is why the ids are recorded instead.
- Validation for the wave: Build.cmd self-host and publish (the publication
  gate's regression sentinels pass), Tests/run-all.ps1 -Tier fast (57 passed, 0
  failed; the retained corpus stays at 39/122) and Tests/pedantic/run.ps1
  -Group language (25 suites, 577 passed, 0 failures).

Compile-speed wave (2026-09-13, cycle 32)
- Workload A in process: `cpc.exe @build/prof-self.rsp` with the compiler's own
  include set, 12 compilations per invocation, measured with the unpacked base
  and candidate builds (same sources and flags, so the change under test is the
  only difference). Round 1, 8 interleaved pairs with the order alternated:
  inner-time median 3500 -> 3445 ms (-1.57%), mean 3496.1 -> 3439.4 ms
  (-1.62%), 8 wins, 0 losses. Round 2, 12 further pairs: median 3508 -> 3484 ms
  (-0.68%), mean 3510.4 -> 3480.3 ms (-0.86%), 10 wins, 2 losses. Session total
  18 wins and 2 losses over 20 pairs.
- Workload A through the packed pair (previous published compiler saved aside
  -> new published compiler), same response file with the leading `-B.` removed
  because `-B<dir>` replaces the payload's include search: 8 interleaved pairs
  measured a 3532 -> 3508 ms inner-time median (-0.68%) and a 3521.4 ->
  3494.3 ms mean (-0.77%), 4 wins, 3 losses, 1 tie. The packed pair is the
  noisier of the two shapes.
- Profiler evidence: temporary counters put one self-compilation at 199,192
  `unary()` calls, 112,400 of which derived `table_ident[tok - TOK_IDENT]->str`
  and compared it against `typename`, `static_cast` (11 characters),
  `reinterpret_cast`, `const_cast`, `dynamic_cast` and `delete`. That is two
  dependent random loads plus up to three `strcmp()` probes per name token, in
  a C translation unit where none of those spellings can occur. The same
  counters placed the identifier fast path of `next_nomacro()` at ~6.5% of
  samples: an 8-byte-bucket histogram plus never-executed `CPC_LAYOUT_MARK()`
  insertions that shift the following code bracketed it to the scan/hash loop
  and the bucket probe, about 2.5% each. A 256-entry last-identifier cache keyed
  on the first byte (counted, not built) would hit 44.8% of the 334,727
  lookups.
- Changes kept: `note_cpp_probed_spelling()` records the ids of `static_cast`,
  `dynamic_cast`, `const_cast`, `reinterpret_cast` and `delete` as the lexer
  interns them, next to cycle 31's seven spellings, and `unary()` compares `tok`
  against them instead of deriving the spelling. The comparison matches exactly
  where the old `strcmp()` did, including a `.c` file that spells one of them as
  an ordinary identifier, because the id is recorded at the intern the source
  itself performs. The unused `tok_name` local is gone with the probes.
- Verified codegen-neutral: the previous published compiler and the new one were
  compared over all 1602 first-party pass-tier sources, one process per source,
  same arguments on both sides; the 1600 that both accepted produced
  byte-identical objects and the other 2 failed for both with byte-identical
  diagnostics (test_coroutine_headers.cpp needs the coroutine flag,
  test_heap_list_push_clear_perf.cpp lacks the GetTickCount64 prototype).
- Workload B, previous published -> new published compiler,
  BuildProfile.cmd -Iterations 20 -Warmups 3, before -> after in ms:
  cpp.call.source.lookup 14.233 -> 13.734, cpp.template.static.lookup
  22.742 -> 22.227, cpp.template.member.list 13.166 -> 12.721,
  c.integer.compile.stress 10.259 -> 10.181, c.empty.main 10.002 -> 10.046,
  test_conversion_template_owner_lookup 17.879 -> 17.912. The unchanged cases
  sit inside this host's +/-3% small-case spread.
- Considered and not kept: an 8-byte masked compare in the identifier probe
  needs the source read past the identifier, which the file buffer at
  `cprime_open_bf()` does not guarantee; a first-byte cache would remove one
  random load for 44.8% of lookups but leave the hash pass and the `memcmp()`
  untouched, and cycle 27 already measured an inline byte compare there as
  -0.5%.
- Measurement protocol for the next cycle: the codegen-neutrality sweep must run
  the previous published compiler against the new one, because an unpacked
  compiler resolves includes from its own directory and only the packaged
  payload carries the SDK headers; `build/prof-self.rsp` starts with `-B.`,
  which on a packed compiler replaces the payload's include search, so only
  unpacked builds can run it as written; and a bare `cpc.exe` handed to a
  helper resolves through PATH to the unrelated archived compiler.
- Validation for the wave: Build.cmd self-host and publish (the publication
  gate's regression sentinels pass), Tests/run-all.ps1 -Tier fast (57 passed, 0
  failed; the retained corpus stays at 39/122) and Tests/pedantic/run.ps1
  -Group language (25 suites, 577 passed, 0 failures).

Compile-speed wave (2026-09-13, cycle 33)
- Workload A in process: `cpc.exe @build/prof-self.rsp` with the compiler's own
  include set, 12 compilations per invocation, measured with the unpacked base
  and candidate builds (same sources and flags, so the change under test is the
  only difference). Round 1, 10 interleaved pairs with the order alternated:
  inner-time median 3437 -> 3422 ms (-0.44%), mean 3432.8 -> 3420.4 ms
  (-0.36%), 6 wins, 2 losses, 2 ties. Round 2, 12 further pairs: median
  3438 -> 3406.5 ms (-0.92%), mean 3441.5 -> 3412.8 ms (-0.83%), 10 wins, 1
  loss, 1 tie. Session total 16 wins, 3 losses and 3 ties over 22 pairs,
  median 3437.5 -> 3407 ms (-0.89%), mean 3437.5 -> 3416.3 ms (-0.62%) and a
  paired delta median of -23.5 ms.
- Workload A through the packed pair (previous published compiler saved before
  Build.cmd published -> new published compiler), using the same response file
  with the leading `-B.` removed because `-B<dir>` replaces the payload's
  include search: 8 interleaved pairs measured a 3484 -> 3453 ms inner-time
  median (-0.89%) and a 3476.2 -> 3447.1 ms mean (-0.84%), 5 wins, 3 losses.
- Workload B, previous published -> new published compiler,
  BuildProfile.cmd -Iterations 20 -Warmups 3, before -> after in ms:
  c.integer.compile.stress 10.274 -> 10.236, c.empty.main 10.076 -> 10.131,
  cpp.call.source.lookup 14.332 -> 14.362,
  test_conversion_template_owner_lookup 18.439 -> 18.096,
  cpp.template.static.lookup 22.462 -> 22.478, cpp.template.member.list
  13.145 -> 12.795. These inputs compile in 10-22 ms and are dominated by
  process start; every change is inside this host's small-case spread.
- Profiler evidence: `set_idnum()` already derives `ident_cont[256]` from
  `isidnum_table` for the identifier scan. The horizontal-whitespace loop after
  a space or tab still loaded `isidnum_table`, masked `IS_SPC` and compared,
  once per byte of every indentation run. Cycle 33 derives
  `ident_space[256]` in the same writer, with entry c set to 1 while c is
  `IS_SPC`, and `next_nomacro()` tests that byte directly.
- Change kept: classify horizontal whitespace once in `set_idnum()` and use
  the derived byte in the tokenizer's whitespace run loop. `set_idnum()` is the
  only writer of `isidnum_table`, so the two stay in step, and the loop stops on
  exactly the same bytes as the old mask.
- Considered and not kept (all reverted): growing the identifier table at 50%
  load (-0.47% median/mean slower); a 1024-entry direct-mapped front lookup
  cache (-0.48% median, -0.72% mean) despite 84% exact hits, with the measured
  hit rates 77% at 256 entries and 89% at 16384 entries; shrinking the final
  table to a 2x load (-1.38% median, -1.55% mean); a final XOR bucket-index
  mix (neutral: 5 wins, 5 losses, -0.06% mean); collapsing space/tab runs in
  `preprocess_skip()` (-0.45% median, -0.46% mean); and a collision-flag scheme
  that skipped the identifier `memcmp()` for unambiguous tokens, which was
  unsound for the first occurrence of a colliding spelling and was not kept.
- Verified codegen-neutral: all 1602 first-party pass-tier sources were
  compiled with the previous published and the new published compiler, one
  process per source and the same arguments on both sides; the 1600 that both
  accepted produced byte-identical objects and the other 2 failed for both with
  byte-identical diagnostics.
- Validation for the wave: Build.cmd self-host and publish (the publication
  gate's regression sentinels pass), Tests/run-all.ps1 -Tier fast (57 passed, 0
  failed; the retained corpus stays at 39/122) and Tests/pedantic/run.ps1
  -Group language (25 suites, 577 passed, 0 failures).

Compile-speed wave (2026-09-13, cycle 34)
- Workload A in process: `cpc.exe @build/prof-self.rsp`, same protocol as the
  previous cycles, with the published cycle-33 compiler saved before any build.
  Nine leads were compiled and measured; none beat the base repeatably, so all
  nine compiler changes were reverted and the published `cpc.exe` is unchanged.
- Rejected measurements, base -> candidate, all 8 or 10 interleaved pairs:
  word-at-a-time `SValue` swap 3437.6 -> 3539.0 ms (+2.94%, 0 wins);
  dropping the tokenizer's per-byte `c` store neutral;
  `ST_INLN` on `g()` 3435.5 -> 3480.5 ms (+1.31%);
  recording the `__is_`/`__has_` type-trait spellings as ids 3451.8 ->
  3481.2 ms (+0.85%);
  four-way identifier-scan unroll 3445.3 -> 3474.6 ms (+0.85%);
  C++ expression-spelling gate neutral;
  unrolled short-spelling compare 3450.3 -> 3489.2 ms (+1.13%);
  folding the first hash step neutral;
  loading the text section once in `g()` 3453.2 -> 3470.3 ms (+0.50%).
- Temporary counters put one self-compilation at 334,941 identifier lookups,
  26.7% keyword tokens and 90.0% matched by a full-spelling compare. Identifier
  lengths were roughly bimodal: 34% at 1-4 bytes, 35% at 5-8 bytes and 31% at
  9+ bytes. The `next_nomacro()` leaf is 11.6% of samples; its heaviest 32-byte
  buckets are the scan/hash loop and the bucket probe, so the next win still
  has to remove a pass or a lookup there rather than shorten the compare.
- Retained change: `src/tools/profile_process.c` now parses its optional
  `-hist` range with `strtoull(..., 0)` instead of `sscanf` with `%llx`; under
  the CPC runtime the latter left the range empty. The rebuilt profiler was
  used for the histogram above.
- No codegen-neutrality sweep was needed this cycle because the compiler binary
  and compiler sources are unchanged.
- Validation with the unchanged root compiler: Tests/run-all.ps1 -Tier fast
  (57 passed, 0 failed; the retained corpus stays at 39/122) and
  Tests/pedantic/run.ps1 -Group language (25 suites, 577 passed, 0 failures).

Compile-speed wave (2026-09-13, cycle 37)
- No compiler change was kept, and the wave is closed. The last named lead was
  re-measured with `build\profile-process.exe build\compiler\cpc.map
  build\compiler\cpc.exe build\prof-self.rsp` (12 compilations of the
  compiler's own source): leaf share 11.4% `next_nomacro`, 5.5% `unary`, 3.2%
  `parse_btype`, 2.8% `next`, 2.3% `g`, 1.9% `vswap`; unmapped leaf samples sit
  in the ntdll heap region (1.40%) and ucrtbase `memset` (1.24%).
- The same run with `-hist 0x40a98e 0x16f7`, the extent of `next_nomacro` from
  the map, puts 7.8% of the whole run in the 288 bytes at 0x40ad6e..0x40ae8e,
  which is the identifier scan/hash loop plus the bucket probe. That matches
  cycle 33's bisection of the same block into scan/hash 2.5% and probe 2.5%.
- Why that block cannot be shortened further here: the loop is bounded by this
  codegen keeping `p`, `h` and the current byte in stack slots (no register
  allocation across statements), and every struct copy is emitted as
  `rep movsq` -- a `-S` listing of `vswap()` shows `movl $13, %ecx` for the 13
  qwords of `sizeof(SValue)` = 104 (array stride 112, `sizeof(Sym)` = 224) --
  so cheaper copies or fewer passes over `SValue` are backend work, not
  front-end work. Rejected on that basis without rebuilding, because each one
  re-tries a measured rejection: prefix/sampled hashes (cycle 27), caches keyed
  on first byte, length or hash (cycles 33-34), table load factors in both
  directions (cycle 33), identifier-scan unrolling, an unrolled short-spelling
  compare, dropping the per-byte classification byte and the per-byte `c`
  store (cycle 34), and replacing `memcmp` (cycle 26).
- Wave record: cycles 19-33 landed thirteen measured changes (self-compile
  3445 ms inner median for 12 compilations at the end of cycle 34); cycles 34
  and 37 measured twenty leads between them and kept none. The published
  `cpc.exe` is byte-identical to the cycle-34 one.
- Retained-row work this cycle (a repaired row is deleted once its behavior is
  in first-party coverage): the `-fcoroutines` support added earlier already
  makes 39 of the 42 retained coroutine rows compile, so six new first-party
  tests were written and nine rows retired -- `test_coroutine_await_local_
  argument.cpp` (`pr95822.C`, `pr95823.C`, `pr95824.C`),
  `test_coroutine_move_only_by_value_parameter.cpp` (`pr95350.C`),
  `test_coroutine_promise_type_alias_wrapper.cpp` (`pr95346.C`),
  `test_coroutine_body_local_function_declaration.cpp`
  (`coro-function-decl.C`), `test_coroutine_throwing_return_object_
  destructor.cpp` (`pr102051.C`), `test_coroutine_yield_conditional_
  temporary.cpp` (`pr109283.C`), plus `coro-pre-proc.C`, which
  `test_coroutine_headers.cpp` already covers.
- Validation with the unchanged root compiler: Tests/run-all.ps1 -Tier fast
  (0 regressions, the retained corpus is 113 rows, 113/113 evaluated, 30
  PASS_COMPILE + 83 FAIL_COMPILE) and the Declarations suite at 17 passed.

Retained-row work (2026-09-13, cycle 38)
- No compiler change was kept; the published cpc.exe is unchanged.  The 30
  remaining retained g++.dg/coroutines rows that -fcoroutines already compiled
  were reduced to twelve first-party tests in
  Tests/features/Declarations/pass/ and the rows and their corpus.json entries
  were deleted.  The now-unused coro.h and coro1-ret-int-yield-int.h support
  headers were retired with them.  The corpus is now 83 rows, all retained
  FAIL_COMPILE; g++.dg/coroutines/pr113457.C is the only coroutine row left.
- Mapping: test_coroutine_return_move_only_operands.cpp
  (co-return-syntax-10-movable.C);
  test_coroutine_return_value_shapes.cpp (pr112341-3.C, pr116880.C,
  pr95591.C); test_coroutine_promise_constructor_preview.cpp
  (pr104981-preview-this.C, pr115550-preview-this.C, pr94682-preview-this.C);
  test_coroutine_lambda_capture_shapes.cpp (pr116327-preview-this.C,
  pr103328.C, pr96517.C, pr96251.C);
  test_coroutine_await_expression_shapes.cpp (pmf-121094.C, pr111728.C,
  pr112341.C, pr116502.C, pr116793-1.C, pr121643.C);
  test_coroutine_yield_await_control_flow.cpp (pr94886-folly-3.C, pr98480.C,
  pr95345.C, pr99575.C, pr95050.C, pr113457-1.C);
  test_coroutine_promise_static_functions.cpp (pr109682.C, pr95440.C);
  test_coroutine_traits_function_object.cpp
  (pr94760-mismatched-traits-and-promise-prev.C);
  test_coroutine_traits_primary_inheritance.cpp (pr94883-folly-2.C);
  test_coroutine_parameter_lifetime.cpp (pr98118.C);
  test_coroutine_optional_promise_value.cpp (pr100127.C); and
  test_coroutine_symmetric_transfer_task.cpp (pr99047.C).
- Validation with the unchanged root compiler: Tests/run-all.ps1 -Tier fast
  (all first-party suites passed with 0 failures; the Declarations suite ran
  29 passed; the retained corpus is 83 rows, 83/83 evaluated, 0 PASS_COMPILE
  + 83 FAIL_COMPILE, 0 regressions), Tests/pedantic/run.ps1 -Group language
  (577 passed, 0 failed), and the five retained-corpus adapter checks.  The
  corpus gate exits nonzero because every remaining row is an unresolved
  failure, which is the expected state after all repaired passes were retired.

Retained-row work (2026-09-13, cycle 39)
- Retired g++.dg/template/conv1.C (PR 4361, template conversion operators were
  not overloaded).  A qualified conversion-operator name hides its member
  template argument list inside the conversion-type-id, so
  `&First<D>::operator First<B>` reached the Class<T>:: member path with
  `operator` itself as the member name and reported the synthetic
  `First__D_operator`; the plain-class path built the spelled target
  `D_operator Second < int >` but never instantiated the member template, so
  the symbol did not exist.
- Change (src/compiler/frontend/cprimegen.c only): both qualified member paths
  now parse the whole operator name after `::` instead of treating `TOK_OPERATOR`
  as the member, take the written conversion target type, deduce the conversion
  member template against it with the existing
  instantiate_cpp_conversion_templates(), and publish the selected
  specialization under the spelled target name a non-template conversion
  function already uses.  The class-template path keeps the tokens the parse
  already consumed, so it no longer calls next() twice.
- Coverage: Tests/features/Templates/pass/
  test_conversion_template_member_address.cpp (contextual member-pointer types
  pin the selected specialization, direct calls pin the values) and
  test_conversion_template_member_address_declarations.cpp (EXPECT_COMPILE_ONLY,
  keeps the upstream shape: incomplete Second, undefined conversions and a
  First<T>::Foo method beside them).
- Validation: Tests/run-all.ps1 -Tier fast (59 first-party passed, 0 failed,
  0 regressions; the retained corpus is 82 rows, 82/82 evaluated, 0
  PASS_COMPILE + 82 FAIL_COMPILE) and Tests/run-all.ps1 -Tier pedantic (26
  suites passed; the language group ran 577 passed, 0 failed).
- Found outside the target: calling through a pointer to member that names a
  member of a class-template instantiation (`&A<int>::plain`) or of a
  member-function-template specialization (`&D::add<int>`) returns garbage,
  while the same shape on a plain class is correct.  Both reproduce with the
  cycle-38 compiler saved before this cycle's build, so the defect predates the
  row repair; the row is compile-only, so it was not repaired here.

Retained-row work (2026-09-13, cycle 41)
- Retired g++.old-deja/g++.jason/synth7.C.  An implicit copy assignment was
  usable inline (`a = b`) but had no function symbol, so `&A::operator=`
  resolved the synthetic `A_operator=` name and reported it undeclared.
- Change (src/compiler/frontend/cprimegen.c and
  src/compiler/frontend/cprimegen_member_pointers.inc): the member-address path
  now materializes the implicit copy assignment before lookup.  The helper is
  limited to classes with no user-declared copy assignment, no user-declared
  move constructor or move assignment, and memberwise-assignable data and
  bases.  It declares the function through declare_member_func() and queues
  the memberwise body with queue_defaulted_assignment_body(), the same path
  used for `= default`.
- Coverage: Tests/features/OperatorOverloads/pass/
  test_implicit_copy_assignment_address.cpp takes `&Owner::operator=` in a
  pointer-to-member declaration and checks the memberwise copy through the
  pointer.
- Validation: Tests/run-all.ps1 -Tier fast (all 17 first-party suites passed
  with 0 regressions; the gcc suite reported the expected 81/81 FAIL_COMPILE)
  and Tests/run-all.ps1 -Tier pedantic (26 suites passed; the language group
  ran 577 passed, 0 failed).  No compiler-speed change was attempted.

Retained-row work (2026-09-13, cycle 45)
- Retired g++.dg/template/access28.C.  A static member function template
  declared inside its class and defined out of line is two TemplateMemberDef
  records that instantiate the same specialization, so
  `&grac::once<Derived>` looked ambiguous to the address path, which has no
  call to rank and required exactly one candidate; the address was reported as
  "no matching static member-template call".
- Change (src/compiler/frontend/cprimegen_templates.inc only): the address
  branch of resolve_qualified_static_template_call() resolves every candidate
  to its function symbol and accepts the set while the symbols agree; two
  distinct functions still fail.  The call branch, which ranks candidates by
  argument types and already saw the duplicate records, is unchanged.
- Coverage: Tests/features/Templates/pass/
  test_static_member_template_address.cpp takes the address in a
  class-template member-initializer, calls through the stored pointer and keeps
  the row's has_R<T> SFINAE overload selection over the ellipsis fallback.  The
  test fails on the cycle-43 published compiler with the row's diagnostic and
  passes on the new one.
- Codegen-neutrality sweep, previous published compiler (saved before Build.cmd
  published) against the new one, one process per source and the same
  arguments on both sides over all 1625 first-party pass sources: 1622
  accepted by both produced byte-identical objects, the other 2 failed for both
  with byte-identical diagnostics (test_coroutine_headers.cpp needs the
  coroutine flag, test_heap_list_push_clear_perf.cpp lacks the GetTickCount64
  prototype), and the only status difference is the new regression test, which
  the base compiler rejects with the row's diagnostic.
- Validation: Tests/run-all.ps1 -Tier fast (61 first-party cases passed, 0
  failed, 0 regressions; the retained corpus is 78 rows, 78/78 evaluated, 0
  PASS_COMPILE + 78 FAIL_COMPILE) and Tests/pedantic/run.ps1 -Group language
  (25 suites passed, 577 passed, 0 failed).  No compiler-speed change was
  attempted.

Retained-row work (2026-09-13, cycle 46)
- Retired g++.dg/template/canon-type-3.C.  `typedef Y (FP) ();` declares FP
  with Y as its return type; the class-body generator that assembles the
  instantiated definition took the last identifier at declarator depth zero,
  which is Y, and published it under the instantiation's joined name
  (`E__int__Y`).  The replayed definition then declared no type, so its
  declarator fell back to the implicit-int path and the instantiation failed
  with "function parameter type expected (got 'FP')".
- Change (src/compiler/frontend/cprimegen_templates.inc only): a new
  template_body_declares_type_name() answers whether the identifier before a
  `(` names a type the replay can resolve (a template type parameter, a
  specialization alias, a typedef, a class/enum, or a class template).  Both
  typedef scans -- materialize_template_class_typedef() and the class body
  builder in template_specialized_body() -- then take the group's own
  identifier as the declared name for `typedef T (name) ();` and keep the
  depth-zero name when the group is a parameter list (`typedef int name(Arg);`).
  The declared name is now mangled and registered like any other member
  typedef, so `E<int>::FP` resolves from outside the class.
- Coverage: Tests/features/Templates/pass/
  test_parenthesized_function_typedef_member.cpp instantiates the row's
  `E<Y>` (with the incomplete `A<T>` default argument of `B`) plus a
  member-typedef-typed variant, names `E<int>::FP` and `D<int, char>::FP` from
  outside, and calls through both pointers.  The test fails on the cycle-45
  published compiler with the row's diagnostic and passes on the new one.
- Codegen-neutrality sweep, previous published compiler (saved before Build.cmd
  published) against the new one, one process per source and the same
  arguments on both sides over all 1626 first-party pass sources: 1623
  accepted by both produced byte-identical objects, 2 failed for both with
  byte-identical diagnostics (test_coroutine_headers.cpp needs the coroutine
  flag, test_heap_list_push_clear_perf.cpp lacks the GetTickCount64
  prototype), and the only status difference is the new regression test, which
  the base compiler rejects with the row's diagnostic.
- Validation: Tests/run-all.ps1 -Tier fast (62 first-party cases passed, 0
  failed, 0 regressions; the retained corpus is 77 rows, 77/77 evaluated, 0
  PASS_COMPILE + 77 FAIL_COMPILE) and Tests/pedantic/run.ps1 -Group language
  (25 suites passed, 577 passed, 0 failed).  No compiler-speed change was
  attempted.

Retained-row work (2026-09-13, cycle 49)
- Retired g++.old-deja/g++.ext/anon3.C.  An out-of-class static data member
  definition is copied into the member's class scope before it is parsed, and
  that copy in try_parse_cpp_scoped_member_def() ended at the first `;` at any
  nesting depth.  An initializer that creates a new type puts a `;` inside the
  type's own body, so the copied declaration was truncated and the replay
  reached the copy's EOF inside the group: `unexpected end of file` with the
  scan still two groups deep.  The row's
  `(__extension__ ((union { ... }){ __c: { ... } }).__d)` static initializer
  stopped at the same `;`.
- Change (src/compiler/frontend/cprimegen.c only): the copy loop tracks
  `(`/`[`/`{` groups and ends at the first `;` outside every group, which is
  the definition's own terminator.  Declarations whose initializer has no
  grouped `;` copy exactly the tokens they copied before; the new
  depth-tracking only extends a copy that used to stop inside a group, where
  the following replay always failed.
- Coverage: Tests/features/Classes/pass/
  test_static_member_initializer_defines_type.cpp checks the row's infinity
  constant, a `sizeof (struct { unsigned char bytes[8]; double value; })`
  member and a lambda-bodied member at run time, and pins that the
  initializer's new type is not injected into the class (sizeof of the class
  with only static members stays 1).  The test fails on the cycle-48 published
  compiler with the row's diagnostic and passes on the new one.
- Codegen-neutrality sweep, previous published compiler (the cycle-48 cpc.exe
  taken from HEAD, packed, so it resolves its own include set) against the new
  one, one process per source with the same arguments over all 1585 first-party
  pass sources: the 1582 that both accepted produced byte-identical objects,
  two failed for both with the same diagnostics modulo the compiler's own name
  (test_coroutine_headers.cpp needs the coroutine flag,
  test_heap_list_push_clear_perf.cpp lacks the GetTickCount64 prototype), and
  the only status difference is the new regression test.
- Validation: Tests/run-all.ps1 -Tier fast (62 first-party cases passed in the
  features suites, 0 failed, 0 regressions; the retained corpus is 74 rows,
  74/74 evaluated, 0 PASS_COMPILE + 74 FAIL_COMPILE) and
  Tests/run-all.ps1 -Tier pedantic (26 suites passed; the language group ran
  577 passed, 0 failed).  No compiler-speed change was attempted.

Retained-row work (2026-09-13, cycle 50)
- Retired g++.old-deja/g++.martin/sts_iarr.C.  Both class-template body
  replays redirected every identifier that named a member of the
  instantiation being replayed to its joined symbol name, including the
  terminal name of a qualified id: inside Outer<2>'s member body
  `typename Outer<N-1>::Inner` kept the qualifier (Outer<1>) but spelled its
  member as Outer<2>'s `Outer____cpc_template_const_2_Inner`, so the lookup
  reported `nested template type member ... must be a typedef`.
- Change (src/compiler/frontend/cprimegen_templates.inc): the class-body
  member-body rewrite (instantiate_template_if_needed()) passes no alias table
  for tokens template_body_name_is_qualified() reports after a `::`, and
  tok_str_add_template_member_subst_scoped() drops the joined spelling for
  those tokens after the probe has run.  Parameters, the template's own name
  and every unqualified member name substitute exactly as before, and the
  joined-name probe still interns the same tokens in the same order.
- Coverage: Tests/features/Templates/pass/
  test_qualified_nested_class_in_member_body.cpp runs the row's nested
  `operator[]` chain down to Outer<1>::Inner (Outer<2> and Outer<3> bodies),
  checks the injected class name qualified through the current instantiation,
  pins the member-typedef alias shape (rejected as a typedef-name redirect
  before this fix) and a member function template of the class template.
  The test fails on the cycle-49 published compiler with the row's diagnostic
  and passes on the new one.
- Codegen-neutrality sweep, previous published compiler (the cycle-49 cpc.exe
  taken from HEAD, packed, so it resolves its own include set) against the new
  one, one process per source with the same arguments over all 1586 first-party
  pass sources: the 1583 that both accepted produced byte-identical objects,
  two failed for both with the same diagnostics modulo the compiler's own name
  (test_coroutine_headers.cpp needs the coroutine flag,
  test_heap_list_push_clear_perf.cpp lacks the GetTickCount64 prototype), and
  the only status difference is the new regression test.  An earlier revision
  of the change skipped the joined-name probe for qualified names and shifted
  the interned token ids that spell `__cpc_bound_type_<n>` in synthesized
  names; running the probe unconditionally keeps that numbering, so the
  objects stay byte-identical.
- Validation: Tests/run-all.ps1 -Tier fast (63 first-party cases passed in the
  features suites, 0 failed, 0 regressions; the retained corpus is 73 rows,
  73/73 evaluated, 0 PASS_COMPILE + 73 FAIL_COMPILE) and
  Tests/run-all.ps1 -Tier pedantic (26 suites passed; the language group ran
  577 passed, 0 failed).  No compiler-speed change was attempted.
