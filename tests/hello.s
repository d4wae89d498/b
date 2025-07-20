.intel_syntax noprefix
.section .note.GNU-stack,"",@progbits
.text
.data
.data
str0: .asciz "Hello world"
.text
.globl main
main:
    push ebp
    mov ebp, esp
    sub esp, 0 #locals
    lea eax, [str0] #string literal
    push eax #arg 0
    call printf
    add esp, 4 #pop args
    mov esp, ebp
    pop ebp
    ret
