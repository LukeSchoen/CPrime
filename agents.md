# Agent Instructions

- Never run git, and never fetch from, pull from or push to GitHub. Publishing
  the base copy is a banned action reserved for the user, done by hand outside
  the agent loop; do not trigger it, script it or ask for it.
- Use the repository's `cpc.exe` for compilation, builds, and regression work.
  Do not invoke Clang, GCC, MSVC, or another compiler, including host rebuilds,
  reference comparisons, and benchmarks, unless the user explicitly requests it.
  A CPC failure is a bug to investigate, not permission to switch compilers.
  `src/scripts/build.exe` self-hosts with root `cpc.exe` and publishes back to that path.
  Keep one active compiler: root `cpc.exe`. Do not introduce host-selection modes
  or alternate working compiler copies. Staging belongs inside the build only;
  a failed build preserves root `cpc.exe` and stops without changing hosts.
  Clang builds must have Clang explicitly in their entry-point name and still
  require authorization. For external projects verify they use fresh validated CPC.
  The test workflow is CPC-only: `Compatibility/tests/test.exe` runs every
  internal case, including the Microsoft x64 ABI facts in
  `Compatibility/tests/features/Abi`.
- Run Cprime compilation and project builds serially: one thread and one compiler
  process at a time. Improve algorithms and data handling, not concurrency.
  Compiled programs may still use threads. MSVC benchmarks may run in parallel.
- Reproduce reported bugs, add regression coverage, implement a coherent fix,
  and run focused relevant tests only.
- Revert failed experiments. Do not retain name-specific hacks or turn active
  reproducers into expected failures to disguise incomplete work.
- Keep all tests in this CPrime repository. Reduce external bug reports to standalone local tests.
  Keep tests minimal, deterministic, and fast; reuse helpers and remove redundant
  fixtures without losing distinct behavior coverage.
- The top level is only `readme.md`, `agents.md`, `LICENSE`, `SECURITY.md`,
  `.gitignore`, `.gitattributes`, `cpc.exe`, `src/`, `Cost/`, `Capability/` and
  `Compatibility/`. Everything else belongs under `src/` or inside the area it
  serves.
- `src/` is the toolchain: `src/compiler/`, `src/runtime/`, `src/tools/`
  (native tool sources and test helpers), `src/scripts/` (workflow sources and
  tracked workflow executables), `src/include/` (compiler and runtime headers),
  `src/lib/` (bootstrap link definitions and the bootstrap runtime archive),
  `src/deploy/` (the shipped libcprime SDK) and `src/third-party/` (vendored
  sources). Generated toolchain output lives in `src/build/`.
- Each area owns its own directory: `Cost/` (compile speed), `Capability/`
  (generated-program speed and quality) and `Compatibility/` (C++17
  correctness). An area holds `worker.cmd`, `task.md`, `tests/`, optional
  `tools/`, its retained baselines and its own generated `build/`.
- Everything a cycle produces that is not toolchain work or retained area
  work belongs in the area's `build/`, which is never published: probes,
  one-off reproducers, scratch sources and every executable or object they
  leave behind. A file left loose in the tree is held back from the cycle
  commit and reported in `build\worker-state.txt` and the cycle row instead of
  being published, so keep scratch under `build/` and reduced cases under
  `tests/`.
- Workflow tools must not assume their own directory: locate the tree root by
  walking up to `cpc.exe` (`nt_tree_root` in `src/scripts/common/native_tool.h`)
  and pass `-B<tree>/src` when the compiler's private `include/` and `lib/`
  trees are needed.
- Use only C, C++, and assembly for first-party code. Native workflow sources
  live in `src/scripts/` and `src/tools/`; do not add batch, cmd, PowerShell, or
  Python.
  Leave third-party sources/tooling unchanged; documentation and data are exempt.
- `Cost\worker.cmd`, `Compatibility\worker.cmd` and `Capability\worker.cmd` are the
  user's agent-loop control surface, are tracked in the repository and are exempt
  from the rule above. They start their own area's TASK_PROMPT file (`task.md`) and
  commit each cycle, so an agent must never delete, move, rename or rewrite any
  `worker.cmd`, and must keep every `task.md` current with that area's remaining
  work. Edit only the `task.md` of the area being worked on.
- Prefer C sources compiled with root `cpc.exe` into tracked native workflow
  executables. Rebuild them serially with `src/scripts/tool-build.exe`; that
  tool rebuilds itself only by hand (`cpc.exe -O2 -o src/scripts/tool-build.exe
  src/scripts/tool_build.c`), because a running image cannot replace itself.
  Keep shared workflow implementation in `src/scripts/`, test-specific sources
  in `src/tools/`, and transient output in the owning `build/` directory.
- Do not run the pedantic tier. It is close to banned: run it only as the last
  and only step of an important confirmation, and avoid it if at all possible.
  Verify with the exact retained case, the affected suite, the fast tier and
  `-Regression` instead; none of those needs pedantic. Exact retained
  reproducers and focused runner unit tests are allowed during repairs. Keep
  the fast gate near a second.
