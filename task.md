# CPrime CL Compatibility and Unification Task

Last updated: 2026-08-26

## Achieved Milestone

We successfully built Racer with CPC. The full compile and link completed, and
the Racer executable was produced.

This establishes a working baseline. Preserve it throughout the remaining
compatibility and cleanup work: every retained change should leave Racer
buildable with CPC.

## 2026-09-02 Verification Wave

- Regenerated the canonical Racer graph with the separate `CoreCodeClip.exe`.
- Removed the unconditional legacy `cpcFreeLancer.cpp` entry from the core
  manifest; the selected application path is now `ModelViewer.cpp -> Racer()`.
- Conventional CPC compile and link: 111.013 seconds.
- Bounded four-source CPC unity compile and link: 80.314 seconds.
- Both modes produced `C:\Luke\Src\OT\cl\builds\racer\Racer.exe` with no
  undefined symbols and a complete x64 runtime dependency closure.
- The unity artifact passed a 10-second startup smoke test.
- The broad batch suite remains blocked by the pre-existing
  `std::initializer_list<int>` zero-argument constructor link failure; the
  focused regressions added in this wave pass.

## 2026-09-02 Parallel Build Performance Wave

- Confirmed the slowdown is concentrated in template-heavy C++ translation
  units rather than CPC startup or linking: small C units complete in roughly
  0.05 seconds, while the former sequential build spent 15-21 seconds in each
  of several large Racer units.
- `build_racer.ps1` now schedules independent CPC compilations concurrently,
  defaults to the machine's logical processor count, retains per-unit timeout
  and diagnostic handling, and accepts `-Jobs` for constrained machines.
- Conventional compile and link fell from 111.013 seconds to 28.359 seconds on
  the 8-core/8-GB measurement machine (3.91x faster).
- Unity compile and link takes approximately 27.4 seconds, excluding the
  required 10-second startup smoke window, down from 80.314 seconds (2.93x
  faster). The complete measured invocation was 37.385 seconds and passed the
  startup smoke test.
- Both modes linked with zero undefined symbols and produced
  `C:\Luke\Src\OT\cl\builds\racer\Racer.exe` with a complete x64 runtime
  dependency closure.
- The remaining gap to Clang/MSVC is frontend throughput within individual
  template-heavy units; the longest parallel critical-path unit still takes
  about 21 seconds. This is the next optimization target rather than process
  startup or linker work.

## Ongoing Goal

Remove the tricks, hacks, workarounds, and Racer-specific specializations that
were introduced to reach the first successful build. Replace them with general,
correct compiler behavior and appropriate regression coverage.

The build should keep working throughout this process. Work incrementally from
the specialized `cpcRacer` build path toward building the unmodified/plain
`racer` target directly. Do not trade away the known-good build while removing
the compatibility scaffolding.

The target progression is:

```text
cpcRacer  ->  racer
```

## Architectural Direction: One Canonical Codebase

Compiler choice should not create a separate family of libraries, applications,
or project names. CPC is one compiler for the normal CL codebase, alongside the
compiler used by MSBuild and Clang. The long-term structure should not require a
parallel `cpc*` implementation of each component.

For example, CPC should compile the canonical `clString`; we should not need a
separate `cpcString` merely because CPC is the selected compiler:

```text
cpcString + clString  ->  clString built by CPC, MSBuild, or Clang
```

This is a merge, not a blind deletion. Before retiring a CPC-specific component,
compare it with the canonical component. If `cpcString` contains good design
ideas, useful functions, clearer names, cleaner interfaces, fixes, or other
improvements, preserve those ideas by incorporating them into `clString` where
they improve the shared API or implementation. The canonical version should
become the best version, not merely the oldest non-CPC version.

Apply the same rule to CPC-only applications and project names. If an application
exists as `cpcFreeLancer`, convert it into the canonical `FreeLancer` project and
make that project buildable with the supported toolchains:

```text
cpcFreeLancer  ->  FreeLancer
```

The `cpc` prefix should describe the compiler executable itself, not a permanent
fork of every program compiled with it.

## Toolchain Goal

The normal projects should be buildable using any of these paths without the
choice being a major source or project-layout event:

- MSBuild and its configured compiler toolchain.
- CPC.
- The bundled Clang executable:
  `C:\Luke\Src\OT\cl\CommonLib\Assets\Programs\Clang\clang.exe`.

