# CPrime: Near Instant Compile & Tiny Builds Of C - The expressiveness and comfort of C++ 

[Download the Windows zip](https://github.com/LukeSchoen/CPrime/archive/refs/heads/main.zip)

CPrime is a programming language and compiler for people who want the speed /
tiny build size of C, but also want larger projects with pleasant ergonomics.

The compiler is called **CPC**. It's job is to take any C code and most of cpp
Almost instantly makes lightweight native outputs, without bulky C++ toolchains.

## The Short Version

C is fast small and beautiful. C++ is powerful expressive but expensive to use,
in compile time, binary size, and toolchain install cost (setup time, and weight)

## Why cPrime Exists

C++ offers a lot of developer comfort: classes, constructors, destructors,
templates, overloaded functions (great ways to organize your large projects)

But the price is steep:

- slow builds
- huge toolchains
- large executables
- layers of legacy based complexity
- features that authors like but that cost every build forever

CPrime asks a simple question:

> What if using C++ could be done in a way that was just really cheap?

## What Do we have Today?

- Full C compatibility: structs, enums, function calls, recursion, pointers,
  arrays, macros, stdio, and compile-fail cases
- Most Of C++: constructors, local variables, classes/struct-style types
- destructors, member functions like `foo.bar(a, b)`
- static member functions and static data members
- function templates and template class specifiers
- function and class-style operator overloads (+= etc)

Some features are deliberately narrow at the moment. For example, templates
`template<typename T>` are limited to a single Type T (to stop SPHINAE etc)

## What Is Intentionally left Out?

Features that are currently intentionally dropped as unwanted:

- namespaces
- inheritance
- class friends

As these tend to produce Rabit-Warrens.

The spirit is simple: avoid features that make code hard to read
complexity needs to payoff and allign realisticallly with goals

## Build

The build.and script expects `cpc.exe` at the repo root: It builds a fresh
new compiler and replaces the old compiler with the newly built one.

## Test

Run the main test suite:

```bat
tests.cmd
```

Tests both confirm C still works and ensures suppport for desired c++ code.

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
