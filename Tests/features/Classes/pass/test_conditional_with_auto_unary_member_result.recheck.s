# cpc -S assembly output

.text
.balign 8
.globl choose_conditional_auto_vec
.type choose_conditional_auto_vec,@function
choose_conditional_auto_vec:
  pushq %rbp
  movq %rsp, %rbp
  subq $64, %rsp
  movq %rcx, 16(%rbp)
  cmpl $0, %eax
  je .Lpc_24
  movl $1, %eax
  movq %rax, %r11
  movq %rax, %r10
  movq %r10, %rcx
  movq %r11, %rdx
  call ConditionalAutoVec__int_constructor_int
  movq %rax, %r10
  movq %r10, %rcx
  call ConditionalAutoVec__int_operator-_const
  movl %eax, -12(%rbp)
  jmp .Lpc_73
.Lpc_24:
  movl $1, %eax
  movq %rax, %r11
  movq %rax, %r10
  movq %r10, %rcx
  movq %r11, %rdx
  call ConditionalAutoVec__int_constructor_int
  jmp .Lpc_108
.Lpc_73:
.Lpc_108:
  movq %rax, %rsi
  movl -20(%rbp), %eax
  leave
  ret

.text
.balign 8
.globl main
.type main,@function
main:
  pushq %rbp
  movq %rsp, %rbp
  subq $32, %rsp
  movl $0, %eax
  leave
  ret

.text
.balign 8
.globl ConditionalAutoVec__int_constructor_int
.type ConditionalAutoVec__int_constructor_int,@function
ConditionalAutoVec__int_constructor_int:
  pushq %rbp
  movq %rsp, %rbp
  subq $32, %rsp
  movq %rcx, 16(%rbp)
  movq %rdx, 24(%rbp)
  movq 16(%rbp), %rax
  movl 24(%rbp), %ecx
  leave
  ret

.text
.balign 4
.uw_base:
  .byte 0x01, 0x04, 0x02, 0x05, 0x04, 0x03, 0x01, 0x50

.data
.balign 8

.section .rdata,"a"
.balign 8

.section .pdata,"a"
.balign 4
  .long .uw_base - 4194304
  .long .uw_base - 4194171
  .long .uw_base - 4194168
  .long .uw_base - 4194160
  .long .uw_base - 4194142
  .long .uw_base - 4194168
  .long .uw_base - 4194142
  .long .uw_base - 4194112
  .long .uw_base - 4194168
