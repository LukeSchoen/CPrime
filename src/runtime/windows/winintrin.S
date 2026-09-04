/* Windows x64 compatibility intrinsics.  These must not be ordinary C
   callees when their semantics are instruction-level or atomic. */

    .text

    .globl _BitScanForward64
_BitScanForward64:
    testq %rdx, %rdx
    jz 1f
    bsfq %rdx, %rax
    movl %eax, (%rcx)
    movl $1, %eax
    ret
1:
    xorl %eax, %eax
    ret

    .globl _InterlockedExchangeAdd
_InterlockedExchangeAdd:
    movl %edx, %eax
    lock xaddl %eax, (%rcx)
    ret

    .globl __debugbreak
__debugbreak:
    int3
    ret

/* alloca is called like a normal Win64 function, but returns with the
   caller's stack pointer moved down by the size supplied in RCX.  The backend
   includes its outgoing argument area in that size and adds the same area to
   the returned RSP, yielding the user allocation while keeping later calls
   ABI-correct. */
    .globl alloca
alloca:
    popq %r11
    addq $15, %rcx
    andq $-16, %rcx
    movq %rsp, %r10
    movq %rcx, %rdx
1:
    cmpq $4096, %rdx
    jb 2f
    subq $4096, %r10
    testb $0, (%r10)
    subq $4096, %rdx
    jmp 1b
2:
    subq %rdx, %r10
    testb $0, (%r10)
    movq %r10, %rsp
    movq %rsp, %rax
    pushq %r11
    ret
