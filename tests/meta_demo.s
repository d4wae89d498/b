.intel_syntax noprefix
.section .note.GNU-stack,"",@progbits
.text
# Meta construct: 
    extern printf;
    
    main() {
        printf("Helloooooo\n");
        return 0;
    }
=== Meta Construct Evaluation ===
B Language Content: 
    extern printf;
    
    main() {
        printf("Helloooooo\n");
        return 0;
    }
Parsed AST: Program
  Extern: printf
  Function: main
    Block
      Statement
        Call: printf
          String: "Helloooooo
"
      Return
        Num: 0
Resolved printf to 0xf7cfe120
Parsed instruction:     push ebp -> opcode=0x55, modrm=0x00, size=1
Parsed instruction:     mov ebp, esp -> opcode=0x89, modrm=0xEC, size=2
Parsed instruction:     sub esp, 0 #locals -> opcode=0x83, modrm=0xEC, size=3
Parsed instruction:     lea eax, [str0] #string literal -> opcode=0x8D, modrm=0x05, size=6
Parsed instruction:     push eax #arg 0 -> opcode=0x50, modrm=0x00, size=1
Parsed instruction:     sub esp, 4 #align stack to 16 bytes -> opcode=0x83, modrm=0xEC, size=3
Parsed instruction:     call printf -> opcode=0xE8, modrm=0x00, size=5
Parsed instruction:     add esp, 4 #restore stack alignment -> opcode=0x83, modrm=0xC4, size=3
Parsed instruction:     add esp, 4 #pop args -> opcode=0x83, modrm=0xC4, size=3
Parsed instruction:     mov eax, 0 -> opcode=0x00, modrm=0x00, size=0
Parsed instruction:     mov esp, ebp -> opcode=0x89, modrm=0xE5, size=2
Parsed instruction:     pop ebp -> opcode=0x5D, modrm=0x00, size=1
Parsed instruction:     ret -> opcode=0xC3, modrm=0x00, size=1
Placed data at 0xf7ee7000: 'Helloooooo\n'
Relocation 0: str0 at offset 7, symbol_addr=0xf7ee7000
  Internal symbol str0: offset=7, target=0xf7ee7000, reloc_value=0xf7ee7000
Relocation 1: printf at offset 16, symbol_addr=0xf7cfe120
  External call to printf: offset=16, target=0xf7cfe120, reloc_value=0xffe1810c
Found main function at 0xf7ee6000
Code size: 31 bytes
Data size: 13 bytes
Main function code: 55 89 EC 83 EC 8D 05 00 70 EE F7 50 83 EC 04 E8 
Successfully generated machine code from assembly!
Machine code: 55 89 EC 83 EC 8D 05 00 70 EE F7 50 83 EC 04 E8 0C 81 E1 FF 83 C4 04 83 C4 04 00 89 E5 5D C3 

Calling printf through symbol resolution: Hello from generated machine code!
Program successfully parsed, assembled, and demonstrated symbol resolution!
=== Meta Construct Evaluation Complete ===

.globl main
main:
    push ebp
    mov ebp, esp
    sub esp, 0 #locals
    mov eax, 0
    mov esp, ebp
    pop ebp
    ret
