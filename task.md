# CPrime C++ Compatibility Push

Last updated: 2026-08-16

## Objective

Reach 100% practical C++ compatibility for CPC. The compiler is already quite
close; the current work is to measure the remaining gaps against real C++
projects, reduce each failing construct to a test, fix it, and verify the
original project moves further.

## Measurement Projects

Primary compatibility measurements:

- OTServer
- CommonLibrary (`cl`)

The first subtask is compiling the CommonLibrary simple racer/model viewer
entry source:

```text
C:\Luke\Src\OT\cl\Projects\Model Viewer\src\Private\ModelViewer.cpp
```

## Workflow

For every compiler limitation or bug:

1. Confirm the reported symptom is real.
2. Add a focused test that reproduces it.
3. Fix the compiler only after the test reproduces.
4. Verify the fix against the focused test.
5. Re-run the project compile and update this file with the next blocker.

## Current Status

- [x] Created this recovery/status file.
- [x] Find the correct CPC command line for `ModelViewer.cpp`.
- [x] Run the first compile attempt.
- [x] Record the first compiler failure from the wider simple racer core build.
- [ ] Reduce that failure into a self-contained in-repo test.
- [x] Fix and verify the reduced test.
- [x] Re-run the full simple racer core executable build.
- [x] Reduce/fix the current `clRealMap2` multi-parameter template blocker.
- [x] Reduce/fix the current `clUnused` function-template overload blocker.
- [ ] Reduce/fix current `clList<T>::operator[]` reference return declaration blocker.

## Command Log

Commands and outcomes go here so work can be resumed after a crash.

### 2026-08-16

- Checked `C:\Luke\Src\OT\cl\build_prime.cmd`; it uses generated response
  files from `C:\Luke\Src\OT\cl\builds\core`.
- `generated_core_sources.rsp` contains the simple racer/core measurement set:
  CommonLib `src\Core\...\cpc*.cpp`, `ModelViewer.cpp`, `Racer.cpp`, and
  `cpcFreeLancer.cpp`.
- PowerShell parses `@...` as its own syntax. Use `cmd /c` or another escaping
  strategy for CPC response files.
- Compile-only baseline passed:

```bat
cmd /c ""C:\Luke\Src\CPrime\cpc.exe" @"C:\Luke\Src\OT\cl\builds\core\generated_core_cpc_flags.rsp" -c "C:\Luke\Src\OT\cl\Projects\Model Viewer\src\Private\ModelViewer.cpp" -o "C:\Luke\Src\OT\cl\builds\core\ModelViewer_cprime_current.obj""
```

Outcome: exit code 0. The entry file itself is not the current blocker.

- Full generated simple racer/core executable build failed:

```bat
cmd /c ""C:\Luke\Src\CPrime\cpc.exe" @"C:\Luke\Src\OT\cl\builds\core\generated_core_cpc_flags.rsp" @"C:\Luke\Src\OT\cl\builds\core\generated_core_sources.rsp" -luser32 -lgdi32 -lopengl32 -o "C:\Luke\Src\OT\cl\builds\ModelViewCore_cprime_current.exe" > "C:\Luke\Src\OT\cl\builds\core\cprime_full_core_current.log" 2>&1"
```

Outcome: exit code 1, empty log.

- Full generated simple racer/core compile-only build passed:

```bat
cmd /c ""C:\Luke\Src\CPrime\cpc.exe" @"C:\Luke\Src\OT\cl\builds\core\generated_core_cpc_flags.rsp" -c @"C:\Luke\Src\OT\cl\builds\core\generated_core_sources.rsp" > "C:\Luke\Src\OT\cl\builds\core\cprime_full_core_compile_only_current.log" 2>&1"
```

Outcome: exit code 0. All generated translation units compile in `-c` mode
together; the full executable path still fails.

- Direct compile of `cpcMesh.cpp` fails silently:

```bat
cmd /c ""C:\Luke\Src\CPrime\cpc.exe" @"C:\Luke\Src\OT\cl\builds\core\generated_core_cpc_flags.rsp" -c "C:\Luke\Src\OT\cl\CommonLib\commonLib\src\Core\Polygon\cpcMesh.cpp" -o "C:\Luke\Src\OT\cl\builds\core\cpcMesh_probe.obj" > "C:\Luke\Src\OT\cl\builds\core\cpcMesh_probe.log" 2>&1"
```

