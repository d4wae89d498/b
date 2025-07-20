#include <dlfcn.h>
#include "../../b.h"
#include <sys/mman.h>

#include <stdint.h>
#include <regex.h>

#include "./as.h"

void *execute_code(Assembler *assembler) {
    // Place data section immediately after code
    size_t code_size = assembler->text_offset;
    size_t data_size = assembler->data_size;
    size_t total_size = code_size + data_size;
    // Round up to next multiple of page size
    size_t page_size = sysconf(_SC_PAGESIZE);
    if (total_size == 0) total_size = page_size;
    if (total_size % page_size != 0) {
        total_size += page_size - (total_size % page_size);
    }
    void *exec_mem = mmap(NULL, total_size, PROT_READ | PROT_WRITE | PROT_EXEC, 
                         MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    
    if (exec_mem == MAP_FAILED) {
        perror("mmap failed");
        return NULL;
    }
    
    unsigned char new_code[] = {
        0x55, 
        0x89, 0xE5, 
      //  0X83, 0xEC, 0x00, 
      //  0x83, 0xEC, 0X0C,
        0X89, 0xEC, 
        0x5D, 
        0xC3, 0X00, 0X00};
    // Copy code and data
   // memcpy(exec_mem, new_code, 13);
    memcpy(exec_mem, assembler->code, code_size);
    memcpy((char*)exec_mem + code_size, assembler->data, data_size);

    // Debug: print all symbols before update
    fprintf(stderr, "[DEBUG] Symbols before address update:\n");
    for (int i = 0; i < assembler->num_symbols; i++) {
        fprintf(stderr, "  Symbol: %s at %p\n", assembler->symbols[i].name, assembler->symbols[i].address);
    }
    // Update symbol addresses to point to new location
    for (int i = 0; i < assembler->num_symbols; i++) {
        // If the address is an offset (from label), update to exec_mem + offset
        intptr_t offset = (intptr_t)assembler->symbols[i].address;
        if (offset >= 0 && offset < (intptr_t)code_size) {
            assembler->symbols[i].address = (char*)exec_mem + offset;
        } else if ((void*)assembler->symbols[i].address >= (void*)assembler->data &&
                   (void*)assembler->symbols[i].address < (void*)(assembler->data + assembler->data_size)) {
            // Data symbol
            size_t doffset = (char*)assembler->symbols[i].address - (char*)assembler->data;
            assembler->symbols[i].address = (char*)exec_mem + code_size + doffset;
        }
    }
    // Debug: print all symbols after update
    fprintf(stderr, "[DEBUG] Symbols after address update:\n");
    for (int i = 0; i < assembler->num_symbols; i++) {
        fprintf(stderr, "  Symbol: %s at %p\n", assembler->symbols[i].name, assembler->symbols[i].address);
    }
    
    // Print data section contents in exec_mem
    fprintf(stderr, "[DEBUG] Data section in exec_mem: ");
    for (size_t i = 0; i < assembler->data_size; i++) {
        fprintf(stderr, "%02X ", ((unsigned char*)exec_mem + code_size)[i]);
    }
    fprintf(stderr, "\n[DEBUG] Data section as string: '%s'\n", (char*)exec_mem + code_size);
    
    return exec_mem;
}

void evaluate_meta_construct(const char *content) {
    fprintf(stderr, "=== Meta Construct Evaluation ===\n");
    fprintf(stderr, "B Language Content: %s\n", content);
    
    // First, parse the B language content as a complete program
    Parser parser;
    parser_init(&parser, content);
    
    // Parse as a complete program (since meta content can contain functions, externs, etc.)
    ASTNode *program = parse_program(&parser);
    if (!program) {
        fprintf(stderr, "Failed to parse B language content in meta construct\n");
        return;
    }
    
    fprintf(stderr, "Parsed AST: ");
    //print_ast(program, 0);
    
    // Generate assembly from the parsed AST
    FILE *temp_file = tmpfile();
    if (!temp_file) {
        fprintf(stderr, "Failed to create temporary file\n");
        free_ast(program);
        return;
    }
    
    // Generate x86 assembly from the AST
    generate_x86(program, temp_file);
    fflush(temp_file);
    rewind(temp_file);
    
    // Get the file descriptor to create a filename
    int fd = fileno(temp_file);
    char temp_filename[64];
    snprintf(temp_filename, sizeof(temp_filename), "/proc/self/fd/%d", fd);
    
    Assembler assembler;
    assembler_init(&assembler);
    
    // Resolve external symbols using dlsym
    void *printf_addr = resolve_external_symbol("printf");
    if (printf_addr) {
        add_symbol(&assembler, "printf", printf_addr);
    } else {
        fprintf(stderr, "Failed to resolve printf symbol\n");
        assembler_cleanup(&assembler);
        fclose(temp_file);
        return;
    }
    
    // Parse assembly file using the existing working function
    if (parse_assembly_file(temp_filename, &assembler) != 0) {
        assembler_cleanup(&assembler);
        fclose(temp_file);
        return;
    }
    fprintf(stderr, "[DEBUG] num_symbols after parsing: %d\n", assembler.num_symbols);
    // Resolve external symbols (no symbol file needed for meta constructs)
    if (resolve_symbols(&assembler, NULL) != 0) {
        assembler_cleanup(&assembler);
        fclose(temp_file);
        return;
    }
    // Assemble instructions
    if (assemble_instructions(&assembler) != 0) {
        assembler_cleanup(&assembler);
        fclose(temp_file);
        return;
    }
    fprintf(stderr, "[DEBUG] num_symbols before execution: %d\n", assembler.num_symbols);
    // Execute the code
    void *exec_mem = execute_code(&assembler);
    if (!exec_mem) {
        assembler_cleanup(&assembler);
        fclose(temp_file);
        return;
    }
    
    // Find main function
    void *main_addr = find_symbol(&assembler, "main");
    if (main_addr) {
        fprintf(stderr, "Found main function at %p (from %p)\n", main_addr, exec_mem);
        fprintf(stderr, "Code size: %d bytes\n", assembler.text_offset);
        fprintf(stderr, "Data size: %d bytes\n", assembler.data_size);
        
        // Dump the first few bytes of the main function
        fprintf(stderr, "Main function code: ");
        unsigned char *code_ptr = (unsigned char*)main_addr;
        for (int i = 0; i < 16 && i < assembler.text_offset; i++) {
            fprintf(stderr, "%02X ", code_ptr[i]);
        }
        fprintf(stderr, "\n");
        
        // Demonstrate that we successfully generated machine code
        fprintf(stderr, "Successfully generated machine code from assembly!\n");
        fprintf(stderr, "Machine code: ");
        for (int i = 0; i < assembler.text_offset; i++) {
            fprintf(stderr, "%02X ", ((unsigned char*)main_addr)[i]);
        }
        fprintf(stderr, "\n");
        
        // Actually call the generated main function!
        fprintf(stderr, "\nCalling generated main function:-----\n");
        typedef int (*main_func_t)(void);
        main_func_t main_fn = (main_func_t)main_addr;
        int main_result = main_fn();
        fprintf(stderr, "======================\n");
        fprintf(stderr, "main() returned: %d\n", main_result);
        
        fprintf(stderr, "Program successfully parsed, assembled, and demonstrated symbol resolution!\n");
    } else {
        fprintf(stderr, "Error: main function not found\n");
        // Print all symbols for debugging
        for (int i = 0; i < assembler.num_symbols; i++) {
            fprintf(stderr, "Symbol: %s at %p\n", assembler.symbols[i].name, assembler.symbols[i].address);
        }
    }
    
    // Cleanup
    munmap(exec_mem, 4096);
    assembler_cleanup(&assembler);
    free_ast(program);
    fclose(temp_file);
    
    fprintf(stderr, "=== Meta Construct Evaluation Complete ===\n\n");
} 