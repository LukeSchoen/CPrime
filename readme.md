# CPrime A Fast C++17 Compiler

Is a self-contained, one-pass C++ compiler,
  It lowers C++ constructs directly to x86-64
  machine code.
  
  templates, constexpr, exception/RTTI, standard library etc
  
  CPC does not cheat by shelling out to Clang, GCC, or MSVC.
  cpc.exe preprocesses, parses, type-checks, generates x86-64 machine
  code, assembles, writes PE objects and creates the executable itself.
  CPC is also able to compile itself.

How CPC works
  CPC is a syntax-directed one-pass, compiler. The important consequence-
  is that there is no intermediate/separate AST or IR stage/representation!
  Expressions are parsed and machine code is Emmited AS-THE-PARSER-PROCEEDS.

This limits what can be done but allows for vastly-faster code compilation.
C++ semantics must therefore be implemented directly in the parser / code generator:
  That is what makes CPC “more direct” than Clang or GCC. which normally goes through a rich AST and LLVM IR before
  optimization and backend lowering; They normally drive separate assembler and linker programs. CPC goes from tokens
  to machine code with MUCH-LESS machinery. The tradeoff is optimization depth: CPC’s optimizer is a bounded peephole
  register-promotion passes and small inlining CPC -O2 is not remotely equivalent to the -O2 mode in Clang or GCC.

Use root `cpc.exe` for development. `scripts/build.exe` rebuilds it serially with
that same CPC host and replaces root `cpc.exe` after validation. There are no
host-selection flags or alternate working compiler copies. Failure preserves
the working compiler and stops the build.
The C-only seed proof is `scripts/seed-tcc.exe -RunExternal`; the resulting CPC
is built before any packaged C++ runtime source is compiled.
Agents must not switch to Clang, GCC, MSVC, or another compiler for builds,
