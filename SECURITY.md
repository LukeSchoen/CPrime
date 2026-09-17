# Security Policy

CPC compiles ASM, C, and C++17 into native x86-64 machine code, and it can also
emit and run generated functions in-process through its library interface. Bugs
in that path can matter more than bugs in an ordinary tool, so reports about
silently wrong machine code are treated as security issues.

## Supported versions

The tip of `main` and the `cpc.exe` published in that tree are the only
supported version. Older builds, forks, locally patched copies, and
non-CPC hosts (the explicitly Clang-named entry points) are not covered; a fix
is developed against the current tree and any backport is decided case by case.

## Reporting a vulnerability

Report privately through GitHub, using the **Report a vulnerability** button on
the Security tab or this direct link:

<https://github.com/LukeSchoen/CPrime/security/advisories/new>

A useful report includes:

- the exact commit hash or build you tested, and the OS,
- the command line, input files, and any manifest or project root involved,
- what you expected versus what happened, with a crash dump, sanitizer-style
  output, or the emitted ASM/C when that is the evidence,
- the smallest standalone reproducer you can construct.

Please keep details out of public issues until a fix is published. If the
private reporting form is unavailable, open an issue that asks for a private
channel without including the technical details.

## In scope

- Memory errors in the compiler, preprocessor, assembler, linker, or runtime
  code generation: out-of-bounds access, use-after-free, uninitialized use,
  double free, stack exhaustion from recursive input.
- Inputs that hang the compiler or make it consume unbounded memory or disk
  beyond what the input's size and shape reasonably justify.
- Miscompilation: source that is accepted and produces wrong, unsafe, or
  non-deterministic machine code without a diagnostic. Silent corruption of
  generated code is a security bug, not a quality bug.
- Path handling: include resolution, output paths (`-o`), build manifest and
  project-root handling, and build caches that read, write, or delete files
  outside the tree the user named.
- Supply chain: a tracked binary (`cpc.exe`, `src/scripts/*.exe`,
  `src/lib/*.a`, `*/tests/test.exe`) that does not match the sources it claims
  to be built from, or a build step that can be redirected to take code from an
  unexpected location.
- In-process code generation: ways a host that embeds CPC for JIT use can be
  made to execute unintended code or have its own memory state corrupted by
  compiled input.

## Out of scope

- Programs you compile with CPC. Their behavior belongs to their authors.
- Running CPC as if it were a sandbox. The compiler is not one: compiling
  attacker-supplied source in a privileged or sensitive context, then executing
  what it produced, is not a vulnerability in CPC. Report a sandbox escape in
  the sandbox, not in the compiler.
- Vendored third-party code under `src/third-party/`. Report it upstream; when
  a fix lands, CPrime vendors it. See each directory for its own license and
  notice.
- Missing compiler hardening flags, optimization quality, or a diagnostic that
  a maintainer would prefer to be clearer.

## What to expect

- Acknowledgement within five business days.
- An assessment with a fix plan, or a reasoned explanation of why the report is
  out of scope.
- Coordinated disclosure: a default of 90 days from acknowledgement, or sooner
  once a fix is published. We will credit you in the advisory unless you prefer
  otherwise.

There is no bug bounty. Good-faith research on your own copies, using the
minimum access needed to demonstrate the issue, is welcome and will not be
treated as hostile; do not test against GitHub infrastructure or systems you do
not own.
