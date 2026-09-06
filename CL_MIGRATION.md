# Canonical CL migration

The CodeClip-selected application currently compiles the canonical CL sources.
Racer and FreeLancer are functions in the application, selected in its source;
compiler selection must not select another source graph. This inventory records
the remaining legacy components before removing them. Paths below are relative
to `C:/Luke/Src/OT/cl` and were inspected on 2026-09-06.

## String comparison

The canonical implementation is
`CommonLib/commonLib/include/Strings/clString.h` with
`CommonLib/commonLib/src/Strings/clString.cpp`. The legacy implementation is
`CommonLib/commonLib/include/core/String/cpcString.h` with
`CommonLib/commonLib/src/Core/String/cpcString.cpp`.

| Legacy operation | Canonical replacement or migration requirement |
| --- | --- |
| `AssignLen(text, n)` | Assign `clString(text, n, false)` for an exact non-null-terminated buffer. Check callers' embedded-null assumptions. |
| `Assign(text)` / `Assign(&other)` | Ordinary string construction / copy assignment; remove the pointer-based copy API at callers. |
| `Append(text)` / `Concat(text)` | `+=` / `+`; retain owned-storage and overlapping-input behavior in regression coverage. |
| `CharAt(i)` | `operator[]` or `at`; canonical access is const, so mutable indexing requires a deliberate replacement. |
| `SubstringFrom(i)` | `Substring(i)`. |
| `PadLeft(n, c)` | `Pad(n, c)`; pass the padding explicitly because the defaults differ (`' '` versus `'0'`). |
| `TrimLeft` / `TrimRight` | `TrimStart` / `TrimEnd`. |
| `Equals(text)` | `operator==`. |
| Search, splitting, replace, case conversion, left/right, deletion around a delimiter | Already present in `clString`; preserve caller expectations at boundaries rather than assuming identical semantics. |

Canonical `clString` additionally has move construction/assignment, wide-string
input, case-sensitive/insensitive matching, split options, joining, formatting,
ownership transfer and generic `clToString` integration. These are supported
through ordinary compiler behavior; retaining a smaller compiler-specific
string would discard useful functionality.

The useful legacy behavior to preserve is explicit handling of assignment and
append from its own buffer before allocation invalidates that buffer. Check
self-append, interior-pointer append/assignment, deep copies, moves and empty
strings against canonical CL. The legacy allocator requests exact capacity and
has no move operations; neither is a reason to preserve the fork. Its allocation
failure behavior is inconsistent (assignment may empty the string while append
keeps it), so it should not silently become the canonical allocation policy.

Boundary semantics require explicit tests during migration: empty search
patterns, null pointer input, negative/out-of-range substring inputs, embedded
nulls, empty split tokens and padding defaults. For example, the legacy reverse
search returns `-1` for an empty pattern while canonical `_FindStringReverse`
returns the source length. Existing legacy tests live in
`CommonLib/commonLib/test/core/String/cpcTests_String.h`.

## Remaining consumers and order

The legacy string is still referenced by `cpcPath`, `cpcFileHelper`, `cpcFolder`,
`cpcAssetPaths`, the `cpcTexture`/mesh/material/OBJ model family,
`cpcRenderObjectCore`, `Projects/Model Viewer/src/Private/FreeLancer.cpp`,
`cpcLanguageModel.cpp` and legacy container/string tests. Removing the type
before migrating these consumers would break application functions outside the
currently selected Racer call.

The other legacy families include `cpcList`, vector/matrix/types/random,
camera, timers, input, audio and the Win32/GL window layer. Their counterparts
and behavioral differences still need component-level review; string API
similarity does not establish that all these forks can be deleted mechanically.

Migrate complete dependency groups to canonical CL, transfer useful behavioral
tests, and fix compiler failures with neutral language regressions. When source
or include membership changes, regenerate the graph with CodeClip.exe and
compile that same graph with Prime and the reference compiler. Preserve timing
measurements and run the selected application through startup and shutdown.
Remove each legacy component only once its remaining consumers and tests have
been migrated.
