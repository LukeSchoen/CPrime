# Self-hosted code-generation results — 2026-09-07

The optimized self-hosted compiler builds OTServ in **22.105 seconds** on
average, compared with **24.582 seconds** for the same compiler sources built
without the new optimizations. This saves **2.477 seconds (10.08%)** end to end.
The historical Clang-host result was 14.75 seconds; the final installed
Clang-host check measured 15.238 seconds (an earlier check in this session
measured 14.672 seconds). This change recovers part of the gap, not all of it. The earlier historical self-host result was 25.04
seconds; the table below uses a fresh, alternating comparison on current code.

| Pair | CPC host built at `-O0` | CPC host built at `-O2` |
| --- | ---: | ---: |
| 1 | 24.616 s | 22.172 s |
| 2 | 24.699 s | 22.071 s |
| 3 | 24.431 s | 22.072 s |
| Average | 24.582 s | 22.105 s |
| Compiler processes, average | 22.791 s | 20.292 s |

These are external wall-clock measurements around `cmd /c
build_prime_unity.cmd -CPrimeRoot <measurement directory>`. The directory holds
a copy of the unchanged build driver and the selected compiler executable.
Preparation, manifest generation, compilation, resources, archiving, linking,
and shell startup are included. The comparison alternates hosts, serially.

Every run rebuilt the same 215 sources in the same 19 translation units.
OTServ retained **`-O0`, `-Werror`, the same inputs, include paths, and argument
order**. All runs produced the executable with zero warnings. No object/PCH
reuse or parallel compilation was introduced. Compiler state is fresh for
each translation unit, including the optimizer's inline-body records.

Executable machine-code sections of all 19 selected objects match exactly
between both compiler hosts across all six runs and the final Clang-host build. The optimized compiler's
second and third bootstrap generations also have identical executable-section
hashes (the complete portable binaries also match). CPrime, rather than Clang, generated the measured host's machine code.

One attempted final run stopped before compilation because Windows reported a
file lock on `compile_inputs.json`. No other compiler process was running when
checked. That failed attempt is excluded; the third pair was rerun successfully.

## What changed

- A bounded Win64 backend pass promotes ordinary scalar stack slots into
  registers. Leaf functions use otherwise unused volatile registers; eligible
  C functions can retain frequently used values across calls in R12–R15.
  Their save/restore instructions have corresponding Windows unwind records.
- Register copy propagation, backwards liveness, dead register-move removal,
  and folding of materialized boolean tests reduce instruction count. For
  eligible C functions, the pass compacts code and updates branches and
  relocations before generating the epilog and unwind information. Unneeded
  reserved prolog space is removed.
- Small, eligible emitted function bodies can be inserted at direct call
  sites. Small C `inline` definitions become available early only when doing
  so cannot introduce previously unused external references. `noinline`,
  `__noinline__`, weak symbols, and `-fno-inline` are respected.
- Fixed structure copies, `memcpy`, and constant-fill `memset` of up to 128
  bytes use short scalar loads/stores. These avoid string-engine startup,
  saving RSI/RDI, and library calls. `-fno-builtin` disables recognition of
  ordinary library calls.
- Constant multiplication uses immediate `imul`; integer comparison with
  zero can use `test`.
- Constructor/destructor symbol construction now uses exact-length names
  instead of a 512-byte formatting buffer. Long names previously collided,
  producing different and incorrect lifecycle bodies under self-hosting.
  The regression covers default, converting, and copy constructors plus
  destruction of a class with a name longer than 512 bytes. This also removes
  repeated fixed-buffer formatting work; both timing columns include the fix.
- The existing Windows self-host build selects `-O2` for the compiler host.
  Runtime bootstrap compilation retains its existing flags. Normal root
  `build.cmd` still installs the Clang-built host when bundled Clang exists.

