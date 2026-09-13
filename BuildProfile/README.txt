Native profiling workflow

Use scripts\profile.exe with a map file, target executable, and response file.
It samples the current process and reports symbols from the matching image.

Use scripts\build-clang.exe only for an explicitly authorized Clang host build;
the executable additionally requires -RunExternal. CPC remains the normal host.

Generated profiles belong under build\.
