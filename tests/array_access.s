.intel_syntax noprefix
.section .note.GNU-stack,"",@progbits
.text
.data
.data
str0: .asciz "hello"
str1: .asciz "%c %c %c %c %c"
.text
.globl main
main:
    push ebp
    mov ebp, esp
    sub esp, 4 #locals
#    str variable declaration
    lea eax, [ebp-4] #var str
    push eax #save lvalue addr
    lea eax, [str0] #string literal
    pop ebx #restore lvalue addr
    mov [ebx], eax #assign
    mov eax, [ebp-4] #var str
    push eax #base
    mov eax, 1
    pop ebx #base
    lea eax, [ebx+eax*4] #array index
    mov eax, [eax] #load array element
    push eax
    mov eax, 255
    mov ebx, eax
    pop eax
    and eax, ebx
    push eax #arg 5
    mov eax, [ebp-4] #var str
    push eax #base
    mov eax, 0
    pop ebx #base
    lea eax, [ebx+eax*4] #array index
    mov eax, [eax] #load array element
    push eax
    mov eax, 24
    mov ebx, eax
    pop eax
    mov cl, bl
    shr eax, cl
    push eax
    mov eax, 255
    mov ebx, eax
    pop eax
    and eax, ebx
    push eax #arg 4
    mov eax, [ebp-4] #var str
    push eax #base
    mov eax, 0
    pop ebx #base
    lea eax, [ebx+eax*4] #array index
    mov eax, [eax] #load array element
    push eax
    mov eax, 16
    mov ebx, eax
    pop eax
    mov cl, bl
    shr eax, cl
    push eax
    mov eax, 255
    mov ebx, eax
    pop eax
    and eax, ebx
    push eax #arg 3
    mov eax, [ebp-4] #var str
    push eax #base
    mov eax, 0
    pop ebx #base
    lea eax, [ebx+eax*4] #array index
    mov eax, [eax] #load array element
    push eax
    mov eax, 8
    mov ebx, eax
    pop eax
    mov cl, bl
    shr eax, cl
    push eax
    mov eax, 255
    mov ebx, eax
    pop eax
    and eax, ebx
    push eax #arg 2
    mov eax, [ebp-4] #var str
    push eax #base
    mov eax, 0
    pop ebx #base
    lea eax, [ebx+eax*4] #array index
    mov eax, [eax] #load array element
    push eax
    mov eax, 255
    mov ebx, eax
    pop eax
    and eax, ebx
    push eax #arg 1
    lea eax, [str1] #string literal
    push eax #arg 0
    call printf
    add esp, 24 #pop args
    mov esp, ebp
    pop ebp
    ret
