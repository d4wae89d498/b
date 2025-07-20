#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <ctype.h>
#include <errno.h>
#include <dlfcn.h>
#include "../../b.h"
#include <stdint.h>
#include <regex.h>

#define MAX_LINE 1024
#define MAX_SYMBOLS 256
#define MAX_INSTRUCTIONS 4096
#define MAX_DATA 4096

typedef struct {
    char name[64];
    void *address;
} Symbol;

typedef struct {
    char *name;
    int offset;
    int size;
} Relocation;

typedef struct {
    unsigned char *code;
    size_t code_size;
    size_t code_capacity;
    unsigned char *data;
    size_t data_size;
    size_t data_capacity;
    Symbol symbols[MAX_SYMBOLS];
    int num_symbols;
    Relocation relocations[MAX_INSTRUCTIONS];
    int num_relocations;
    int text_offset;
    int data_offset;
} Assembler;

// Function prototypes
void assembler_init(Assembler *assembler);
void assembler_cleanup(Assembler *assembler);
int parse_assembly_file(const char *filename, Assembler *assembler);
int resolve_symbols(Assembler *assembler, const char *symbol_file);
int assemble_instructions(Assembler *assembler);
void *execute_code(Assembler *assembler);
void add_symbol(Assembler *assembler, const char *name, void *address);
void *find_symbol(Assembler *assembler, const char *name);

// Forward declarations for B language parsing
typedef struct {
    const char *src;
    size_t pos;
    int line;
    int col;
    int cur;
} Parser;

void parser_init(Parser *p, const char *src);
ASTNode *parse_statement(Parser *p);
ASTNode *parse_program(Parser *p);
void print_ast(ASTNode *node, int indent);
void free_ast(ASTNode *node);
void generate_x86(ASTNode *ast, FILE *out);

// Simple x86 instruction encoding
typedef struct {
    unsigned char opcode;
    unsigned char modrm;
    unsigned char sib;
    unsigned char imm8;
    unsigned int imm32;
    int size;
} Instruction;