- Keep Markdown limited to remaining work and decisions; completed work is code.
  Do not add reference tests or benchmarks without explicit user authorization.
  Clang builds are available only through explicitly Clang-named entry points.
  Investigate CPC failures using CPC; do not change the default host.

Builds create the compiler from the C/assembly bootstrap runtime first. The
candidate compiler builds the packaged C++ runtime only when runtime, SDK, or
package inputs invalidate the cache; use `src/scripts/build.exe -RebuildRuntime`
for an explicit ABI/code-generation refresh. Packaging and the regression gate
must pass before replacing root `cpc.exe`. Preserve that executable and
`src/lib/` for bootstrapping. Generated output belongs in `src/build/`.

The explicit seed-host proof is `src/scripts/seed-tcc.exe -RunExternal`. It
builds the compiler translation unit as C with TCC and proves the resulting CPC
can compile and run C. TCC execution still requires explicit authorization.

For solution builds, use an exported build manifest and
`src/scripts/build-project.exe -ProjectRoot <root>`. The serial project driver is
native C, and `src/scripts/build-project-clang.exe` is the same driver for the
external Clang toolchain (it requires `-RunExternal -Toolchain Clang`).
Rebuild workflow executables after tool-source changes with
`src/scripts/tool-build.exe`. Paths are configurable; CPC remains the compiler.
Build metrics and dependency caches live in the output directory.
Use `-Rebuild` to compile every unit again. `-Unity -UnityBatchSize 24` groups
up to 24 compatible C++ sources per unit; the default batch size is 32.
Keep file-local name conflicts in separate manifest `unityGroup` values.
Metrics include elapsed and CPU seconds for each tool process.

## Where this agent is running

Work is normally split across four copies of this repository on one machine:

- `C:\Luke\Src\PRIME\CPrime` is the base copy. Its own origin is GitHub, and it
  is the origin of the three work copies. Do not work in it.
- `C:\Luke\Src\PRIME\CPrime_Capability`,
  `C:\Luke\Src\PRIME\CPrime_Compatibility` and
  `C:\Luke\Src\PRIME\CPrime_Cost` are the work copies. Each runs exactly one
  area worker, and each work copy's origin is the local base copy, not GitHub.

The three workers are the three arms of one shared branch. The base copy holds
the branch they meet on, so nothing reaches GitHub until the user publishes the
base copy.

The user starts the workers with `PRIME\work.cmd` and stops them with
`PRIME\stop.cmd`, which writes the area's `done.x` stop marker. Both live outside
the repository. They are the user's controls: never create `done.x`, and never
start, stop or restart a worker yourself.

What follows from that layout:

- Work only inside the copy whose `worker.cmd` started you. A sibling copy is
  another agent's tree: reading it is fine, editing it is not.
- The copy is shared. Work you leave behind is published to the other two
  copies, and their work arrives in yours, so leave the tree in a state that
  survives a merge and never rewrite another agent's work to make a merge easy.
- Never run git. The worker commits, fetches, rebases and pushes at every cycle
  boundary against the local base copy only, never GitHub, and it hands you the
  rebase when a conflict cannot be settled mechanically.
- The three copies share one CPU. Timing evidence taken while the other workers
  are running is noisier than the same work alone on its own machine, so keep
  timing claims to repeated serial runs and medians, and record which workers
  were active.

## Test and develop

```
Compatibility/tests/test.exe -All -Tier fast
Compatibility/tests/test.exe -Suite features/Templates
Compatibility/tests/test.exe -Regression
Cost/tests/test.exe -All -Tier fast
Capability/tests/test.exe -All -Tier fast
```

`tiers.json` lists the `fast` subset, which is the open-work list; every other
retained internal case belongs to the pedantic tier, which combines short pass
cases into unity units and runs test programs `-Jobs` at a time while
compilation stays one serial batch per suite. Coverage there is real, but
running it is not part of the workflow: see the pedantic rule above.

See [test commands and layout](Compatibility/tests/README.md) and the
[development loop](Compatibility/tests/DEVELOPMENT.md). Bug reports should
include a standalone reproducer, the command, and expected versus actual
behavior.

`Compatibility/tests/test.exe -All -Tier fast` is the routine loop and
`Compatibility/tests/test.exe -Regression` is the publication gate, which
`src/scripts/build.exe` runs on its own. Suites are discovered under the tests
directory of the harness that runs them, so each area can keep its own cases;
measurement inputs stay in `Cost/tests/compile` (compile cost) and
`Capability/tests/runtime` with `Capability/tests/performance` (generated-code
runtime and correctness). Tests are CPC-only; a change that would need a second
compiler belongs in an explicitly named tool and needs separate authorization.

Implementation lives in `src/`, headers in `src/include/`, test and benchmark
inputs in the owning area's `tests/`, and native workflow sources and
executables in `src/scripts/`.