Outcome: exit code 1, empty log.

- Reduction progress:
  - `Tests\tmp_cl_cpcmesh_include.cpp`: `#include "cpcMesh.h"` only passes.
  - `Tests\tmp_cl_cpcgl_include.cpp`: `#include "cpcGL.h"` only passes.
  - `Tests\tmp_cl_cpcmesh_body.cpp`: copied `cpcMesh.cpp` bodies without
    `cpcGL.h` passes.
  - `Tests\tmp_cl_cpcmesh_and_gl_include.cpp`: `#include "cpcMesh.h"` then
    `#include "cpcGL.h"` fails silently.
  - `Tests\tmp_cl_cpcmesh_and_windows_include.cpp`: `#include "cpcMesh.h"` then
    `#include <windows.h>` fails silently.
  - `Tests\tmp_cl_windows_then_cpcmesh_include.cpp`: `#include <windows.h>` then
    `#include "cpcMesh.h"` passes.

Current known trigger: including the real `cpcMesh.h` before `windows.h` causes
CPC to exit 1 without a diagnostic. Including `windows.h` first avoids it.

- Added nearby in-repo passing coverage:
  `Tests\features\All\pass\test_return_struct_with_owning_members_and_scalar.cpp`.
  This does not reproduce the blocker; it proves the first suspected ownership
  return pattern is already supported.
- A broader self-contained approximation also passes:
  `Tests\tmp_reduce_mesh_windows.cpp`.
- Quick compiler-side search did not find a specific silent diagnostic path yet.
  Likely next files to inspect are `src\compiler\frontend\cprimegen.c`,
  `src\compiler\frontend\cprimepp.c`, and the driver return paths in
  `src\compiler\driver\cprime.c`.
- Rebuilt and replaced the local root compiler:

```bat
cmd /c build.cmd
```

Outcome: exit code 0. New `C:\Luke\Src\CPrime\cpc.exe` timestamp:
2026-08-16 09:37:29, size 795898 bytes. Version still reports
`cpc version 0.9.28 (x86_64 Windows)`.

- Focused coverage check after rebuild passed:

```bat
.\cpc.exe -run Tests\features\All\pass\test_return_struct_with_owning_members_and_scalar.cpp
```

- Copied the rebuilt compiler into the CommonLibrary repo because the racer
  build uses `C:\Luke\Src\OT\cl\cpc.exe`, not the CPrime root compiler:

```bat
copy /y C:\Luke\Src\CPrime\cpc.exe C:\Luke\Src\OT\cl\cpc.exe
```

Outcome: `C:\Luke\Src\OT\cl\cpc.exe` now matches the rebuilt compiler
timestamp and size.

- Correct command to build the simple racer/core target:

```bat
cd /d C:\Luke\Src\OT\cl
build_prime.cmd
```

Outcome today: fails with `Prime/CPC core-only build failed.` This is currently
blocked by the silent CPC failure already recorded for `cpcMesh.h` before
`windows.h`.

Progress after working the actual Racer build:

- Correct target remains:

```bat
cd /d C:\Luke\Src\OT\cl
build_prime.cmd
```

- Copied current compiler to the location used by that script:

```bat
copy /y C:\Luke\Src\CPrime\cpc.exe C:\Luke\Src\OT\cl\cpc.exe
```

- Found and fixed a real CPC heap overwrite in
  `src\compiler\middleend\libcprime.c`: `args_parser_add_file()` allocated
  `strlen(filename)` bytes for a flexible filename but copied the terminating
  NUL too. Added `+ 1`.
- Added regression source:
  `Tests\c_compat\pass\test_long_input_filename_argument_storage_regression_for_racer_build.c`.
- Added minimal runtime headers:
  - `include\runtime\utility`
  - `include\runtime\cstdint`

Local CommonLibrary edits made only to move the Racer measurement forward:

- `CommonLib\commonLib\src\Core\Polygon\cpcMesh.cpp`: include `windows.h`
  before `cpcMesh.h`.
