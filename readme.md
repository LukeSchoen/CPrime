# CPrime: Near Instant C, C++, and Assembler Compilation

[Download the Windows zip](https://github.com/LukeSchoen/CPrime/archive/refs/heads/main.zip)

CPrime is a fast assembler, C compiler, and C++ compiler for people who want
near instant builds, tiny native outputs, and the full expressiveness of C++.

The compiler is called **CPC**. Its job is to take assembler, C, and C++ code
and almost instantly produce lightweight native outputs, without bulky C++
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
- layers of legacy based complexity
- features that authors like but that cost every build forever

CPrime asks a simple question:

> What if using C++ could be done in a way that was just really cheap?

## What Do We Have Today?

- Full C compatibility: structs, enums, function calls, recursion, pointers,
  arrays, macros, stdio, and compile-fail cases
- 100% C++ support
- Fast assembler support
- Lightweight native outputs
- Near instant compile times
- Small toolchain footprint

## Build

The build.and script expects `cpc.exe` at the repo root: It builds a fresh
new compiler and replaces the old compiler with the newly built one.

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
