/* Win64 runtime support for GCC's same-function __builtin_setjmp and
   __builtin_longjmp.  GCC documents a save area of five void *, which is
   smaller than the CRT jmp_buf, so the frontend lowers these builtins onto
   the helpers below instead of wrapping setjmp.

   __cpc_builtin_setjmp records the caller's resume address, the stack pointer
   the caller will have once this call returns, and the caller's frame pointer,
   then reports zero.  __cpc_builtin_longjmp restores that state and jumps back
   to the resume address, so the caller continues as if __cpc_builtin_setjmp
   had returned the value (never zero).  Both helpers touch only volatile
   registers, so the resumed frame needs no further register restoration. */

    .text

    .globl __cpc_builtin_setjmp
__cpc_builtin_setjmp:
    movq (%rsp), %rax
    movq %rax, 16(%rcx)
    leaq 8(%rsp), %rax
    movq %rax, 0(%rcx)
    movq %rbp, 8(%rcx)
    xorl %eax, %eax
    ret

    .globl __cpc_builtin_longjmp
__cpc_builtin_longjmp:
    movl %edx, %eax
    testl %eax, %eax
    jne 1f
    movl $1, %eax
1:  movq 0(%rcx), %rsp
    movq 8(%rcx), %rbp
    jmp *16(%rcx)