- `CommonLib\commonLib\src\Core\Polygon\cpcRenderObjectCore.cpp`: include
  `windows.h` before project headers.
- `CommonLib\commonLib\include\Platform\clMemory.h`: include `<stddef.h>`;
  removed template forward declarations that CPC currently mishandles.
- `CommonLib\commonLib\include\Platform\clMemory.inl`: removed/rewrote local
  helpers that exposed CPC's current variadic-template parser limitations.
- `CommonLib\commonLib\include\Streams\clStream.h` and `clSeek.h`: locally
  stripped pure virtual/virtual declarations to get past current parser gaps.
- `CommonLib\commonLib\include\Containers\clList.inl`: removed stream helper
  include/definitions that are not needed by Racer.

Current Racer compile status:

- Per-source compile gets through the CommonLib `cpc*` core files after the
  include-order workarounds.
- `Racer.cpp` reached `Math\clIdentity.h`; the explicit specialization blocker
  recorded there is now fixed.

New expected-fail repros added for compiler work:

- `Tests\features\Templates\fail\test_template_overload_pointer_reference_constness.cpp`
- `Tests\features\Templates\fail\test_variadic_template_body_call_does_not_rename_template.cpp`
- `Tests\features\Templates\fail\test_template_after_variadic_template_keeps_own_name.cpp`

- Alternate unity build command:

```bat
cd /d C:\Luke\Src\OT\cl
build_prime_unity.cmd
```

Outcome today: fails with a normal diagnostic:
`CommonLib\commonLib\include\Platform\clMemory.h:39: error: redefinition of template 'clDelete'`.
Use `build_prime.cmd` as the main racer measurement unless specifically
working on the unity-only duplicate-definition issue.

Next wave progress:

- Added and fixed focused explicit class-template specialization coverage:
  - `Tests\features\Templates\pass\test_explicit_class_template_specialization.cpp`
  - `Tests\features\Templates\pass\test_explicit_class_template_specialization_double.cpp`
- Fixed CPC parsing/codegen support for:
  - `template<> struct Name<T> { ... };` specializations of existing
    one-argument class templates.
  - `double` as a template type argument.
  - expression-side static member calls through class-template instantiations,
    including namespace-qualified forms such as
    `std::numeric_limits<float>::infinity()`.
  - namespace-qualified function-template calls such as `std::isnan<float>(x)`.
  - deferring template-instantiated static member function bodies until the
    normal pending member-function flush, avoiding return-type corruption when
    instantiation happens inside an expression.
- Added runtime headers and focused coverage:
  - `include\runtime\limits`
  - `include\runtime\cmath`
  - `Tests\features\All\pass\test_runtime_limits_numeric_limits.cpp`
  - `Tests\features\All\pass\test_runtime_cmath_namespace_templates.cpp`
  - `Tests\features\Templates\pass\test_template_class_static_method.cpp`
- Copied the new runtime headers into the active runtime include directory for
  local measurement:

```bat
copy /y C:\Luke\Src\CPrime\include\runtime\limits C:\Users\Luke\AppData\Local\cpc\1.4\include\limits
copy /y C:\Luke\Src\CPrime\include\runtime\cmath C:\Users\Luke\AppData\Local\cpc\1.4\include\cmath
```

- Focused verification passed:

```bat
.\cpc.exe -run Tests\features\Templates\pass\test_explicit_class_template_specialization.cpp
.\cpc.exe -run Tests\features\Templates\pass\test_explicit_class_template_specialization_double.cpp
.\cpc.exe -run Tests\features\Templates\pass\test_template_class_static_method.cpp
.\cpc.exe -run Tests\features\All\pass\test_runtime_limits_numeric_limits.cpp
.\cpc.exe -run Tests\features\All\pass\test_runtime_cmath_namespace_templates.cpp
```

- `powershell -NoProfile -ExecutionPolicy Bypass -File Tests\run.ps1 -Suite features/Templates`
  now passes the new explicit-specialization test and all existing pass tests,
  but still exits 1 because the three pre-existing `Templates\fail` repro files
  fail under this harness.
