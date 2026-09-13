// PERF_NAME: cost.self.driver
// PERF_SOURCE: src/compiler/driver/cprime.c
// PERF_TIER: heavy
// PERF_ITERATIONS: 7
// PERF_TCC: no
// PERF_ARGS: -Iinclude/runtime -Iinclude/cprime -Ithird-party/win32-sdk/include -Ithird-party/win32-sdk/include/winapi -Isrc/compiler/frontend -Isrc/compiler/middleend -Isrc/compiler/backend/x64 -I. -DCPRIME_TARGET_PE -DCPRIME_TARGET_X86_64
// Diagnostic self-compilation; the same complete driver and arguments as the performance suite.
