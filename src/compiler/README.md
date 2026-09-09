# Compiler modules and performance requirements

CPC compiles serially. Measure full rebuilds with root `cpc.exe`, fixed inputs,
flags, runtime, and output paths. Report compiler process time separately from
project-driver and packaging time. Use local standalone cases for correctness
and scaling coverage; external applications are measurement workloads only.

## Current structure

`middleend/libcprime.c` includes the frontend, backend, and linker when
`ONE_SOURCE` is enabled. The frontend already uses several implementation
includes, but most state and declarations still belong to `cprimegen.c` and
`include/cprime/cprime.h`. Splitting files alone does not reduce compilation
work. A useful boundary must also establish data ownership and efficient access.

## Proposed boundaries

| Module | Owns | Performance requirement |
| --- | --- | --- |
| Source and tokens | Source buffers, include search, interned identifiers, saved tokens | Resolve each lookup once within its valid lifetime; avoid repeated path construction and token copying. |
| Declaration registry | Names, template owners, member and overload indexes | Look up an owner directly, then visit only relevant candidates; preserve declaration order where semantic lookup requires it. |
| Templates and replay | Definition recipes, arguments, instantiation state, pending bodies | Publish interfaces once and materialize selected bodies on demand; own and restore replay state explicitly. |
| Function compilation | Expression state, lifetime state, emitted instructions, local optimization | Keep frequently accessed data together; operate on one function at a time with bounded passes. |
| Object and link | Sections, symbols, relocations, imports, output buffers | Index symbol resolution and batch output writes; avoid repeated scans or transformations of the same data. |

Use statically linked C interfaces. Keep hot lookup primitives visible to the
compiler where inlining is beneficial. Cross module boundaries for meaningful
operations such as registering a declaration or emitting a function. A token,
expression, or instruction should not require a chain of dispatch calls.

Each allocation and cache needs an owner and a lifetime: compiler invocation,
translation unit, template replay, or function. Batch jobs must reset all state
that depends on tokens, macros, scopes, options, or input files. Reuse across jobs
requires explicit validation of everything that affects the cached result.

## Remaining work

1. Add opt-in phase measurements covering setup, input/preprocessing, semantic
   work, function emission/optimization, object writing, and cleanup. Nested
   template replay must not double-count time. Keep disabled overhead negligible.
2. Audit remaining global scans in template and member lookup. Extend existing
   indexes where justified by profiles, retaining candidate order, declaration
   updates, deferred bodies, and failure cleanup.
3. Extract declaration registration and ownership lookup behind a small internal
   interface, with batch-reset tests. Keep type definitions in `include/` and
   implementation in `src/`. Start with source organization under the current
   build, then measure separately compiled modules before changing the build.
4. Make replay and function scratch storage explicit. Measure allocation count,
   copied bytes, and peak memory before choosing arenas or reusable buffers.
5. Isolate object/link and platform setup once their shared-state dependencies
   are explicit. Measure both CPC self-build time and the speed of the resulting
   compiler; reducing self-build time must preserve application compilation speed.

Accept each step only after focused correctness tests, relevant batch/dependency
gates, and repeated serial timing on identical inputs. Keep raw commands,
compiler identities, logs, and results under `build/`. Retain meaningful changes
as separate reviewable units and revert experiments that fail these requirements.
