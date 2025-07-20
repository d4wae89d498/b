.intel_syntax noprefix
.section .note.GNU-stack,"",@progbits
.text
.data
.data
str0: .asciz "Hello %i"
.text
.globl main
main:
    push ebp
    mov ebp, esp
    sub esp, 0 #locals
    mov eax, 42
    push eax #arg 1
    lea eax, [str0] #string literal
    push eax #arg 0
    sub esp, 4 #align stack to 16 bytes
    call printf
    add esp, 4 #restore stack alignment
    add esp, 8 #pop args
    mov esp, ebp
    pop ebp
    ret
