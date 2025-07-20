.intel_syntax noprefix
.section .note.GNU-stack,"",@progbits
.text
.data
.data
str0: .asciz "%d"
.text
.globl main
main:
    push ebp
    mov ebp, esp
    sub esp, 8 #locals
#    i variable declaration
#    y variable declaration
    lea eax, [ebp-4] #var i
    push eax #save lvalue addr
    mov eax, 0
    pop ebx #restore lvalue addr
    mov [ebx], eax #assign
.L0:
    mov eax, [ebp-4] #var i
    push eax
    mov eax, 10
    mov ebx, eax
    pop eax
    cmp eax, ebx
    setl al
    movzx eax, al #relational result
    test eax, eax
    jz .L1
    mov eax, [ebp-4] #var i
    push eax #arg 1
    lea eax, [str0] #string literal
    push eax #arg 0
    call printf
    add esp, 8 #pop args
    lea eax, [ebp-8] #var y
    push eax #save lvalue addr
    mov eax, [ebp-4] #var i
    push eax
    mov eax, 1
    mov ebx, eax
    pop eax
    add eax, ebx
    pop ebx #restore lvalue addr
    mov [ebx], eax #assign
    lea eax, [ebp-4] #var i
    push eax #save lvalue addr
    lea eax, [ebp-8] #var y
    push eax #save lvalue addr
    mov eax, [ebp-8] #var y
    pop ebx #restore lvalue addr
    mov [ebx], eax #assign
    pop ebx #restore lvalue addr
    mov [ebx], eax #assign
    jmp .L0
.L1:
    mov esp, ebp
    pop ebp
    ret
