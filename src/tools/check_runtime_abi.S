.text
.globl check_stack_probe_abi
check_stack_probe_abi:
    push %rbp
    push %r12
    sub $40, %rsp
    mov %rsp, %r12
    mov $12336, %eax
    mov $17, %ecx
    mov $23, %edx
    mov $31, %r8d
    mov $47, %r9d
    call __chkstk
    /* Keep a nonvolatile copy of RSP so even the bootstrap helper can be
       rejected cleanly after it allocates a frame and changes RBP/RAX. */
    cmp %r12, %rsp
    jne L_bad_probe
    cmp $12336, %rax
    jne L_bad_probe
    cmp $17, %rcx
    jne L_bad_probe
    cmp $23, %rdx
    jne L_bad_probe
    cmp $31, %r8
    jne L_bad_probe
    cmp $47, %r9
    jne L_bad_probe
    xor %eax, %eax
    jmp L_restore
L_bad_probe:
    mov $1, %eax
L_restore:
    mov %r12, %rsp
    add $40, %rsp
    pop %r12
    pop %rbp
    ret
