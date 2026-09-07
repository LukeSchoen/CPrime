# GCC C++ regression assessment

Upstream: https://github.com/gcc-mirror/gcc

Pinned revision: `5f6257c26b814de1a14c71b2d3a49291765b6577` (2026-09-07).

The runner fetches a sparse checkout at `build/gcc-upstream` contains `gcc/testsuite/g++.dg`,
`g++.old-deja`, `c-c++-common`, and shared test support. Upstream source and
copyright/license files remain unchanged in that checkout. The pin makes the
download reproducible; it is not a rolling dependency.

Run from the CPrime root:

```powershell
python Tests/gcc/run.py --fetch
python Tests/gcc/run.py --select g++.dg/init/ --out build/gcc-init
python Tests/gcc/run.py --probe --timeout 2 --out build/gcc-all
python Tests/gcc/test_runner.py
```

`--select` accepts repeatable relative path prefixes, including individual
filenames. `--compiler` selects a compiler executable. All invocations are
serial, with a freshly removed output artifact before each compilation.
Timeouts apply separately to compilation and execution; timed-out processes
are killed and waited for by Python before continuing.

This is a conservative CPC assessment adapter, **not a replacement for
DejaGnu and not a claim that the whole GCC testsuite passes**. It inventories
source files, including support sources; dedicated upstream `.exp` drivers
can define more complex test groupings. It never evaluates upstream Tcl.
The ordinary G++ driver includes only the root and `cpp/` portions of
`c-c++-common`; other shared directories require specialized drivers and are
classified accordingly.

Supported unconditional actions: preprocess, compile/assemble to an object,
link, and run with expected exit zero. In particular, `PASS_COMPILE` measures
CPC object generation, not GCC's assembly output or exact diagnostics.
Supported explicit options are currently optimization levels, debug info,
and the inline/builtin opt-outs. CPC's default C++ mode is used; GCC's default
pedantic checks and standard-version matrix are not reproduced.

Conditional target selectors, standard-selection flags, diagnostic checks,
assembly/tree-dump scans, extra source files, and special drivers are reported
as `UNSUPPORTED`. They are not counted as passes. `--probe` additionally tries
those files as standalone C++ sources with the understood options, recording
`PROBE_ACCEPTED` or `PROBE_REJECTED`. These observations do **not** establish
the expected result. A compiler crash is never treated as a successful
rejection, even for a test containing `dg-error`.

Each output directory contains:

- `metadata.json`: upstream revision, compiler path/hash, settings and scope.
- `results.jsonl`: every source, unsupported reasons, command, exit status,
  timing, compiler output, and runtime result when applicable.
- `summary.json`: aggregate statuses. Nonzero runner exit means an executed
  checked case failed, or a compiler invocation crashed/timed out.

The current adapter deliberately leaves diagnostic expectations unchecked:
a nonzero compiler exit is insufficient evidence that the intended error was
diagnosed. Add directive support with adapter regression tests before changing
that classification. Fix compiler failures in CPC; do not edit upstream tests
or convert new failures to expected failures to improve the totals.

GCC directive semantics:
https://gcc.gnu.org/onlinedocs/gccint/Directives.html
