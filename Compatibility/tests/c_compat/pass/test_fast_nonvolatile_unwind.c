// EXPECT_EXIT: 0
// EXPECT_COMPILE_ARGS: -O2
#include <windows.h>
NTSYSAPI PEXCEPTION_ROUTINE NTAPI RtlVirtualUnwind(
    DWORD, DWORD64, DWORD64, PRUNTIME_FUNCTION, PCONTEXT,
    PVOID *, PDWORD64, PKNONVOLATILE_CONTEXT_POINTERS);

static CONTEXT parent_context;
static int failed;

static void probe(void *return_pc)
{
    CONTEXT context;
    DWORD64 image_base;
    PVOID handler_data;
    DWORD64 frame;
    PRUNTIME_FUNCTION function;
    int i;
    RtlCaptureContext(&context);
    for (i = 0; i < 8 && context.Rip != (DWORD64)return_pc; ++i) {
        function = RtlLookupFunctionEntry(context.Rip, &image_base, 0);
        if (!function) { failed = 1; return; }
        RtlVirtualUnwind(0, image_base, context.Rip, function,
                         &context, &handler_data, &frame, 0);
    }
    if (context.Rip != (DWORD64)return_pc
        || context.R12 != parent_context.R12 || context.R13 != parent_context.R13
        || context.R14 != parent_context.R14 || context.R15 != parent_context.R15
        || context.Rbp != parent_context.Rbp) failed = 2;
}

static unsigned exercise(unsigned a, unsigned b, unsigned c, unsigned d)
{
    unsigned i;
    for (i = 0; i < 5; ++i) {
        a += 3; b += a; c ^= b; d += c;
        probe(__builtin_return_address(0));
        a += d; b ^= c; c += a; d ^= b;
    }
    return a ^ b ^ c ^ d;
}

int main(void)
{
    volatile unsigned result;
    RtlCaptureContext(&parent_context);
    result = exercise(1, 2, 3, 4);
    return failed || result == 0;
}
