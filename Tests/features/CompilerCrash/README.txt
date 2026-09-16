Compiler crash suite

Scope:
- One case per crash, each a valid C++17 program the compiler must accept.
- A crash aborts the rest of the suite's compile batch, so every crashing
  reproducer needs its own suite; keeping them here stops one crash from
  masking unrelated cases in a shared batch.
- A case leaves this suite when the crash is repaired; if the program still
  fails afterwards it moves to the suite that owns the language behavior.

Run:
- Tests\test.exe -Suite features/CompilerCrash
