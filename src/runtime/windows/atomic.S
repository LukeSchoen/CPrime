/* Win64 runtime primitives for the compiler's __atomic_* lowering.
   Naturally aligned 1/2/4/8-byte loads are atomic on x64. Plain loads supply
   acquire (and, with the locked stores below, sequentially consistent) loads.
   Release stores need no hardware fence; seq_cst stores use implicit-lock
   XCHG. Compare-exchange uses a full-barrier locked instruction for both
   strong and weak operations and writes the observed value on failure.
   All functions are leaf functions and leave nonvolatile registers intact. */

    .text

    .globl __atomic_load_1
__atomic_load_1:
    movzbl (%rcx), %eax
    ret
    .globl __atomic_load_2
__atomic_load_2:
    movzwl (%rcx), %eax
    ret
    .globl __atomic_load_4
__atomic_load_4:
    movl (%rcx), %eax
    ret
    .globl __atomic_load_8
__atomic_load_8:
    movq (%rcx), %rax
    ret

    .globl __atomic_store_1
__atomic_store_1:
    cmpl $5, %r8d
    je 1f
    movb %dl, (%rcx)
    ret
1:  xchgb %dl, (%rcx)
    ret
    .globl __atomic_store_2
__atomic_store_2:
    cmpl $5, %r8d
    je 1f
    movw %dx, (%rcx)
    ret
1:  xchgw %dx, (%rcx)
    ret
    .globl __atomic_store_4
__atomic_store_4:
    cmpl $5, %r8d
    je 1f
    movl %edx, (%rcx)
    ret
1:  xchgl %edx, (%rcx)
    ret
    .globl __atomic_store_8
__atomic_store_8:
    cmpl $5, %r8d
    je 1f
    movq %rdx, (%rcx)
    ret
1:  xchgq %rdx, (%rcx)
    ret

    .globl __atomic_compare_exchange_1
__atomic_compare_exchange_1:
    movzbl (%rdx), %eax
    lock cmpxchgb %r8b, (%rcx)
    jne 1f
    movl $1, %eax
    ret
1:  movb %al, (%rdx)
    xorl %eax, %eax
    ret
    .globl __atomic_compare_exchange_2
__atomic_compare_exchange_2:
    movzwl (%rdx), %eax
    lock cmpxchgw %r8w, (%rcx)
    jne 1f
    movl $1, %eax
    ret
1:  movw %ax, (%rdx)
    xorl %eax, %eax
    ret
    .globl __atomic_compare_exchange_4
__atomic_compare_exchange_4:
    movl (%rdx), %eax
    lock cmpxchgl %r8d, (%rcx)
    jne 1f
    movl $1, %eax
    ret
1:  movl %eax, (%rdx)
    xorl %eax, %eax
    ret
    .globl __atomic_compare_exchange_8
__atomic_compare_exchange_8:
    movq (%rdx), %rax
    lock cmpxchgq %r8, (%rcx)
    jne 1f
    movl $1, %eax
    ret
1:  movq %rax, (%rdx)
    xorl %eax, %eax
    ret

    /* The legacy __sync compare-and-swap pair takes the expected value
       directly instead of through a pointer and reports either the outcome
       of the comparison or the value that was observed. */
    .globl __sync_bool_compare_and_swap_1
__sync_bool_compare_and_swap_1:
    movb %dl, %al
    lock cmpxchgb %r8b, (%rcx)
    jne 1f
    movl $1, %eax
    ret
1:  xorl %eax, %eax
    ret
    .globl __sync_bool_compare_and_swap_2
__sync_bool_compare_and_swap_2:
    movw %dx, %ax
    lock cmpxchgw %r8w, (%rcx)
    jne 1f
    movl $1, %eax
    ret
1:  xorl %eax, %eax
    ret
    .globl __sync_bool_compare_and_swap_4
__sync_bool_compare_and_swap_4:
    movl %edx, %eax
    lock cmpxchgl %r8d, (%rcx)
    jne 1f
    movl $1, %eax
    ret
1:  xorl %eax, %eax
    ret
    .globl __sync_bool_compare_and_swap_8
__sync_bool_compare_and_swap_8:
    movq %rdx, %rax
    lock cmpxchgq %r8, (%rcx)
    jne 1f
    movl $1, %eax
    ret
1:  xorl %eax, %eax
    ret

    /* Both forms leave the observed (pre-exchange) value in %rax. */
    .globl __sync_val_compare_and_swap_1
__sync_val_compare_and_swap_1:
    movb %dl, %al
    lock cmpxchgb %r8b, (%rcx)
    movzbl %al, %eax
    ret
    .globl __sync_val_compare_and_swap_2
__sync_val_compare_and_swap_2:
    movw %dx, %ax
    lock cmpxchgw %r8w, (%rcx)
    movzwl %ax, %eax
    ret
    .globl __sync_val_compare_and_swap_4
__sync_val_compare_and_swap_4:
    movl %edx, %eax
    lock cmpxchgl %r8d, (%rcx)
    ret
    .globl __sync_val_compare_and_swap_8
__sync_val_compare_and_swap_8:
    movq %rdx, %rax
    lock cmpxchgq %r8, (%rcx)
    ret

    /* The test-and-set builtin, like atomic_flag, accesses one byte. */
    .globl __atomic_test_and_set_1
    .globl __atomic_test_and_set_2
    .globl __atomic_test_and_set_4
    .globl __atomic_test_and_set_8
    .globl atomic_flag_test_and_set
    .globl atomic_flag_test_and_set_explicit
__atomic_test_and_set_1:
__atomic_test_and_set_2:
__atomic_test_and_set_4:
__atomic_test_and_set_8:
atomic_flag_test_and_set:
atomic_flag_test_and_set_explicit:
    movl $1, %eax
    xchgb %al, (%rcx)
    ret

    .globl atomic_flag_clear
atomic_flag_clear:
    xorl %eax, %eax
    xchgb %al, (%rcx)
    ret
    .globl atomic_flag_clear_explicit
atomic_flag_clear_explicit:
    movl %edx, %r8d
    xorl %edx, %edx
    jmp __atomic_store_1

    .globl atomic_thread_fence
atomic_thread_fence:
    cmpl $5, %ecx
    jne 1f
    mfence
1:  ret
    .globl atomic_signal_fence
atomic_signal_fence:
    ret