// Parse a single instruction line
int parse_instruction(const char *line, Instruction *inst, Assembler *assembler) {
    memset(inst, 0, sizeof(Instruction));
    
    // Skip leading whitespace
    while (isspace(*line)) line++;
    
    // Parse instruction
    if (strncmp(line, "push", 4) == 0) {
        if (strstr(line, "ebp")) {
            inst->opcode = 0x55; // push ebp
            inst->size = 1;
        } else if (strstr(line, "eax")) {
            inst->opcode = 0x50; // push eax
            inst->size = 1;
        }
        return inst->size > 0;
    } else if (strncmp(line, "pop", 3) == 0) {
        if (strstr(line, "ebp")) {
            inst->opcode = 0x5D; // pop ebp
            inst->size = 1;
        } else if (strstr(line, "ebx")) {
            inst->opcode = 0x5B; // pop ebx
            inst->size = 1;
        }
        return inst->size > 0;
    } else if (strncmp(line, "mov", 3) == 0) {
        if (strstr(line, "ebp, esp")) {
            inst->opcode = 0x89; // mov ebp, esp
            inst->modrm = 0xE5;
            inst->size = 2;
        } else if (strstr(line, "esp, ebp")) {
            inst->opcode = 0x89; // mov esp, ebp
            inst->modrm = 0xEC;
            inst->size = 2;
        } else if (strstr(line, "eax,")) {
            // mov eax, imm32
            const char *imm_start = strstr(line, "eax,");
            if (imm_start) {
                imm_start += 4;
                while (isspace(*imm_start) || *imm_start == ',') imm_start++;
                int imm = atoi(imm_start);
                inst->opcode = 0xB8; // mov eax, imm32
                inst->imm32 = imm;
                inst->size = 5;
            }
        } else if (strstr(line, "[ebx], eax")) {
            // mov [ebx], eax
            inst->opcode = 0x89;
            inst->modrm = 0x03; // mod=00, reg=000 (EAX), rm=011 (EBX)
            inst->size = 2;
        }
        return inst->size > 0;
    } else if (strncmp(line, "sub", 3) == 0) {
        int imm = 0;
        if (sscanf(line, "sub esp, %d", &imm) == 1) {
            inst->opcode = 0x83;
            inst->modrm = 0xEC;
            inst->imm8 = (unsigned char)imm;
            inst->size = 3;
        }
        return inst->size > 0;
    } else if (strncmp(line, "add", 3) == 0) {
        int imm = 0;
        if (sscanf(line, "add esp, %d", &imm) == 1) {
            inst->opcode = 0x83;
            inst->modrm = 0xC4;
            inst->imm8 = (unsigned char)imm;
            inst->size = 3;
        }
        return inst->size > 0;
    } else if (strncmp(line, "lea", 3) == 0) {
        // lea eax, [ebp-imm]
        int disp = 0;
        if (sscanf(line, "lea eax, [ebp%*[-]%d", &disp) == 1) {
            inst->opcode = 0x8D;
            inst->modrm = 0x45;
            inst->imm8 = (unsigned char)(-disp & 0xFF); // signed disp8
            inst->size = 3;
        } else {
            // lea eax, [symbol]
            char symbol[64];
            if (sscanf(line, "lea eax, [%63[^]]]", symbol) == 1) {
                inst->opcode = 0x8D;
                inst->modrm = 0x05;
                inst->size = 6; // 2 for opcode+modrm, 4 for imm32
                fprintf(stderr, "[DEBUG] [[%s]]\n", symbol);
                // todo: resolve symbol here
                // Try to resolve symbol now
                void *sym_addr = NULL;
                // Try to find symbol in assembler symbol table
                for (int i = 0; i < assembler->num_symbols; ++i) {
                    if (strcmp(assembler->symbols[i].name, symbol) == 0) {
                        sym_addr = assembler->symbols[i].address;
                        break;
                    }
                }
                
                if (sym_addr) {
                    inst->imm32 = (unsigned int)(uintptr_t)sym_addr;
                    fprintf(stderr, "[DEBUG] Resolved symbol '%s' to %p at parse time\n", symbol, sym_addr);
                } else {
                    fprintf(stderr, "[DEBUG] Symbol '%s' not found at parse time, will relocate\n", symbol);
                }
            }
        }
        return inst->size > 0;
    } else if (strncmp(line, "call", 4) == 0) {
        char symbol[64];
        if (sscanf(line, "call %63s", symbol) == 1) {
            inst->opcode = 0xE8;
            inst->size = 5;
            void *sym_addr = NULL;
                // Try to find symbol in assembler symbol table
                for (int i = 0; i < assembler->num_symbols; ++i) {
                    if (strcmp(assembler->symbols[i].name, symbol) == 0) {
                        sym_addr = assembler->symbols[i].address;
                        break;
                    }
                }
                
                if (sym_addr) {
                    inst->imm32 = (unsigned int)(uintptr_t)sym_addr;
                    fprintf(stderr, "[DEBUG] Resolved symbol '%s' to %p at parse time\n", symbol, sym_addr);
                } else {
                    fprintf(stderr, "[DEBUG] Symbol '%s' not found at parse time, will relocate\n", symbol);
                }
            return 1;
        }
        inst->opcode = 0xE8; // fallback
        inst->size = 5;
        return 1;
    } else if (strncmp(line, "ret", 3) == 0) {
        inst->opcode = 0xC3; // ret
        inst->size = 1;
        return 1;
    }
    
    return 0;
}

void assembler_init(Assembler *assembler) {
    memset(assembler, 0, sizeof(Assembler));
    assembler->code_capacity = 4096;
    assembler->data_capacity = 4096;
    assembler->code = (unsigned char*) malloc(assembler->code_capacity);
    assembler->data = (unsigned char*) malloc(assembler->data_capacity);
    assembler->text_offset = 0;
    assembler->data_offset = 0;
}

void assembler_cleanup(Assembler *assembler) {
    if (assembler->code) free(assembler->code);
    if (assembler->data) free(assembler->data);
}

void add_symbol(Assembler *assembler, const char *name, void *address) {
    if (assembler->num_symbols < MAX_SYMBOLS) {
        strncpy(assembler->symbols[assembler->num_symbols].name, name, 63);
        assembler->symbols[assembler->num_symbols].name[63] = '\0';
        assembler->symbols[assembler->num_symbols].address = address;
        fprintf(stderr, "[DEBUG] add_symbol: '%s' at %p (index %d)\n", name, address, assembler->num_symbols);
        assembler->num_symbols++;
    } else {
        fprintf(stderr, "[DEBUG] add_symbol: symbol table full, could not add '%s'\n", name);
    }
}

void *find_symbol(Assembler *assembler, const char *name) {
    for (int i = 0; i < assembler->num_symbols; i++) {
        if (strcmp(assembler->symbols[i].name, name) == 0) {
            return assembler->symbols[i].address;
        }
    }
    return NULL;
}