Ideally, selecting a compiler is a build configuration or command-line choice.
It should not require renaming the target, selecting a compiler-specific source
fork, or maintaining a parallel application. Keep one canonical project name,
one primary source graph, and the smallest defensible amount of toolchain
configuration.

This does not mean restricting the codebase to whatever all three compilers can
already handle. When a valid shared source construct exposes a CPC limitation,
expand and improve CPC. When the problem is in project discovery, flags,
dependency ordering, generated files, object layout, or linking, improve the CL
build scripts. Use portable source changes when they genuinely improve the
shared code, but do not disguise compiler bugs with permanent CPC-only source
specializations.

## Working Rules

1. Keep a reproducible, known-good CPC Racer build as the baseline.
2. Identify one hack, trick, workaround, or specialization at a time.
3. Add or retain a focused compiler regression that captures the underlying
   language behavior.
4. Replace the special case with a general compiler fix.
5. Rebuild CPC and run focused and surrounding compiler regressions.
6. Copy the verified compiler into the Racer build environment and rebuild the
   real project through compile and link.
7. Confirm that the Racer executable still exists before retaining the change.
8. Revert failed experiments rather than accumulating overlapping workarounds.
9. Continue until the plain `racer` target builds without the `cpcRacer`
   compatibility path.
10. For each `cpc*` library or application fork, compare it with its canonical
    counterpart and inventory any ideas worth preserving before removing or
    merging it.
11. Move worthwhile CPC-fork improvements into the canonical component with
    appropriate coverage, then update all consumers to use that component.
12. Prefer build-script/toolchain selection over compiler-specific project names
    and duplicated source trees.
13. Recheck the affected canonical target with CPC, MSBuild, and bundled Clang
    whenever that toolchain is usable for the target.

## Verification

For each coherent cleanup wave:

```cmd
cmd /c build.cmd
```

Run the relevant focused compiler tests and nearby suites, then run the real
Racer compile and link. Verify the expected executable explicitly:

```powershell
Test-Path 'C:\Luke\Src\OT\cl\builds\racer\Racer.exe'
```

Expected path:

```text
C:\Luke\Src\OT\cl\builds\racer\Racer.exe
```

For component and project unification work, also verify the canonical target
through the applicable build paths. Record the exact commands and distinguish
between:

- a CPC language or code-generation limitation;
- a shared-source portability defect;
- a CL build-script or project-configuration defect; and
- a toolchain-specific dependency or linker requirement.

Fix the issue in the appropriate layer and keep the already-working toolchains
green while adding the next one.

## Completion Criteria

The ongoing task is complete when:

1. The plain `racer` target builds with CPC through compile and link.
2. The `cpcRacer`-specific build path and its associated compatibility tricks,
   hacks, workarounds, and specializations are no longer required.
3. Their replacements are general compiler fixes with passing regression
   coverage.
4. Relevant compiler test suites pass.
5. The expected Racer executable exists at the path above.
6. Any remaining warnings or limitations that could affect runtime behavior are
   documented explicitly.
7. Canonical shared components such as `clString` compile directly with CPC; a
   parallel component such as `cpcString` is not required.
8. Useful APIs, naming, fixes, and design ideas from CPC-specific components have
   been evaluated and, where beneficial, preserved in the canonical components.
9. CPC-only project identities such as `cpcFreeLancer` have been converted into
   canonical projects such as `FreeLancer`.
10. The canonical projects can select MSBuild, CPC, or bundled Clang through the
    build system without compiler-specific source forks or renamed applications.

## Cleanup Expectations

- Remove temporary `CPC_*` diagnostics and debug-only `fprintf(stderr, ...)`
  traces from retained compiler changes.
- Remove scratch reducers and logs that are not promoted to passing tests.
- Remove name-specific and Racer-specific compiler behavior.
- Remove redundant `cpc*` source and project forks only after their worthwhile
  differences have been merged into the canonical implementation.
- Do not lose useful CPC-side functions, names, fixes, or design improvements in
  the cleanup; promote them into shared code when they are genuinely better.
- Keep compiler-selection differences in build configuration and narrowly
  scoped portability layers rather than duplicated applications or libraries.
- Keep each retained change coherent and reviewable, with its implementation,
  regression coverage, and task notes together.
- Keep Racer working at every step while moving from `cpcRacer` to `racer`.

## Frontend performance wave (2026-09-03)

- Profiled the heaviest conventional Racer unit (`clRenderObject.cpp`).  Its
  template overload path issued about 291,000 type-compatibility queries in a
  single compile.
