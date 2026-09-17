#CPrime

An ultra-fast ASM, C, and C++17 compiler.

CPrime (CPC) can:

Read ASM, C, and C++17
Write x64 executables, ASM, and C
Assemble ASM directly to machine code
Compile C and C++ directly to x86-64 machine code
Compile itself and bootstrap from a pure C compiler
Prime Development Via Extreme Speed

CPC is designed around compilation speed rather than optimization depth.

C: >30% faster than the fastest C compilers such as TCC and QBE
ASM: >25% faster than the next-fastest assemblers such as FASM and MASM
C++: 100% to 20,000% faster than GCC, Clang, MSVC, and other C++ compilers

Across several large and difficult C++ projects, CPC came out over 20x faster than MSVC or Clang.

The exact speedup depends on the project, compiler options, and workload, but, you WILL be happy.

#Direct Compilation
CPC is self-contained, single-pass and designed for ultra-high-speed source-code conversion.
CPC does not build any kind of intermediate AST or conventional separate IR representation.

Expressions are instead parsed, checked, and emitted as machine code As-The-Parser-Proceeds.

This directness makes CPC substantially faster than traditional compiler architectures such as GCC or Clang,
which inevitably must pass through multiple rich intermediate representations before producing machine code.

CPC also directly handles assembly and linking internally rather than driving separate libs or programs.

CPC is forced to implement C++ semantics directly in the parser / code generator.
Its optimizer is therefore minimal, only using cheap effective techniques such as:
Register promotion, Small-scale inlining and Peephole optimization

CPC -O2 is NOT comparable to GCC or Clangs -O2.

CPC Minimizes compilation cost while producing perfectly Fast-Enough native code.

During dev iteration, reducing C++ compilation times for a small reduction in exe speed is a Godly trade.

Self-Hosting
CPC can compile itself.
The compiler is also able to bootstrap itself using pure C, allowing you to reseed it from a simple C compiler.

Project Layout
src/
  The toolchain:
  compiler, runtime, native tools, workflow scripts,
  headers, bootstrap libraries, shipped libcprime SDK,
  and vendored third-party sources.

  src/include/
    Headers

  src/lib/
    Bootstrap libraries

  src/deploy/
    Shipped libcprime SDK

  src/third-party/
    Vendored sources

#Compatibility/
  C++17 correctness

#Cost/
  Compilation-speed

#Capability/
  Output-programs performance

CPC is Particularly useful for code bases where you already have advanced performance from via explicit low-level implementatiosn.
For example, SSE routines written in ASM already execute at essentially their intended machine-code speed.
They don't need optimizing to run fast. This makes CPC a perfect fit for projects using AVX or SSE code.

#CPC can also be used as a RUNTIME code-generation system.

CPC provides in-memory compilation through a DLL/library interface:
Allowing ASM, C, and C++ source to be compiled into native functions with Extremely-Low-Latency.

Generated functions can then become directly callable functions within the host program,
They can also call back into the rest of the application.

This makes CPC useful for applications that may need to generate and execute native-code dynamically.
