# CPrime: Near Instant C, C++, and Assembler Compilation

[Download the Windows zip](https://github.com/LukeSchoen/CPrime/archive/refs/heads/main.zip)

CPrime is a fast assembler, C compiler, and C++ compiler for people who want
near instant builds, tiny native outputs, and the full expressiveness of C++.

The compiler is called **CPC**. It is general purpose and requires no
application-specific libraries. It is a full C++ compiler: you can throw a big
C++ project at it and it will compile. CPC takes assembler, C, and C++ code and
almost instantly produces lightweight native outputs, without bulky C++
toolchains.

## The Short Version

C is fast, small, and beautiful. C++ is powerful and expressive. CPC brings
both together with fast compilation, small binaries, and a lightweight toolchain.

## Why cPrime Exists

C++ offers a lot of developer comfort: classes, constructors, destructors,
templates, overloaded functions, and the language features needed to organize
large projects.

But the price is steep:

- slow builds
- huge toolchains
- large executables
- layers of accumulated complexity
- features that authors like but that cost every build forever

CPrime asks a simple question:

> What if using C++ could be done in a way that was just really cheap?

## What Do We Have Today?

- Full C compatibility: structs, enums, function calls, recursion, pointers,
  arrays, macros, stdio, and compile-fail cases
- Full C++ project support
- 100% C++ support
- Fast assembler support
- Lightweight native outputs
- Near instant compile times
- Small toolchain footprint

## Build

Run `Build.cmd` with the bootstrap `cpc.exe` at the repository root to self-build
the compiler and runtime serially with CPC. Run `BuildClang.cmd` to use Clang
for the optimized compiler host when available. Both commands
package and replace the root `cpc.exe`.
Build scripts live in `scripts/windows/`; generated compiler and runtime files
go into `build/compiler/`. A successful build packages and replaces the root
`cpc.exe`. Keep that executable and `lib/` for bootstrapping.

Compiler/runtime sources live in `src/`, headers in `include/`, and tests
and benchmark inputs in `Tests/`.

For solution builds, `scripts/windows/export-build-manifest.ps1 -ProjectRoot <root>
-SolutionPath <solution>` exports a manifest to `build/manifest/Release-x64.json`.
`build_project.ps1 -ProjectRoot <root>` consumes it. Build-driver defaults use
`build/`; explicit manifest, output, and compiler paths remain configurable.

## Test

Run the main test suite:

```bat
tests.cmd
```

Tests confirm C, C++, and assembler support.

```text
c_compat
features/All
features/Templates
features/Destructors
features/Constructors
features/InlineLifecycle
features/MemberFunctions
features/OperatorOverloads
```

## Contributing

Ideas, reports, experiments, bug reductions, and new test cases are welcome.

Use the issue tracker:

Good contributions include a concrete feature goal or error symptom:

1. show current/desired behavior
2. add or propose a test that reproduces it
3. fix the compiler only after the test proves the issue
