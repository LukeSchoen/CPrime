# Canonical CL migration

The canonical migration is integrated and application-tested on 2026-09-06.
The packaged compiler passes 990 feature tests and 48 canonical CL tests.
Both the 65-source Racer graph and 89-source FreeLancer graph compile and link
with zero warnings using the shared manifest driver. Racer renders and passes
keyboard/wheel input and clean shutdown; FreeLancer renders its textured scene
and exits cleanly after 35 seconds. Executables are preserved at
`C:/Luke/Src/OT/cl/builds/Racer.exe` and
`C:/Luke/Src/OT/cl/builds/FreeLancer_prime.exe`. Racer is the current ordinary
function selection. See task.md for current logs, timings and compatibility
limits. FreeLancer retains the native reference's shader binding warnings.

Racer, FreeLancer, and the test entry point are ordinary application functions.
CodeClip selects the source graph, exports the VS project-derived manifest, and
the shared build driver consumes that manifest for Prime and Clang. Compiler
selection does not select a different library or application graph.
CoreCodeClip.exe, generate_core.cmd, and the separate Premake core action have
been removed.
The obsolete build_racer.ps1 driver, its old-core compile probe, and the four
Racer-specific ABI/KNN/platform/printing shim sources are also removed. Their
source substitutions and the CPRIME_RACER_BUILD drawing branch are unnecessary
with the shared manifest and normal CL implementation.

## Shared string, filesystem, and assets

The duplicate compiler-era string, list, path, file, folder, and asset-path
implementations were removed after migrating their consumers. These names in
the table describe the old APIs, not compatibility aliases retained in CL.
Paths below are relative to `C:/Luke/Src/OT/cl`.

| Old API | Canonical API |
| --- | --- |
| `cpcString` | `clString` |
| `AssignLen`, pointer-based `Assign` | Counted construction and ordinary copy/move assignment |
| `Append`, `Concat`, `Equals`, `CharAt` | `+=`, `+`, `==`, `operator[]`/`at` |
| `SubstringFrom`, `TrimLeft`, `TrimRight` | `Substring`, `TrimStart`, `TrimEnd` |
| `PadLeft` | `Pad`, with explicit padding where the old space default is required |
| `cpcList`, pointer accessors, lowercase adapters | `clList`, reference accessors, canonical operation names |
| `cpcPath`, `cpcFile`, `cpcFileHelper`, `cpcFolder` | Corresponding `clPath`, `clFile`, `clFileHelper`, `clFolder` APIs |
| `cpcAssetPaths` | `clAssets` |

Mutable string indexing is limited to actual characters, preserving the final
NUL terminator. Counted construction, empty strings after move/ownership
transfer, overlapping appends, reverse-search boundaries, empty delimiters,
padding, and extreme substring counts now have passing reference coverage.
Canonical conventions remain explicit: file size for a missing file is zero,
and deleting an already absent file or folder succeeds.

Wide-pointer construction now selects the wide-string conversion overload for
mutable input too. UTF-8 conversion measures the required output bytes before
allocation. Unicode file moves use MoveFileExW with cross-volume copy support
and explicit replacement flags; failed moves do not pre-delete the target.
Recursive folder deletion reports failures promptly, including locked files,
and removes directory reparse points without following their targets.

Asset discovery honors explicit overrides, then searches executable-relative
`../CommonLib/Assets` and `Assets`, followed by the existing working-directory
candidates. A standalone process fixture verifies the precedence using actual
files under a private temporary directory. Existing encoded fallback paths and
the encoding APIs remain intact.

## Test migration and validation

All 27 original string/list/file/folder cases now live in canonical test headers
under `CommonLib/commonLib/test/{Strings,Containers,Platform}` and register with
`src/Platform/clTests.cpp`. The duplicate cpcTests runner and its four test
headers were removed. The Model Viewer test function calls `clTests::RunAll`.
New random/timing, renderer, and controller tests also register with the
canonical runner.

Verified during this wave:

