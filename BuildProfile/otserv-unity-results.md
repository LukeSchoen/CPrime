OTServ unity build results — 2026-09-07

The exact command `C:\Luke\Src\OT\build_prime_unity.cmd` completed three
consecutive builds in **14.775, 14.747, and 14.742 seconds**. These are external
wall-clock measurements around `cmd /c build_prime_unity.cmd`, including
PowerShell startup, project preparation, manifest regeneration, compilation,
resource generation, archiving, and linking.

| Run | Whole command | Compile phase | All CPrime processes |
| --- | ---: | ---: | ---: |
| 10 | 14.775 s | 12.504 s | 12.952 s |
| 11 | 14.747 s | 12.396 s | 12.852 s |
| 12 | 14.742 s | 12.462 s | 12.897 s |

All 215 source files were compiled on every run, in the original 19 unity/
isolated translation units, with the same input order, working directories,
and compiler flags. The selected Debug configuration still passes `-O0` and
`-Werror`. Every build produced `C:\Luke\Src\OT\cl\builds\OTServ_prime.exe`
with zero warnings. Selected objects and the executable are removed before
building; there is no incremental object or precompiled-header reuse.

Compilation remains serial. The existing batch mode creates and destroys a
complete compiler state for each translation unit. Batching removes process
startup overhead; it does not share parsed headers or template state between
units. The batch driver verifies completion records and fresh object files,
enforces a timeout for each unit, preserves diagnostics, and fails on crashes
or incomplete output.

These timings include an optimized CPrime host built by the bundled Clang with
`-O3`. Clang builds CPrime itself; CPrime performs all OTServ compilation and
linking. The normal CPrime `build.cmd` rebuilds the target runtime, builds this
optimized host when bundled Clang is present, packages it, and installs
`cpc.exe`. Compiler source changes also substantially improve the self-hosted
compiler: the two large server units measured approximately 4.44 and 3.99
seconds there, versus roughly 25 and 15.5 seconds in the original report.

The original `source_0108` access violation came from storing an in-class
static integral constant's value in `Sym.c`, which is also the object-symbol
index. An out-of-class definition then treated a value such as 3000 as a
symbol index and corrupted symbol storage. Constant values now remain in the
constant-value fields; actual definitions allocate and initialize real storage.
Regression coverage includes positive, negative, zero, and wide constants,
address-taking before definitions, and volatile runtime reads.

The main performance changes replace repeated global member/template scans
with indices, deduce each overload family once per resolution, grow identifier
and type tables geometrically, avoid rescanning consumed template work items,
and reduce repeated token rewriting and allocator searches. A variadic type
identity bug exposed by verification was also fixed: comparisons must use
actual argument counts, including empty packs and every pack element.

Final verification of the installed compiler:

- 1,117 feature tests passed across 16 suites. The same two pre-existing
  failures remain: `Functions/test_converted_pointer_const_reference.cpp`
  and `All/test_cpp_style_all_in_one.cpp`. Both also fail with the original
  compiler; they are not recorded as passing tests.
- `Tests/check_build_manifest.ps1` passed, including effective settings,
  warning policy, spaced paths, resources, archive ordering, native COFF,
  selected SDK/toolset libraries, unity, and legacy manifests.
- `Tests/check_batch_build.ps1` passed forced crash, timeout, nonzero exit,
  missing object, incomplete protocol, and fresh-state runtime checks.
- The formerly crashing unity unit passed 20 fresh-process compilations with
  varied environment sizes after the final compiler rebuild.

Feature suites used `Tests/run.ps1` with the OT
`cl/builds/manifest/Release-x64.json` test manifest. This feature-test manifest
is separate from the Debug manifest used for the timed OTServ builds.
No live-server gameplay validation is claimed by these build measurements.

Recorded compiler SHA-256:
`9a494b98eafaac83282d1dc274edcd6e88d9bd17513d447c55823c28c57bd2b3`.
Timing logs and metrics are under OT's `intermediate/prime/production-build-10.log`
through `production-build-12.log` and corresponding `production-wall-*.json`.
The original report's 80.958 seconds was a failed compile phase, not a
successful end-to-end baseline. Earlier intermediate revisions sometimes
exceeded 15 seconds; the table above records the final revision on this machine.
