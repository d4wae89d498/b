.intel_syntax noprefix
.section .note.GNU-stack,"",@progbits
.text
.data
.data
str0: .asciz "%d "
str1: .asciz "%d %d "
str2: .asciz "%d %d"
.text
.globl main
main:
    push ebp
    mov ebp, esp
    sub esp, 8 #locals
#    i variable declaration
#    j variable declaration
    lea eax, [ebp-4] #var i
    push eax #save lvalue addr
    mov eax, 0
    pop ebx #restore lvalue addr
    mov [ebx], eax #assign
    lea eax, [ebp-8] #var j
    push eax #save lvalue addr
    mov eax, 0
    pop ebx #restore lvalue addr
    mov [ebx], eax #assign
    lea eax, [ebp-4] #var i
    inc dword ptr [eax] #increment
    mov eax, [eax] #return new value
    mov eax, [ebp-4] #var i
    push eax #arg 1
    lea eax, [str0] #string literal
    push eax #arg 0
    sub esp, 4 #align stack to 16 bytes
    call printf
    add esp, 4 #restore stack alignment
    add esp, 8 #pop args
    lea eax, [ebp-4] #var i
    mov ebx, eax #save address
    mov eax, [ebx] #load original value
    push eax #save original value
    inc dword ptr [ebx] #increment
    pop eax #return original value
    mov eax, [ebp-4] #var i
    push eax #arg 1
    lea eax, [str0] #string literal
    push eax #arg 0
    sub esp, 4 #align stack to 16 bytes
    call printf
    add esp, 4 #restore stack alignment
    add esp, 8 #pop args
    lea eax, [ebp-4] #var i
    dec dword ptr [eax] #decrement
    mov eax, [eax] #return new value
    mov eax, [ebp-4] #var i
    push eax #arg 1
    lea eax, [str0] #string literal
    push eax #arg 0
    sub esp, 4 #align stack to 16 bytes
    call printf
    add esp, 4 #restore stack alignment
    add esp, 8 #pop args
    lea eax, [ebp-4] #var i
    mov ebx, eax #save address
    mov eax, [ebx] #load original value
    push eax #save original value
    dec dword ptr [ebx] #decrement
    pop eax #return original value
    mov eax, [ebp-4] #var i
    push eax #arg 1
    lea eax, [str0] #string literal
    push eax #arg 0
    sub esp, 4 #align stack to 16 bytes
    call printf
    add esp, 4 #restore stack alignment
    add esp, 8 #pop args
    lea eax, [ebp-8] #var j
    push eax #save lvalue addr
    lea eax, [ebp-4] #var i
    inc dword ptr [eax] #increment
    mov eax, [eax] #return new value
    pop ebx #restore lvalue addr
    mov [ebx], eax #assign
    mov eax, [ebp-8] #var j
    push eax #arg 2
    mov eax, [ebp-4] #var i
    push eax #arg 1
    lea eax, [str1] #string literal
    push eax #arg 0
    sub esp, 4 #align stack to 16 bytes
    call printf
    add esp, 4 #restore stack alignment
    add esp, 12 #pop args
    lea eax, [ebp-8] #var j
    push eax #save lvalue addr
    lea eax, [ebp-4] #var i
    mov ebx, eax #save address
    mov eax, [ebx] #load original value
    push eax #save original value
    inc dword ptr [ebx] #increment
    pop eax #return original value
    pop ebx #restore lvalue addr
    mov [ebx], eax #assign
    mov eax, [ebp-8] #var j
    push eax #arg 2
    mov eax, [ebp-4] #var i
    push eax #arg 1
    lea eax, [str1] #string literal
    push eax #arg 0
    sub esp, 4 #align stack to 16 bytes
    call printf
    add esp, 4 #restore stack alignment
    add esp, 12 #pop args
    lea eax, [ebp-8] #var j
    push eax #save lvalue addr
    lea eax, [ebp-4] #var i
    dec dword ptr [eax] #decrement
    mov eax, [eax] #return new value
    pop ebx #restore lvalue addr
    mov [ebx], eax #assign
    mov eax, [ebp-8] #var j
    push eax #arg 2
    mov eax, [ebp-4] #var i
    push eax #arg 1
    lea eax, [str1] #string literal
    push eax #arg 0
    sub esp, 4 #align stack to 16 bytes
    call printf
    add esp, 4 #restore stack alignment
    add esp, 12 #pop args
    lea eax, [ebp-8] #var j
    push eax #save lvalue addr
    lea eax, [ebp-4] #var i
    mov ebx, eax #save address
    mov eax, [ebx] #load original value
    push eax #save original value
    dec dword ptr [ebx] #decrement
    pop eax #return original value
    pop ebx #restore lvalue addr
    mov [ebx], eax #assign
    mov eax, [ebp-8] #var j
    push eax #arg 2
    mov eax, [ebp-4] #var i
    push eax #arg 1
    lea eax, [str2] #string literal
    push eax #arg 0
    sub esp, 4 #align stack to 16 bytes
    call printf
    add esp, 4 #restore stack alignment
    add esp, 12 #pop args
    mov esp, ebp
    pop ebp
    ret