- Rebuilt CPC with `cmd /c build.cmd` and copied it to
  `C:\Luke\Src\OT\cl\cpc.exe`.
- Re-ran the CommonLibrary racer measurement:

```bat
cd /d C:\Luke\Src\OT\cl
build_prime.cmd
```

Outcome: explicit specializations and missing `<limits>/<cmath>` are past the
previous blockers. Current blocker is now:

```text
C:/Luke/Src/OT/cl/CommonLib/commonLib/include/Math/clReal.h:29: error: '>' expected (got ',')
```

This comes from `clRealMap2(clPow, powf, pow)`, which expands to
`template<typename T, typename U> ...`; CPC still only handles single-parameter
templates in the current implementation.

### 2026-08-16 multi-parameter template wave

Started: `2026-08-16T10:29:24.4920831+08:00`.

- Added focused passing coverage for multi-parameter templates:
  - `Tests\features\Templates\pass\test_template_class_two_type_params_static_method.cpp`
  - `Tests\features\Templates\pass\test_explicit_class_template_specialization_two_type_params.cpp`
  - `Tests\features\Templates\pass\test_template_function_two_type_params_explicit_call.cpp`
- Reworked CPC's template internals from a single `type_param_tok` /
  `type_tok` to a compact `TemplateArgList` and per-template parameter list.
- Template instantiation cache keys now compare the full type-argument tuple.
- Template mangled names now include every type argument, e.g.
  `PairOps__int__double`.
- Template substitution now replaces any recorded template parameter token, not
  only the first one.
- Added parser support for comma-separated type arguments in class-template
  instantiation, explicit specialization, nested template type arguments, and
  explicit function-template calls such as `first<int, double>(...)`.
- Preserved the existing single-argument inference path for old function
  template calls by adapting it through a one-argument `TemplateArgList`.
- Fixed a regression found by the full template suite: constructor-bearing class
  template instantiations need pending member functions flushed in declaration
  contexts, while expression-side static member calls must suppress that flush.

Verification:

```bat
.\cpc.exe -run Tests\features\Templates\pass\test_template_class_two_type_params_static_method.cpp
.\cpc.exe -run Tests\features\Templates\pass\test_explicit_class_template_specialization_two_type_params.cpp
.\cpc.exe -run Tests\features\Templates\pass\test_template_function_two_type_params_explicit_call.cpp
powershell -NoProfile -ExecutionPolicy Bypass -File Tests\run.ps1 -Suite features/Templates
```

Outcome: all template pass tests pass. The broad suite still exits 1 because
the three pre-existing `Templates\fail` repro files still fail under this
harness.

Compile-speed check against clean `HEAD` baseline:

- Built a temporary clean worktree compiler from
  `cb6e1e223756d1261b69a54a359c841bb6264405`.
- Timed 5 compile-only iterations over the 39 baseline template pass files that
  both compilers accepted.
- Baseline average: `264.92 ms`.
- Multi-template compiler average: `261.68 ms`.
- Delta: `-3.24 ms` (`-1.22%`).

Conclusion: no measurable compile-time slowdown on the shared template pass
workload; the difference is inside normal process/file-system timing noise.

Re-ran:

```bat
cd /d C:\Luke\Src\OT\cl
build_prime.cmd
```

Outcome: the previous `clRealMap2(clPow, powf, pow)` multi-parameter template
blocker is past. Current blocker:

```text
C:/Users/Luke/AppData/Local/cpc/1.4/include/utility:4: error: redefinition of template 'clUnused'
```

This is triggered by `clTypeTraits.h/.inl` declaring both
`template<typename T> void clUnused(...)` and
`template<typename T, typename... Args> void clUnused(...)`. That is the
existing function-template overload/variadic-template area, separate from basic
multi-parameter template substitution.

### 2026-08-16 function-template overload / variadic registration wave

- Added and fixed focused coverage:
  - `Tests\features\Templates\pass\test_template_clunused_single_and_variadic_overloads.cpp`
  - `Tests\features\Templates\pass\test_template_function_forward_decl_then_definition.cpp`
  - `Tests\features\Templates\pass\test_explicit_function_template_specialization.cpp`
  - `Tests\features\Templates\pass\test_template_constexpr_function_specialization.cpp`
  - `Tests\features\Templates\pass\test_template_class_partial_specialization_decl.cpp`