// Resolve external symbols using dlsym
void *resolve_external_symbol(const char *name) {
    void *handle = RTLD_DEFAULT;
    void *symbol = dlsym(handle, name);
    if (!symbol) {
        fprintf(stderr, "Warning: Could not resolve symbol %s: %s\n", name, dlerror());
        return NULL;
    }
    fprintf(stderr, "Resolved %s to %p\n", name, symbol);
    return symbol;
}

void print_ebx_addr(unsigned int ebx_val) {
    fprintf(stderr, "[DEBUG] About to write to [ebx]=0x%08X\n", ebx_val);
}

void print_esp_addr(unsigned int esp_val) {
    fprintf(stderr, "[DEBUG] At call, esp=0x%08X\n", esp_val);
}

int parse_assembly_file(const char *filename, Assembler *assembler) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Error: Cannot open file %s\n", filename);
        return -1;
    }
    
    char line[MAX_LINE];
    int line_num = 0;
    int in_text = 0;
    int in_data = 0;
    
    while (fgets(line, sizeof(line), file)) {
        line_num++;
        
        // Remove newline and carriage return
        line[strcspn(line, "\r\n")] = 0;
        
        // Debug print
        //printf("[DEBUG] Parsing line: '%s'\n", line);
        
        // Skip empty lines and comments
        if (line[0] == 0 || line[0] == '#') continue;
                // Check for section directives
        if (strstr(line, ".text")) {
            in_text = 1;
            in_data = 0;
            continue;
        }
        
        if (strstr(line, ".data")) {
            in_data = 1;
            in_text = 0;
            continue;
        }

          // Skip assembler directives
        if (line[0] == '.' || strncmp(line, ".globl", 6) == 0) continue;
        
        
        // Handle data section
        if (in_data) {
            char *colon = strchr(line, ':');
            if (colon) {
                *colon = 0;
                char *label = line;
                while (isspace(*label)) label++;
                
                char *directive = colon + 1;
                while (isspace(*directive)) directive++;
                
                if (strncmp(directive, ".asciz", 6) == 0) {
                    char *str_start = strchr(directive, '"');
                    if (str_start) {
                        str_start++;
                        char *str_end = strrchr(str_start, '"');
                        if (str_end) {
                            *str_end = 0;
                            int len = strlen(str_start) + 1;
                            
                            // Add to data section
                            if (assembler->data_size + len <= assembler->data_capacity) {
                                strcpy((char*)assembler->data + assembler->data_size, str_start);
                                assembler->data_size += len;
                                
                                // Add symbol for this label
                                add_symbol(assembler, label, assembler->data + assembler->data_offset);
                                assembler->data_offset += len;
                            }
                        }
                    }
                }
            }
            continue;
        }
        
        // Handle text section
        if (in_text) {
            char *colon = strchr(line, ':');
            if (colon) {
                *colon = 0;
                char *label = line;
                while (isspace(*label)) label++;
                // Trim trailing whitespace
                char *end = label + strlen(label) - 1;
                while (end > label && isspace(*end)) { *end = 0; end--; }
                if (label[0] != 0) {
                    add_symbol(assembler, label, (void*)(intptr_t)assembler->text_offset);
                } else {
                    fprintf(stderr, "[DEBUG] Label is empty after trim, skipping.\n");
                }
                continue;
            }
            
            // Parse instruction
            Instruction inst;
            if (parse_instruction(line, &inst, assembler)) {
                fprintf(stderr, "Parsed instruction: %s -> opcode=0x%02X, modrm=0x%02X, size=%d\n", 
                       line, inst.opcode, inst.modrm, inst.size);
                int start_offset = assembler->text_offset;
                // Only emit if size > 0
                if (inst.size > 0) {
                    assembler->code[assembler->text_offset++] = inst.opcode;
                    if (inst.size > 1 &&  inst.modrm != 0)
                         assembler->code[assembler->text_offset++] = inst.modrm;
                    // Handle lea eax, [symbol] (modrm==0x05, size==6)
                    if (inst.opcode == 0x8D && inst.modrm == 0x05 && inst.size == 6) {
                        char *symbol = (char*)&inst + sizeof(Instruction);
                        if (inst.imm32) {
                            // Symbol was resolved at parse time, emit address
                            unsigned int v = inst.imm32;
                            assembler->code[assembler->text_offset++] = v & 0xFF;
                            assembler->code[assembler->text_offset++] = (v >> 8) & 0xFF;
                            assembler->code[assembler->text_offset++] = (v >> 16) & 0xFF;
                            assembler->code[assembler->text_offset++] = (v >> 24) & 0xFF;
                            fprintf(stderr, "[DEBUG] Emitted resolved address for lea eax, [%s]: 0x%08X\n", symbol, v);
                        } else {
                            // Add relocation for symbol
                            assembler->relocations[assembler->num_relocations].name = strdup(symbol);
                            assembler->relocations[assembler->num_relocations].offset = assembler->text_offset;
                            assembler->relocations[assembler->num_relocations].size = 4;
                            assembler->num_relocations++;
                            assembler->code[assembler->text_offset++] = 0;
                            assembler->code[assembler->text_offset++] = 0;
                            assembler->code[assembler->text_offset++] = 0;
                            assembler->code[assembler->text_offset++] = 0;
                            fprintf(stderr, "[DEBUG] Placeholder for lea eax, [%s] at offset %d\n", symbol, assembler->text_offset-4);
                        }
                    } else if (inst.opcode == 0xE8 && inst.size == 5) { // call symbol
                        // Emit: mov eax, <absolute address>; call eax
                        unsigned int addr = inst.imm32;
                        assembler->text_offset -= 1;
                        assembler->code[assembler->text_offset++] = 0xB8; // mov eax, imm32
                        assembler->code[assembler->text_offset++] = addr & 0xFF;
                        assembler->code[assembler->text_offset++] = (addr >> 8) & 0xFF;
                        assembler->code[assembler->text_offset++] = (addr >> 16) & 0xFF;
                        assembler->code[assembler->text_offset++] = (addr >> 24) & 0xFF;
                        assembler->code[assembler->text_offset++] = 0xFF; // call eax
                        assembler->code[assembler->text_offset++] = 0xD0;
                        fprintf(stderr, "[DEBUG] Emitted mov eax, 0x%08X; call eax for call\n", addr);
                    
                    } else if (inst.opcode == 0xB8) { // mov eax, imm32
                        unsigned int v = inst.imm32;
                        assembler->code[assembler->text_offset++] = v & 0xFF;
                        assembler->code[assembler->text_offset++] = (v >> 8) & 0xFF;
                        assembler->code[assembler->text_offset++] = (v >> 16) & 0xFF;
                        assembler->code[assembler->text_offset++] = (v >> 24) & 0xFF;
                    } else if (inst.opcode == 0x8D && inst.modrm == 0x45) { // lea eax, [ebp-imm8]
                        assembler->code[assembler->text_offset++] = inst.imm8;
                    } else if (inst.size == 3) {
                        assembler->code[assembler->text_offset++] = inst.imm8;
                    }
                    // Debug print: dump emitted bytes for this instruction
                    fprintf(stderr, "[EMIT] ");
                    for (int i = start_offset; i < assembler->text_offset; ++i) {
                        fprintf(stderr, "%02X ", assembler->code[i]);
                    }
                    fprintf(stderr, "\n");
                }
            } else {
                fprintf(stderr, "Parse error at %s\n", line);
            }
        }
    }
    
    fclose(file);
    return 0;
}

