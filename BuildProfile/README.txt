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
