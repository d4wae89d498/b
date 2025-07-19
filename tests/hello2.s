.intel_syntax noprefix
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
    call printf
    add esp, 8 #pop args
    mov esp, ebp
    pop ebp
    ret
