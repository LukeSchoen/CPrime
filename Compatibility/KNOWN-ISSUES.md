# Known CPC issues

Open defects, each with its reduced shape and the work it needs. Completed work
is code and retained cases; nothing here is a progress log.

## clCRC.cpp: a leading `::` in a replayed member function template

An out-of-class member function template whose declaration and definition spell
a parameter type with a leading `::` (`::G`) is replayed without its declarator,
so the redefinition reports incompatible types (`... versus 'inline unsigned
long long'`). The reproducer is about 20 lines in the clCRC checkout.

Work required:

- Reduce it to a standalone CPrime-local case.
- Keep the declarator when the replay re-spells the parameter type.
- Retain the case in the suite that owns the behavior.

## faceDetectCNN.cpp: SSE and AVX vector types

`__m256` is undeclared: CPrime's `immintrin.h` is a stub, so AVX vector types
are unimplemented. The project also needs the SSE types and intrinsics.

Work required:

- Implement the SSE vector types and intrinsics the project uses, then the AVX
  ones.
- Retain a compile-and-run case for the types and for the intrinsics added.

## clProcessList.cpp: psapi.h

`psapi.h` is absent from `third-party/win32-sdk/include`; the installed Windows
SDK ships `um/Psapi.h`.

Work required:

- Choose the packaging shape for the missing SDK header and add it.
- Compile and run the uses the project makes of the process API.

## RTMPose / Kpose kernels (`C:\Luke\Src\Kinect`, read only)

Convolutions dominate inference at roughly 85% of the time, and a hand-written
SSE GEMM measured about 5x faster than the C loop in isolation, but it cannot
ship while the two defects below are open, so the Kpose kernels stay portable C.
`KPOSE_THREADS=1` forces single-threaded execution for A/B measurements. The
external reproducer and notes are in `C:\Luke\Src\Kinect\README.md`; fixtures
and models are regenerated with `python tools\vendor_rtmpose.py --all`.

### InterlockedIncrement ignores its result

`InterlockedIncrement` returns 0 instead of the new value, so an atomic work
counter never advances.

Reduced shape:

```c
long next = InterlockedIncrement(&job->next) - 1;
/* The loop never observes 1. */
```

Work required:

- Reduce the reproducer to a standalone CPrime-local test.
- Lower the intrinsic to return the post-increment value.
- Run the fixed intrinsic and the retained test in the affected suite.

### Inline SSE asm corrupts surrounding float code

Inline SSE asm using `movups`/`mulps`/`addps` and `xmm0`-`xmm2` corrupts the
surrounding generated float code: a function that runs the asm and then does
scalar float maths crashes with `0xC0000005`. The reduced shape is a GEMM whose
`N=25` crashes while `N=24` and `N=28` do not; `cpc -b` and separate-function
layouts do not avoid it. `__attribute__((vector_size(16)))` works but compiles
about 3x slower than the plain C loop.

Work required:

- Reduce the `N=25` GEMM to a standalone CPrime-local reproducer.
- Fix register allocation and liveness across inline asm and generated float
  code.
- Retain the reproducer as a regression case.
- Re-check the isolated SSE GEMM performance after the fix.

## Explorer++ probe: `[[noreturn]]` between declaration specifiers

Probe: `C:\Luke\Src\Archive\explorerplusplus`, a manifest build of 232
translation units driven by `src\scripts\project.exe`; entry point
`build_cpc.cmd`, reduced cases and the re-run command in `Scripts\cpc\gaps` and
`Scripts\cpc\Test-CpcGaps.ps1`. Compile it with root `cpc.exe`, reduce each
failure to a minimal local case, and never edit the consumer.

The build stops in `boost/throw_exception.hpp` as reached from
`boost/type_index/stl_type_index.hpp`: a function template declared
`[[noreturn]]`. Its replay emits `__attribute((weak)) inline` in front of the
copied declaration, so the standard attribute lands between declaration
specifiers, which `parse_btype` rejects with `identifier expected`.

Reduced shape:

```cpp
inline [[noreturn]] void gap_f(int const &e);
```

Work required:

- Accept a standard attribute-specifier sequence between declaration
  specifiers; the fast-tier case
  `features/Cpp17Gaps/pass/test_attribute_before_function_template.cpp` covers
  the instantiation shape.
- Continue from the next floor the consumer build reports.

## `std::abs` overload set is ambiguous after a using-declaration import

`std::abs(-1L)` reports `ambiguous overloaded function '__cpc_ns_std_abs'`. The
shipped `<cstdlib>` imports the C declarations into `namespace std` and then
declares the same signatures again, as the reference implementation does.
Fast-tier case:
`features/All/pass/test_runtime_absolute_value_overloads.cpp`.

Reduced shape:

```cpp
inline int abs(int v) { return v; }
inline long abs(long v) { return v; }
inline double abs(double v) { return v; }

namespace std {
  using ::abs;
  inline int abs(int v) { return v; }
  inline long abs(long v) { return v; }
}

int main() { return (int)std::abs(-1L) == -1 ? 0 : 1; }
```

Work required:

- Make an overload set imported by a using-declaration and a same-signature
  declaration in the importing namespace form one set, so the exact `long` match
  wins.
