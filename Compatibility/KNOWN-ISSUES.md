# Known CPC issues

━━━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
   clCRC.cpp            Compiler bug, reproduced in ~20 lines: an out-of-class member function template whose
                        declaration and definition spell a parameter type with a leading :: (::G) is replayed
                        without its declarator → incompatible types for redefinition … versus 'inline unsigned long
                        long'. Last compiler bug before the build proceeds.
  ───────────────────  ──────────────────────────────────────────────────────────────────────────────────────────────
   faceDetectCNN.cpp    __m256 undeclared — CPrime's immintrin.h is a stub; AVX vector types are unimplemented. (we want sse / avx implemented)
  ───────────────────  ──────────────────────────────────────────────────────────────────────────────────────────────
   clProcessList.cpp    psapi.h is absent from third-party/win32-sdk/include. The installed Windows SDK has um/
                        Psapi.h; vendoring one is a packaging call I left to you.


## RTMPose / Kpose findings (2026-09-16)

Measured on the development machine (Intel i5-8250U, 4 cores/8 threads):

| Model | Inference per frame |
| --- | --- |
| rtmpose-t (default) | ~0.53 s |
| rtmpose-m | ~2.9 s |

Convolutions dominate at roughly 85% of inference time. Threading the
convolutions over output pixels gives only about 1.35x here; `KPOSE_THREADS=1`
forces single-threaded execution for A/B measurements. A hand-written SSE GEMM
measured about 5x faster than the C loop in isolation, but cpc miscompiles it
as recorded below, so the shipped Kpose kernels remain portable C.

The external reproducer and notes are in `C:\Luke\Src\Kinect\README.md`;
fixtures and models are regenerated with
`python tools\vendor_rtmpose.py --all`.

### InterlockedIncrement ignores its result

`InterlockedIncrement` always returns 0 instead of the new value. An atomic
work counter therefore never advances.

Minimal reproducer shape:

```c
long next = InterlockedIncrement(&job->next) - 1;
/* The loop never observes 1. */
```

The Kpose kernels use static slice partitioning because of this bug.

Work required:

- Reduce the reproducer to a standalone CPrime-local test.
- Fix the intrinsic lowering so the returned value is the post-increment value.
- Verify the fixed intrinsic and the retained test in the affected suite.

### Inline SSE asm corrupts surrounding float code

Inline SSE asm using `movups`/`mulps`/`addps` and `xmm0`-`xmm2` corrupts
surrounding cpc-generated float code. A function that runs the asm and then
does scalar float maths crashes with `0xC0000005`.

The reduced shape is a GEMM where `N=25` crashes while `N=24` and `N=28` do
not. `cpc -b` and separate-function layouts do not avoid the failure.
`__attribute__((vector_size(16)))` works but compiles about 3x slower than the
plain C loop.

Work required:

- Reduce the `N=25` GEMM to a standalone CPrime-local reproducer.
- Correct register allocation/liveness across inline asm and generated float
  code.
- Retain the reproducer as a regression case.
- Re-check the isolated SSE GEMM performance after the fix.