- Added canonical `CType` identity fast paths so repeated pointer, function,
  structure, and scalar compatibility queries avoid recursively walking an
  already-identical type graph.
- Indexed template declarations by lookup token and instantiated class
  templates by their generated class token.  Both indexes preserve original
  declaration order; instantiated lookup retains the historical spelling
  fallback for replay-created equivalent tokens.
- Rejected and removed a free-function overload bucket experiment after it
  changed overload selection/object output.
- Real Racer results on this machine, including compile and link:
  conventional parallel build 26.176 s (previous 28.359 s); unity build
  24.116 s (previous approximately 27.4 s).  Unity's slowest individual unit
  was 19.624 s under concurrent load.
- Verified `Racer.exe` at `C:\Luke\Src\OT\cl\builds\racer\Racer.exe`, with
  zero undefined symbols, complete x64 runtime dependency closure, and a
  successful 10-second startup smoke test.
- The broad batch suite remains blocked by the pre-existing undefined
  `__cpc_ns_std_initializer_list__int_constructor_0args`.  The template suite
  completed 232 passing cases and reported 12 existing failures; the first
  constructor-suite failure was unchanged when the type fast path was removed.
- A CPU phase split of isolated `clRenderObject.cpp` compilation measured
  9.315 s in the initial declaration/frontend pass, 0.781 s in deferred
  template finalization, and 2.329 s in inline emission (12.425 s compiler CPU
  total).  The approximately 20 s per-unit number occurs under concurrent
  heavy-unit contention and is not 20 s of template-finalization replay.
- Positive auto-return caches and a last-specialization cache were tried and
  removed after leaving the real build unchanged (26.34 s versus 26.18 s).
  They did not justify their added state.
- The self-host build was also measured with CPC `-O1` and `-O2`.  Both made
  the branch-heavy compiler slower (`clRenderObject` CPU totals of 12.854 s
  and 14.488 s respectively), so the build configuration was restored.  This
  identifies a separate CPC optimizer/code-quality opportunity but is not a
  frontend-speed fix by itself.
- Preprocessing the heavy `clRenderObject.cpp` unit with its real Racer flags
  takes only 0.433 s, confirming that nearly all of the 9.315 s initial pass
  is semantic declaration processing rather than include/tokenization cost.
- Cached the frontend's diagnostic environment switches.  The 134 accumulated
  `getenv()` trace guards previously searched the Windows CRT environment from
  hot expression, overload, lifecycle, and template paths even when tracing
  was disabled.  Each distinct switch is now queried once with a fixed,
  allocation-free 64-entry cache.
- Disabled scan profiling no longer performs unconditional counter updates in
  overload/template loops.  The counters remain available when
  `CPC_PROFILE_SCANS` is requested.
- With these declaration-path fixes, the real parallel unity compile/link fell
  from 24.116 s to 22.757 s.  Warning-heavy unity unit 20 fell from 13.584 s to
  9.678 s; the slowest contended unit fell from 19.624 s to 18.613 s.
- Converting additional member/template scans to the existing candidate
  buckets was measured and removed: unity regressed to 23.896 s and the
  slowest unit rose to 19.833 s.  This indicates that candidate traversal is
  not the remaining dominant declaration cost; work performed while checking
  each candidate and processing expressions is the next target.
- High-resolution profiling then isolated class-specialization token
  substitution as the dominant semantic declaration cost: 7.215 s of an
  isolated `clRenderObject.cpp` compile, versus 0.086 s parsing the generated
  class declarations and 0.698 s publishing member interfaces.  `clList`
  accounted for nearly all of the repeated work.
- Class replay used to perform class-template and typedef-stack lookup for
  every identifier, including ordinary method, field, and parameter names.
  Those semantic lookups are now gated by syntax: class-template lookup only
  runs when an angle argument list follows, namespace lookup only runs before
  `::`, and typedef lookup only runs where a type can occur.  The `template`
  keyword token is interned once instead of repeatedly converting tokens to
  strings and comparing them.
- The isolated production `clRenderObject.cpp` compile now completes
  consistently in 7.45-7.57 s (including finalization and code generation),
  down from 11.71 s in the directly comparable profiled run.  Substitution
  itself fell from 7.215 s to 2.947 s in the profiling build.