The optimizer caps functions at 4 KiB / 1,024 instructions, tracks at most 64
stack slots, caps inline bodies at 192 bytes, and bounds liveness iteration.
It uses reusable scratch storage without retaining compilation results.
Increasing the function limit to 16 KiB did not give a useful timing gain and
was discarded. Other deliberately conservative cases include unsupported
instruction encodings, address escapes, narrow/overlapping slots, volatile
accesses, assembly, variadic functions, VLA stack manipulation, and large
probed frames. Nonvolatile promotion and code compaction do not alter C++ EH
frames. Register promotion and automatic inlining are disabled with `-g`
until debug variable-location tracking supports them.

The unwind format follows Microsoft's
[x64 exception-handling specification](https://learn.microsoft.com/en-us/cpp/build/exception-handling-x64?view=msvc-170).

## Cost and validation

Using the optimized self-host to rebuild the compiler itself, the three
`-O0` measurements were 0.388/0.297/0.296 seconds, and `-O2` measurements were
0.365/0.373/0.370 seconds. The steady samples show about 70–80 ms of extra work
to generate better code, versus about 2.48 seconds saved on an OTServ build.
These short measurements are naturally more sensitive to startup variation.

- **1,152 tests passed** across 18 suites with `-O2` enabled, using both the
  optimized self-host and the newly installed Clang-built host.
- The two existing failures remain: `test_converted_pointer_const_reference.cpp`
  (overloaded `read`) and `test_cpp_style_all_in_one.cpp` (overloaded `malloc`).
- `Tests/check_fast_codegen.py` verifies actual object relocations, inlining
  and its opt-outs, library-call elimination and its opt-out, runtime results,
  debug-mode behavior, and fresh state across an optimized batch. A boundary
  regression verifies that stack-frame alignment cannot bypass the 4 KiB
  stack-probe guard.
- New runtime tests cover small copy/fill boundaries and return values,
  argument side effects, aggregate copies, loops, mixed-width values, address
  escapes, volatile data, variadic calls, and unused inline declarations.
  The existing packed argument-field test now explicitly runs at `-O2`.
- `test_fast_nonvolatile_unwind.c` uses `RtlVirtualUnwind` across an optimized
  frame and checks restoration of R12–R15 and RBP.
- 359 generated arithmetic/loop functions produce the same checksum at
  `-O0`, `-O2`, `-O2 -g`, and `-O2 -fno-inline`.
- Batch failure handling and build-manifest/COFF/resource/link compatibility
  checks pass with the optimized self-host. `git diff --check` passes.

No live-server gameplay test or restart was performed; this task changes
compiler code generation and measures fresh builds.

## Artifacts

- Optimized portable self-host: `C:\Luke\Src\CPrime\build\cpc-selfhost.exe`
  (2,219,202 bytes), SHA-256
  `0a86a3d005eb2479ca3d91e92450071107826b50683280ee25e4f51251d5dc80`.
- Installed Clang-built host: `C:\Luke\Src\CPrime\cpc.exe`, SHA-256
  `8357a110252f622836c34a38c5bce16beb0608d2aca2d68efba7cc9329092fd5`.
- Produced server: `C:\Luke\Src\OT\cl\builds\OTServ_prime.exe`
  (16,474,112 bytes).
- Compiler bootstrap executable-section SHA-256 for both optimized
  generations:
  `a5a2d8e0a378f3f60a9eb78e694ce7f82c32645270d68ba4d821523a93aaf5c7`.
- Alternating measurements and selected-object hashes:
  `C:\Luke\Src\OT\intermediate\prime\quality-comparison.json`.
- Final Clang-host measurement: `intermediate\prime\quality-final-clang.json`.
- Full build logs: `intermediate\prime\quality-{baseline,optimized}-{1,2,3}.log`.
- Test logs: `CPrime\build\quality-O2-*.log`,
  `quality-check_batch_build.ps1.log`, and `quality-check_build_manifest.ps1.log`.
- Compiler build-cost samples: `CPrime\build\quality-optimization-cost.json`.