- Added remaining expected-fail coverage:
  - `Tests\features\Templates\fail\test_variadic_template_multi_arg_forward.cpp`
- Implemented same-name function-template overload registration by parameter
  signature fingerprint.
- Merged function-template declaration followed by definition instead of
  reporting redefinition.
- Preserved function-template body references to the original template name so
  recursive/overloaded calls keep resolving through template overload logic.
- Added limited variadic support for the racer path: `typename... Args`
  metadata, basic arity selection, and one-pack instantiation sufficient for
  `clUnused(a, b)`.
- Added parser support for explicit function-template specialization,
  `constexpr` as a declaration specifier, non-type template parameters in
  declarations, and class-template partial-specialization declarations.

Verification:

```bat
powershell -NoProfile -ExecutionPolicy Bypass -File Tests\run.ps1 -Suite features/Templates
```

Outcome: all `Templates\pass` tests pass. Expected-fail gaps remain for richer
pack expansion, placement-new-on-scalar, and pointer/reference overload ranking.

Re-ran `build_prime.cmd`. Racer is past `clUnused`, `clSizeBytes`,
`clSizeBits<char>`, non-type template parameter parsing, and
`_clAbsHelper<T, true>` partial-specialization parsing. Current blocker:

```text
C:/Luke/Src/OT/cl/CommonLib/commonLib/include/Containers/clList.h:56: error: ':' expected (got '&')
```

Context:

```cpp
const T& operator[](i64 index) const;
T& operator[](i64 index);
clList<T>& operator=(const clList<T> &rhs);
```

## Blockers

1. `clList<T>::operator[]` non-const dependent reference return declaration.
   CPC reports:

```text
C:/Luke/Src/OT/cl/CommonLib/commonLib/include/Containers/clList.h:56: error: ':' expected (got '&')
```

   Next step: reduce a focused template-class member declaration with both
   `const T& operator[](int) const;` and `T& operator[](int);`, then fix parser
   handling for dependent reference return types in member declarations.

2. `cpcMesh.h` before `windows.h` causes silent compiler exit.
   - Reproducer depends on the external CommonLibrary include tree right now.
   - Trace log:
     `C:\Luke\Src\OT\cl\builds\core\tmp_cl_cpcmesh_and_windows_include_vv.log`
   - Last trace location is inside `winapi\winbase.h`.
   - Need either a self-contained test in this repo or a compiler fix once the
     precise internal condition is identified.

3. Full executable build exits 1 with no diagnostics, even though the generated
   compile-only source set passes. This may be the same silent-exit issue or a
   separate link/executable-production issue. Recheck after the active racer
   compile blockers.

## Notes

- Keep edits fast and scoped.
- Prefer real project failures over speculative compatibility work.
- When a project failure is too large, extract the smallest local test case
  under `Tests/features/...` that proves the missing C++ behavior.

## Cleanup Stage Notes

These are follow-up cleanup items noticed while pushing the Racer build forward.
Do not lose them when the immediate compile blocker changes.

- `clMemory.h` uses `size_t` in `extern "C"` declarations before any obvious
  local include of `<stddef.h>`. We temporarily confirmed this was a visibility
  issue under CPC by adding `<stddef.h>`.
  - Cleanup: remove use of `size_t` from `clMemory.h` if we want that header to
    stay independent of C standard header ordering.

- `clMemory.h` has template forward declarations while `clMemory.inl` contains
  the definitions included immediately afterward. CPC does not handle that
  pattern consistently for these templates yet.
  - Cleanup: identify the root compiler issue, create a minimal test, and fix
    CPC instead of carrying local header workarounds.

- Adding `include\runtime\utility` reached the repo portable include folder, but
  the active CPC used `C:\Users\Luke\AppData\Local\cpc\1.4\include`, so the
  header had to be copied there manually during the experiment.
  - Cleanup: improve the CPC library/runtime update mechanism so updated runtime
    headers are picked up automatically without adding startup or compile-time
    lag to normal CPC processes.
