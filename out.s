.data
g: .long 42
h: .long 0
.data
str0: .asciz "hello"
.text
.globl foo
foo:
    push ebp
    mov ebp, esp
    sub esp, 0 ; locals
    mov eax, [ebp+8] ; var a
    push eax
    mov eax, [ebp+12] ; var b
    mov ebx, eax
    pop eax
    add eax, ebx
    mov esp, ebp
    pop ebp
    ret
    mov esp, ebp
    pop ebp
    ret
.globl main
main:
    push ebp
    mov ebp, esp
    sub esp, 16 ; locals
    mov eax, 41
    push eax ; arg 0
    mov eax, 1
    push eax
    mov eax, 5
    mov ebx, eax
    pop eax
    add eax, ebx
    call eax ; indirect call
    add esp, 4 ; pop args
    lea eax, [ebp-4] ; var k
    push eax ; save lvalue addr
    lea eax, [str0] ; string literal
    pop ebx ; restore lvalue addr
    mov [ebx], eax ; assign
    lea eax, [ebp-8] ; var x
    push eax ; save lvalue addr
    mov eax, 1
    push eax
    mov eax, 2
    mov ebx, eax
    pop eax
    add eax, ebx
    pop ebx ; restore lvalue addr
    mov [ebx], eax ; assign
    lea eax, [g] ; global g
    push eax ; save lvalue addr
    mov eax, [g] ; global g
    push eax
    mov eax, 1
    mov ebx, eax
    pop eax
    add eax, ebx
    pop ebx ; restore lvalue addr
    mov [ebx], eax ; assign
    lea eax, [ebp-8] ; var x
    push eax ; save lvalue addr
    mov eax, [h] ; global h
    pop ebx ; restore lvalue addr
    mov [ebx], eax ; assign
    lea eax, [ebp-12] ; var y
    push eax ; save lvalue addr
    mov eax, [ebp-8] ; var x
    push eax ; base
    mov eax, 3
    pop ebx ; base
    lea eax, [ebx+eax*4] ; array index
    mov eax, [eax] ; load array element
    push eax
    mov eax, [ebp-8] ; var x
    push eax ; base
    mov eax, 1
    push eax
    mov eax, 1
    mov ebx, eax
    pop eax
    add eax, ebx
    pop ebx ; base
    lea eax, [ebx+eax*4] ; array index
    mov eax, [eax] ; load array element
    push eax ; arg 0
    call bar
    add esp, 4 ; pop args
    mov ebx, eax
    pop eax
    add eax, ebx
    pop ebx ; restore lvalue addr
    mov [ebx], eax ; assign
    mov eax, [ebp-8] ; var x
    push eax
    mov eax, 2
    mov ebx, eax
    pop eax
    cmp eax, ebx
    setg al
    movzx eax, al ; relational result
    test eax, eax
    jz .L0
    lea eax, [ebp-8] ; var x
    push eax ; save lvalue addr
    mov eax, [ebp-8] ; var x
    push eax
    mov eax, 1
    mov ebx, eax
    pop eax
    sub eax, ebx
    pop ebx ; restore lvalue addr
    mov [ebx], eax ; assign
    jmp .L1
.L0:
.L1:
    mov eax, [ebp-8] ; var x
    push eax
    mov eax, 2
    mov ebx, eax
    pop eax
    add eax, ebx
    push eax ; base
    mov eax, 0
    pop ebx ; base
    lea eax, [ebx+eax*4] ; array index
    mov eax, [eax] ; load array element
    lea eax, [ebp-12] ; var y
    push eax ; save lvalue addr
    mov eax, [ebp-8] ; var x
    mov eax, [eax] ; deref
    pop ebx ; restore lvalue addr
    mov [ebx], eax ; assign
    lea eax, [ebp-12] ; var y
    push eax ; save lvalue addr
    lea eax, [ebp-8] ; var x
    pop ebx ; restore lvalue addr
    mov [ebx], eax ; assign
    lea eax, [ebp-12] ; var y
    push eax ; save lvalue addr
    mov eax, [ebp-8] ; var x
    push eax ; arg 0
    call foo
    add esp, 4 ; pop args
    mov eax, [eax] ; deref
    pop ebx ; restore lvalue addr
    mov [ebx], eax ; assign
    lea eax, [ebp-12] ; var y
    push eax ; save lvalue addr
    mov eax, [ebp-8] ; var x
    push eax ; base
    mov eax, 1
    pop ebx ; base
    lea eax, [ebx+eax*4] ; array index
    pop ebx ; restore lvalue addr
    mov [ebx], eax ; assign
    lea eax, [ebp-16] ; var i
    push eax ; save lvalue addr
    mov eax, 0
    pop ebx ; restore lvalue addr
    mov [ebx], eax ; assign
.L2:
    mov eax, [ebp-16] ; var i
    push eax
    mov eax, 10
    mov ebx, eax
    pop eax
    cmp eax, ebx
    setl al
    movzx eax, al ; relational result
    test eax, eax
    jz .L3
    mov eax, [ebp-16] ; var i
    push eax
    mov eax, 5
    mov ebx, eax
    pop eax
    cmp eax, ebx
    sete al
    movzx eax, al ; relational result
    test eax, eax
    jz .L4
    jmp .L3 ; break
    jmp .L5
.L4:
.L5:
    mov eax, [ebp-16] ; var i
    push eax
    mov eax, 2
    mov ebx, eax
    pop eax
    cmp eax, ebx
    sete al
    movzx eax, al ; relational result
    test eax, eax
    jz .L6
    lea eax, [ebp-16] ; var i
    push eax ; save lvalue addr
    mov eax, [ebp-16] ; var i
    push eax
    mov eax, 1
    mov ebx, eax
    pop eax
    add eax, ebx
    pop ebx ; restore lvalue addr
    mov [ebx], eax ; assign
    jmp .L2 ; continue
    jmp .L7
.L6:
.L7:
    lea eax, [ebp-16] ; var i
    push eax ; save lvalue addr
    mov eax, [ebp-16] ; var i
    push eax
    mov eax, 1
    mov ebx, eax
    pop eax
    add eax, ebx
    pop ebx ; restore lvalue addr
    mov [ebx], eax ; assign
    jmp .L2
.L3:
    lea eax, [ebp-16] ; var i
    push eax ; save lvalue addr
    mov eax, 0
    pop ebx ; restore lvalue addr
    mov [ebx], eax ; assign
.L_loop:
    lea eax, [ebp-16] ; var i
    push eax ; save lvalue addr
    mov eax, [ebp-16] ; var i
    push eax
    mov eax, 1
    mov ebx, eax
    pop eax
    add eax, ebx
    pop ebx ; restore lvalue addr
    mov [ebx], eax ; assign
    mov eax, [ebp-16] ; var i
    push eax
    mov eax, 5
    mov ebx, eax
    pop eax
    cmp eax, ebx
    setl al
    movzx eax, al ; relational result
    test eax, eax
    jz .L8
    jmp .L_loop ; goto
    jmp .L9
.L8:
.L9:
    mov eax, [ebp-16] ; var i
    mov esp, ebp
    pop ebp
    ret
    mov esp, ebp
    pop ebp
    ret
