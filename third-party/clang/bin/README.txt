Missing: clang.exe (formerly Clang 12.0.0, x86_64-pc-windows-msvc).

The optional 115 MB host compiler was removed to keep this repository small.
Normal self-hosted CPC builds use the preserved root cpc.exe.

To use an existing Clang installation, run from the repository root:
  powershell -File BuildProfile/build-cpc-clang.ps1 -ClangPath C:\path\to\clang.exe

Alternatively, copy clang.exe into this directory for BuildClang.cmd to use it.
The local binary is ignored by Git and should not be committed again.