- Real Racer compile/link improved to 15.36 s in unity mode (previously
  22.757 s) and 17.97 s in conventional parallel mode (previously 25.759 s).
  Both modes linked with zero undefined symbols and complete x64 dependency
  closure.  The final conventional executable passed the 10-second startup
  smoke test at `C:\Luke\Src\OT\cl\builds\racer\Racer.exe`.
- Added template replay coverage for dependent class templates, namespace
  qualification, aliases, and ordinary identifiers sharing a template name.
  The template suite is now 233 passing cases with the same 12 pre-existing
  failures.
- A second native CPU trace of the now-faster heavy unity unit exposed three
  remaining accidental whole-program searches.  `find_typedef_sym_in_stack`
  walked the complete global symbol stack; demanded member-body queuing
  repeatedly searched every instantiation of every member definition; and
  member-definition method tokens were decoded from their captured token
  strings on every query.  Token-chain lookup, an allocation-free set of
  materialized bodies, and cached decoded method tokens removed those costs.
- A post-fix trace then found the shifted hotspots: linear recognition of
  mangled member functions, linear free/static overload-name lookup, repeated
  scans of all template members for a `(class, member)` pair, and repeated
  global typedef-by-type searches.  Exact token sets and the existing ordered
  template-member buckets now answer the overload/member predicates.  Each
  fixed set retains the original exhaustive scan as a full-table fallback.
- Template argument deduction now caches globally stable type spellings by
  exact `CType` identity after checking visible local aliases.  Cached pointer
  types retain and validate a stable type snapshot so recycled local symbol
  nodes cannot create false hits.  This made alias types canonical while
  avoiding repeated walks of the entire global declaration stack.
- The heavy isolated Racer unity unit fell from 7.45-7.57 s after the first
  replay wave to 1.74-1.78 s after the demanded-body fixes, 1.19-1.31 s after
  overload/member indexing, and finally 0.738-0.853 s with type-spelling
  caching.  Its generated object remained the same size across those measured
  frontend-indexing steps.
- Real Racer unity compilation now takes 3.535 s.  A clean compile plus link
  takes 3.796 s, versus the stated 11.16 s Clang unity target.  The real
  conventional compile plus link takes 5.784 s, down from 17.97 s in the
  preceding wave.  Unity jobs are submitted in measured heavy-unit-first
  order so short units cannot delay the parallel critical path.
- The final unity build linked with zero undefined symbols, passed complete
  x64 runtime dependency closure, and stayed running through the 10-second
  startup smoke test.  The executable is
  `C:\Luke\Src\OT\cl\builds\racer\Racer.exe`.
- Verification after the complete optimization wave: template suite 233 pass
  with the same 12 known failures; constructor suite 39 pass with its same
  known failure; destructor suite 14/14 pass; member-function suite 39/39
  pass.  CPC also rebuilt itself successfully with the retained changes.

## tinyxml2 real-project compatibility wave (2026-09-04)

- Vendored pristine tinyxml2 11.0.0 into
  `C:\Luke\Src\CPrimeCppTests\vendor\tinyxml2`; `tinyxml2.cpp`, `tinyxml2.h`,
  and `xmltest.cpp` remain byte-for-byte identical to the upstream archive.
- Added the external `CPrimeCppTests\run.ps1` harness, project/version/hash
  manifest, provenance notes, and ignored build-output layout.
- The harness compiles both upstream translation units, links, and runs the
  official test program.  Final result: `Pass 517, Fail 0`; executable:
  `C:\Luke\Src\CPrimeCppTests\build\tinyxml2\xmltest.exe`.
- Fixed sibling-scope aliasing between CPC's cached register-spill slots and
  later C++ objects by retiring the temporary cache whenever a normal lexical
  scope releases its stack allocation.
- Routed the preserved receiver for virtual calls through the same temporary
  allocator, avoiding independent raw stack slots that could collide with
  call-target spills.
- Corrected enum identity and conversion direction during overload ranking,
  and rejected conflicting repeated function-template deductions so an old
  template specialization cannot defeat the viable non-template overload.
- All 12 focused regressions for the complete tinyxml2 wave pass.  Against a
  source build of committed `b141f35`, the surrounding Classes suite improves
  from 123/131 to 124/131, Templates from 238/255 to 239/255, and All remains
  26/27; there are no newly failing surrounding tests.
- The pristine project still emits CPC's existing incompatible-pointer
  warnings for valid C++ base/derived conversions.  They are diagnostic noise
  in this target (all 517 runtime checks pass), but warning cleanup remains a
  separate compatibility improvement.