- Packaged CPC and Clang canonical runners: 47 passed, 0 failed each, including
  the 27 migrated cases,
  new ownership/boundary/Unicode/filesystem cases, shared utilities, and renderer
  CPU-resource copy tests and controller event/virtual-device tests. The CPC
  run also links using clControls' own SDL dependency declaration.
- The added transform regression checks a quarter-turn, nonuniform scale,
  translation, copied matrices, and a transformed point against scalar expected
  values. It now passes with both compilers after correcting preservation of
  deduced member return types during later template specialization discovery.
- The renamed LanguageModel example passes CPC and Clang tests with an isolated
  in-memory corpus, including prefix filtering, word boundaries, reset behavior,
  and 100 generated words. Its actual missing-file path also exits cleanly with
  both compilers. The default external corpus path is unchanged and no files
  were created in the archive directory.
- Asset process fixture: 6 passed with CPC and 6 with Clang, using freshly
  compiled string, filesystem, cipher, and encoding sources.
- Both CPrime compiler probes previously including the real cpcList header now
  compile against canonical clList with CPC and Clang.
- Native/CPC DLL import interoperability: 24 COFF fixture cases pass, including
  both input orders and relocatable linking for direct and IAT references to
  the same import. The native controller fixture links and runs successfully.

These tests exposed general compiler limitations in conditional-expression
copy/move behavior, constructor delegation, character literals, cv-qualified
partial-specialization ordering, and fundamental UTF character types. The packed
compiler now passes the focused gates. Source runtime math tests also
pass in C and C++ with CPC and Clang, covering negative zero, infinities,
subnormals, NaN payloads, numeric overloads, and integral/floating type traits.

Normal CodeClip regenerated the project files and manifest with 65 canonical
sources and no removed core source/include paths. The same shared driver built
Clang Racer with zero warnings in 54.199 seconds of compilation using four
workers. Its executable is
`C:/Luke/Src/CPrime/build/migration-racer-clang/Racer.exe`. The smoke capture
shows the rendered red car; it runs for eight seconds and exits zero on
WM_CLOSE. Reference capture:
`C:/Luke/Src/CPrime/build/migration-racer-clang-transform-window.png`.

Only C: is available for physical-volume testing in this environment. Unicode
same-volume moves, explicit overwrite policy, and failure preservation are
tested; an actual move between two physical volumes has not been exercised.

The prescribed OT root `buildServer.cmd` was also exercised. Its migrated
common library compiled with MSVC, but the server build stopped on existing
`clDelete(this)` calls in `clOTItem.h` and `clOTContainer.h`: `clDelete(T *&)`
cannot accept the temporary pointer value. The calls and signature also exist
in the repository baseline and were left unchanged. The live server was not
running, and this check did not start one. The log is
`C:/Luke/Src/CPrime/build/migration-otserver-build.log`.

FreeLancer's full build and scene startup/shutdown checks now pass with the
packaged compiler. The retained `Tests/test_cl_assimp.py` integration fixture
checks the canonical Assimp C API wrapper's geometry/material import-export
roundtrip, texture-path resolution and missing-input/output failure handling.
Native large-stack probing and constructor-owned vtables have focused mixed
Clang/CPC regression coverage. Racer selection and its CodeClip manifest have
been restored after the FreeLancer check.

## Additional compiler compatibility checks

The reviewed compiler also passes the unmodified TinyXML-2 11.0.0 official
suite: 517 passed, 0 failed. Its three source files were checked byte-for-byte
against the pristine upstream archive. This check exposed and now covers
pointer-to-bool overload conversion, const-qualified C++ string literals, and
local-class virtual parameter metadata surviving deferred template emission.
TinyXML-2 still reports four mutable-member qualifier warnings, one overload
pointer warning, and repeated derived/base pointer comparison warnings; the
successful runtime suite does not imply those diagnostics have been resolved.

A further retained regression checks nested auto-return member calls on
different receiver classes. Completing the nested member body now preserves
the explicitly selected outer function. The real canonical clMesh.cpp unit
compiles without diagnostics using that reviewed compiler. These changes have
focused CPC/Clang runtime coverage and surrounding suite checks; the final
packaged application verification is recorded separately in task.md.
