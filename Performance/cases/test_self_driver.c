// PERF_NAME: c.self.driver
// PERF_SOURCE: src/compiler/driver/cprime.c
// PERF_TIER: heavy
// PERF_ITERATIONS: 3
// PERF_TCC: no
// PERF_ARGS: -Iinclude/runtime -Iinclude/cprime -Ithird-party/win32-sdk/include -Ithird-party/win32-sdk/include/winapi -Isrc/compiler/frontend -Isrc/compiler/middleend -Isrc/compiler/backend/x64 -I. -DCPRIME_TARGET_PE -DCPRIME_TARGET_X86_64
/* Real workload: the compiler compiling its own driver. tcc cannot accept this
   source, so the row is CPC-only and is compared against the reference CPC
   build, and against the recorded milliseconds when no reference is given.
   This file carries metadata only; PERF_SOURCE names the translation unit. */