int resolve_symbols(Assembler *assembler, const char *symbol_file) {
    if (!symbol_file) return 0;
    
    FILE *file = fopen(symbol_file, "r");
    if (!file) {
        fprintf(stderr, "Warning: Cannot open symbol file %s\n", symbol_file);
        return 0;
    }
    
    char line[MAX_LINE];
    while (fgets(line, sizeof(line), file)) {
        char *newline = strchr(line, '\n');
        if (newline) *newline = 0;
        
        char *space = strchr(line, ' ');
        if (space) {
            *space = 0;
            char *name = line;
            char *addr_str = space + 1;
            
            void *address = (void*)strtoul(addr_str, NULL, 16);
            add_symbol(assembler, name, address);
        }
    }
    
    fclose(file);
    return 0;
}



int assemble_instructions(Assembler *assembler) {
    // Resolve relocations
    for (int i = 0; i < assembler->num_relocations; i++) {
        Relocation *reloc = &assembler->relocations[i];
        void *symbol_addr = find_symbol(assembler, reloc->name);
        
        if (symbol_addr) {
            // Calculate relative address for function calls
            if (reloc->size == 4) {
                unsigned int *addr_ptr = (unsigned int*)(assembler->code + reloc->offset);
                *addr_ptr = (unsigned int)((char*)symbol_addr - ((char*)assembler->code + reloc->offset + 4));
            }
        } else {
            fprintf(stderr, "Warning: Undefined symbol %s\n", reloc->name);
        }
    }
    
    return 0;
}
