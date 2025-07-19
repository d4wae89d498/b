#include <stdio.h>
#include "../b.h"
#include <string.h>

#define ASMEND "#"

static void gen_lvalue(ASTNode *expr, FILE *out);
static void gen_expr(ASTNode *expr, FILE *out);
static void gen_stmt(ASTNode *stmt, FILE *out);
static int label_count = 0;
static int is_global(const char *name);
#define MAX_LOCALS 64

typedef struct { const char *name; int offset; } Local;

static Local locals[MAX_LOCALS];
static int num_locals = 0;
static int stack_offset = 0;

// Add parameters to params table with positive offsets
#define MAX_PARAMS 32
static Local params[MAX_PARAMS];
static int num_params = 0;

static void add_params(ASTNodeList *paramlist) {
    int offset = 8; // [ebp+8] is first param in cdecl
    num_params = 0;
    for (ASTNodeList *l = paramlist; l; l = l->next) {
        if (num_params < MAX_PARAMS) {
            params[num_params].name = l->node->data.var.name;
            params[num_params].offset = offset;
            num_params++;
            offset += 4;
        }
    }
}

// Add locals to locals table with negative offsets
static void collect_locals(ASTNode *node) {
    if (!node) return;
    switch (node->type) {
        case AST_BLOCK:
            for (ASTNodeList *l = node->data.block.statements; l; l = l->next)
                collect_locals(l->node);
            break;
        case AST_ASSIGN:
            if (node->data.assign.var && node->data.assign.var->type == AST_VAR) {
                // Only add if not already present in params, locals, or globals
                const char *name = node->data.assign.var->data.var.name;
                int found = 0;
                for (int i = 0; i < num_params; ++i) {
                    if (strcmp(params[i].name, name) == 0) { found = 1; break; }
                }
                for (int i = 0; i < num_locals; ++i) {
                    if (strcmp(locals[i].name, name) == 0) { found = 1; break; }
                }
                if (is_global(name)) found = 1;
                if (!found && num_locals < MAX_LOCALS) {
                    locals[num_locals].name = name;
                    locals[num_locals].offset = 0; // will be set later
                    num_locals++;
                }
            }
            collect_locals(node->data.assign.expr);
            break;
        case AST_IF:
            collect_locals(node->data.if_stmt.cond);
            collect_locals(node->data.if_stmt.then_branch);
            collect_locals(node->data.if_stmt.else_branch);
            break;
        case AST_WHILE:
            collect_locals(node->data.while_stmt.cond);
            collect_locals(node->data.while_stmt.body);
            break;
        case AST_RETURN:
            collect_locals(node->data.ret.expr);
            break;
        default:
            break;
    }
}

static void assign_local_offsets() {
    stack_offset = 0;
    for (int i = 0; i < num_locals; ++i) {
        stack_offset -= 4;
        locals[i].offset = stack_offset;
    }
}

// Find variable offset: check params first, then locals
static int find_var_offset(const char *name) {
    for (int i = 0; i < num_params; ++i) {
        if (strcmp(params[i].name, name) == 0)
            return params[i].offset;
    }
    for (int i = 0; i < num_locals; ++i) {
        if (strcmp(locals[i].name, name) == 0)
            return locals[i].offset;
    }
    if (is_global(name))
        return 0x7fffffff; // special marker for global
    return 0; // not found
}

#define MAX_GLOBALS 128
static const char *global_names[MAX_GLOBALS];
static int global_inits[MAX_GLOBALS]; // 0 if uninitialized, else value
static int num_globals = 0;

static int is_global(const char *name) {
    for (int i = 0; i < num_globals; ++i)
        if (strcmp(global_names[i], name) == 0) return 1;
    return 0;
}

// Forward declaration for function name check
extern int is_function_name(const char *name, struct ASTNodeList *funcs);
extern struct ASTNodeList *top_level_funcs;

static void gen_lvalue(ASTNode *expr, FILE *out) {
    if (!expr) return;
    switch (expr->type) {
        case AST_VAR: {
            int off = find_var_offset(expr->data.var.name);
            if (off == 0x7fffffff) {
                fprintf(out, "    lea eax, [%s] " ASMEND "global %s\n", expr->data.var.name, expr->data.var.name);
            } else {
                fprintf(out, "    lea eax, [ebp%+d] " ASMEND "var %s\n", off, expr->data.var.name);
            }
            break;
        }
        case AST_INDEX: {
            gen_expr(expr->data.index.array, out); // base address in eax
            fprintf(out, "    push eax " ASMEND "base\n");
            gen_expr(expr->data.index.index, out); // index in eax
            fprintf(out, "    pop ebx " ASMEND "base\n");
            fprintf(out, "    lea eax, [ebx+eax*4] " ASMEND "array index\n");
            break;
        }
        case AST_UNOP: // address-of
            if (strcmp(expr->data.unop.op, "&") == 0) {
                gen_lvalue(expr->data.unop.expr, out);
            } else if (strcmp(expr->data.unop.op, "*") == 0) {
                gen_expr(expr->data.unop.expr, out);
                // eax now points to the address, so just pass through
            }
            break;
        default:
            // TODO: handle more lvalues
            break;
    }
}

// String literal table
#define MAX_STRINGS 128
static const char *string_literals[MAX_STRINGS];
static int num_strings = 0;

// Return label for string literal, adding to table if new
static const char *get_string_label(const char *value) {
    for (int i = 0; i < num_strings; ++i) {
        if (strcmp(string_literals[i], value) == 0)
            { static char buf[32]; snprintf(buf, sizeof(buf), "str%d", i); return buf; }
    }
    if (num_strings < MAX_STRINGS) {
        string_literals[num_strings] = value;
        static char buf[32];
        snprintf(buf, sizeof(buf), "str%d", num_strings);
        num_strings++;
        return buf;
    }
    return "str_overflow";
}

// Emit string literals in .data
static void emit_string_literals(FILE *out) {
    if (num_strings == 0) return;
    fprintf(out, ".data\n");
    for (int i = 0; i < num_strings; ++i) {
        fprintf(out, "str%d: .asciz \"", i);
        const char *s = string_literals[i];
        for (const char *p = s; *p; ++p) {
            if (*p == '\\' || *p == '"') fprintf(out, "\\%c", *p);
            else if (*p == '\n') fprintf(out, "\\n");
            else if (*p == '\t') fprintf(out, "\\t");
            else if ((unsigned char)*p < 32 || (unsigned char)*p > 126) fprintf(out, "\\%03o", (unsigned char)*p);
            else fputc(*p, out);
        }
        fprintf(out, "\"\n");
    }
}

static void gen_expr(ASTNode *expr, FILE *out) {
    if (!expr) return;
    switch (expr->type) {
        case AST_NUM:
            fprintf(out, "    mov eax, %d\n", expr->data.num.value);
            break;
        case AST_VAR: {
            if (is_function_name(expr->data.var.name, top_level_funcs)) {
                fprintf(out, "    lea eax, [%s] " ASMEND "function pointer\n", expr->data.var.name);
            } else {
                int off = find_var_offset(expr->data.var.name);
                if (off == 0x7fffffff) {
                    fprintf(out, "    mov eax, [%s] " ASMEND "global %s\n", expr->data.var.name, expr->data.var.name);
                } else {
                    fprintf(out, "    mov eax, [ebp%+d] " ASMEND "var %s\n", off, expr->data.var.name);
                }
            }
            break;
        }
        case AST_INDEX: {
            gen_lvalue(expr, out);
            fprintf(out, "    mov eax, [eax] " ASMEND "load array element\n");
            break;
        }
        case AST_UNOP:
            if (strcmp(expr->data.unop.op, "!") == 0) {
                gen_expr(expr->data.unop.expr, out);
                fprintf(out, "    cmp eax, 0\n");
                fprintf(out, "    sete al\n");
                fprintf(out, "    movzx eax, al " ASMEND "logical not\n");
            } else if (strcmp(expr->data.unop.op, "*") == 0) {
                gen_expr(expr->data.unop.expr, out);
                fprintf(out, "    mov eax, [eax] " ASMEND "deref\n");
            } else if (strcmp(expr->data.unop.op, "&") == 0) {
                gen_lvalue(expr->data.unop.expr, out);
            } else {
                // TODO: handle other unary ops
            }
            break;
        case AST_BINOP:
            if (strcmp(expr->data.binop.op, "+") == 0 || strcmp(expr->data.binop.op, "-") == 0 ||
                strcmp(expr->data.binop.op, "*") == 0 || strcmp(expr->data.binop.op, "/") == 0) {
                gen_expr(expr->data.binop.left, out);
                fprintf(out, "    push eax\n");
                gen_expr(expr->data.binop.right, out);
                fprintf(out, "    mov ebx, eax\n");
                fprintf(out, "    pop eax\n");
                if (strcmp(expr->data.binop.op, "+") == 0) {
                    fprintf(out, "    add eax, ebx\n");
                } else if (strcmp(expr->data.binop.op, "-") == 0) {
                    fprintf(out, "    sub eax, ebx\n");
                } else if (strcmp(expr->data.binop.op, "*") == 0) {
                    fprintf(out, "    imul eax, ebx\n");
                } else if (strcmp(expr->data.binop.op, "/") == 0) {
                    fprintf(out, "    cdq\n");
                    fprintf(out, "    idiv ebx\n");
                }
            } else if (
                strcmp(expr->data.binop.op, "==") == 0 || strcmp(expr->data.binop.op, "!=") == 0 ||
                strcmp(expr->data.binop.op, "<") == 0 || strcmp(expr->data.binop.op, ">") == 0 ||
                strcmp(expr->data.binop.op, "<=") == 0 || strcmp(expr->data.binop.op, ">=") == 0) {
                gen_expr(expr->data.binop.left, out);
                fprintf(out, "    push eax\n");
                gen_expr(expr->data.binop.right, out);
                fprintf(out, "    mov ebx, eax\n");
                fprintf(out, "    pop eax\n");
                fprintf(out, "    cmp eax, ebx\n");
                if (strcmp(expr->data.binop.op, "==") == 0) {
                    fprintf(out, "    sete al\n");
                } else if (strcmp(expr->data.binop.op, "!=") == 0) {
                    fprintf(out, "    setne al\n");
                } else if (strcmp(expr->data.binop.op, "<") == 0) {
                    fprintf(out, "    setl al\n");
                } else if (strcmp(expr->data.binop.op, ">") == 0) {
                    fprintf(out, "    setg al\n");
                } else if (strcmp(expr->data.binop.op, "<=") == 0) {
                    fprintf(out, "    setle al\n");
                } else if (strcmp(expr->data.binop.op, ">=") == 0) {
                    fprintf(out, "    setge al\n");
                }
                fprintf(out, "    movzx eax, al " ASMEND "relational result\n");
            } else if (strcmp(expr->data.binop.op, "&&") == 0) {
                int l_false = label_count++;
                int l_end = label_count++;
                gen_expr(expr->data.binop.left, out);
                fprintf(out, "    test eax, eax\n");
                fprintf(out, "    jz .L%d\n", l_false);
                gen_expr(expr->data.binop.right, out);
                fprintf(out, "    test eax, eax\n");
                fprintf(out, "    jz .L%d\n", l_false);
                fprintf(out, "    mov eax, 1\n");
                fprintf(out, "    jmp .L%d\n", l_end);
                fprintf(out, ".L%d:\n", l_false);
                fprintf(out, "    mov eax, 0\n");
                fprintf(out, ".L%d:\n", l_end);
            } else if (strcmp(expr->data.binop.op, "||") == 0) {
                int l_true = label_count++;
                int l_end = label_count++;
                gen_expr(expr->data.binop.left, out);
                fprintf(out, "    test eax, eax\n");
                fprintf(out, "    jnz .L%d\n", l_true);
                gen_expr(expr->data.binop.right, out);
                fprintf(out, "    test eax, eax\n");
                fprintf(out, "    jnz .L%d\n", l_true);
                fprintf(out, "    mov eax, 0\n");
                fprintf(out, "    jmp .L%d\n", l_end);
                fprintf(out, ".L%d:\n", l_true);
                fprintf(out, "    mov eax, 1\n");
                fprintf(out, ".L%d:\n", l_end);
            } else {
                // TODO: handle other binary ops
            }
            break;
        case AST_CHAR:
            fprintf(out, "    mov eax, %d " ASMEND "char literal\n", (unsigned char)expr->data.char_lit.value);
            break;
        case AST_STRING: {
            const char *label = get_string_label(expr->data.string_lit.value);
            fprintf(out, "    lea eax, [%s] " ASMEND "string literal\n", label);
            break;
        }
        case AST_CALL: {
            int argc = 0;
            for (ASTNodeList *l = expr->data.call.args; l; l = l->next) argc++;
            ASTNodeList *args[64];
            int i = 0;
            for (ASTNodeList *l = expr->data.call.args; l; l = l->next) args[i++] = l;
            for (int j = argc-1; j >= 0; --j) {
                gen_expr(args[j]->node, out);
                fprintf(out, "    push eax " ASMEND "arg %d\n", j);
            }
            if (expr->data.call.name) {
                fprintf(out, "    call %s\n", expr->data.call.name);
            } else if (expr->data.call.left) {
                gen_expr(expr->data.call.left, out);
                fprintf(out, "    call eax " ASMEND "indirect call\n");
            } else {
                fprintf(out, "; invalid call node\n");
            }
            if (argc > 0)
                fprintf(out, "    add esp, %d " ASMEND "pop args\n", argc*4);
            break;
        }
        default:
            // TODO: handle more expressions
            break;
    }
}

// Loop label stack for break/continue
#define MAX_LOOP_DEPTH 16
static int break_labels[MAX_LOOP_DEPTH];
static int continue_labels[MAX_LOOP_DEPTH];
static int loop_depth = 0;

static void gen_stmt(ASTNode *stmt, FILE *out) {
    if (!stmt) return;
    switch (stmt->type) {
        case AST_BLOCK:
            for (ASTNodeList *l = stmt->data.block.statements; l; l = l->next)
                gen_stmt(l->node, out);
            break;
        case AST_ASSIGN:
            gen_lvalue(stmt->data.assign.var, out);
            fprintf(out, "    push eax " ASMEND "save lvalue addr\n");
            gen_expr(stmt->data.assign.expr, out);
            fprintf(out, "    pop ebx " ASMEND "restore lvalue addr\n");
            fprintf(out, "    mov [ebx], eax " ASMEND "assign\n");
            break;
        case AST_IF: {
            int l_else = label_count++;
            int l_end = label_count++;
            gen_expr(stmt->data.if_stmt.cond, out);
            fprintf(out, "    test eax, eax\n");
            fprintf(out, "    jz .L%d\n", l_else);
            gen_stmt(stmt->data.if_stmt.then_branch, out);
            fprintf(out, "    jmp .L%d\n", l_end);
            fprintf(out, ".L%d:\n", l_else);
            if (stmt->data.if_stmt.else_branch)
                gen_stmt(stmt->data.if_stmt.else_branch, out);
            fprintf(out, ".L%d:\n", l_end);
            break;
        }
        case AST_WHILE: {
            int l_cond = label_count++;
            int l_end = label_count++;
            // Push loop labels
            break_labels[loop_depth] = l_end;
            continue_labels[loop_depth] = l_cond;
            loop_depth++;
            fprintf(out, ".L%d:\n", l_cond);
            gen_expr(stmt->data.while_stmt.cond, out);
            fprintf(out, "    test eax, eax\n");
            fprintf(out, "    jz .L%d\n", l_end);
            gen_stmt(stmt->data.while_stmt.body, out);
            fprintf(out, "    jmp .L%d\n", l_cond);
            fprintf(out, ".L%d:\n", l_end);
            // Pop loop labels
            loop_depth--;
            break;
        }
        case AST_FOR: {
            int l_cond = label_count++;
            int l_step = label_count++;
            int l_end = label_count++;
            // Push loop labels
            break_labels[loop_depth] = l_end;
            continue_labels[loop_depth] = l_step;
            loop_depth++;
            // Init
            if (stmt->data.for_stmt.init)
                gen_stmt(stmt->data.for_stmt.init, out);
            fprintf(out, ".L%d:\n", l_cond);
            // Cond
            if (stmt->data.for_stmt.cond) {
                gen_expr(stmt->data.for_stmt.cond, out);
                fprintf(out, "    test eax, eax\n");
                fprintf(out, "    jz .L%d\n", l_end);
            }
            // Body
            if (stmt->data.for_stmt.body)
                gen_stmt(stmt->data.for_stmt.body, out);
            fprintf(out, ".L%d:\n", l_step);
            // Step
            if (stmt->data.for_stmt.step)
                gen_stmt(stmt->data.for_stmt.step, out);
            fprintf(out, "    jmp .L%d\n", l_cond);
            fprintf(out, ".L%d:\n", l_end);
            // Pop loop labels
            loop_depth--;
            break;
        }
        case AST_BREAK:
            if (loop_depth > 0) {
                fprintf(out, "    jmp .L%d " ASMEND "break\n", break_labels[loop_depth-1]);
            } else {
                fprintf(out, "; break outside loop (ignored)\n");
            }
            break;
        case AST_CONTINUE:
            if (loop_depth > 0) {
                fprintf(out, "    jmp .L%d " ASMEND "continue\n", continue_labels[loop_depth-1]);
            } else {
                fprintf(out, "; continue outside loop (ignored)\n");
            }
            break;
        case AST_RETURN:
            if (stmt->data.ret.expr) {
                gen_expr(stmt->data.ret.expr, out);
                fprintf(out, "    mov esp, ebp\n");
                fprintf(out, "    pop ebp\n");
                fprintf(out, "    ret\n");
            }
            break;
        case AST_LABEL:
            fprintf(out, ".L_%s:\n", stmt->data.label.label);
            break;
        case AST_GOTO:
            fprintf(out, "    jmp .L_%s " ASMEND "goto\n", stmt->data.go.label);
            break;
        case AST_STATEMENT:
            gen_expr(stmt->data.statement.stmt, out);
            break;
        default:
            // TODO: handle more statements
            break;
    }
}

static void gen_function(ASTNode *fn, FILE *out) {
    // Reset locals and params for each function
    num_locals = 0;
    num_params = 0;
    stack_offset = 0;
    // Add parameters first
    add_params(fn->data.function.params);
    // Collect locals
    if (fn->data.function.body)
        collect_locals(fn->data.function.body);
    assign_local_offsets();
    fprintf(out, ".globl %s\n", fn->data.function.name);
    fprintf(out, "%s:\n", fn->data.function.name);
    // Prologue
    fprintf(out, "    push ebp\n");
    fprintf(out, "    mov ebp, esp\n");
    fprintf(out, "    sub esp, %d " ASMEND "locals\n", -stack_offset);
    // Body
    if (fn->data.function.body)
        gen_stmt(fn->data.function.body, out);
    // Epilogue (if not already returned)
    fprintf(out, "    mov esp, ebp\n");
    fprintf(out, "    pop ebp\n");
    fprintf(out, "    ret\n");
}

// Helper: collect global variables from AST
static void collect_globals(ASTNode *ast) {
    if (!ast) return;
    if (ast->type == AST_PROGRAM) {
        for (ASTNodeList *l = ast->data.program.functions; l; l = l->next)
            collect_globals(l->node);
    } else if (ast->type == AST_GLOBAL) {
        if (num_globals < MAX_GLOBALS) {
            global_names[num_globals] = ast->data.global.name;
            if (ast->data.global.init && ast->data.global.init->type == AST_NUM)
                global_inits[num_globals] = ast->data.global.init->data.num.value;
            else
                global_inits[num_globals] = 0;
            num_globals++;
        }
    }
}

static void collect_strings(ASTNode *n) {
    if (!n) return;
    if (n->type == AST_STRING) get_string_label(n->data.string_lit.value);
    #define RECURSE(x) collect_strings(x)
    switch (n->type) {
        case AST_PROGRAM:
            for (ASTNodeList *l = n->data.program.functions; l; l = l->next) RECURSE(l->node); break;
        case AST_FUNCTION:
            for (ASTNodeList *l = n->data.function.params; l; l = l->next) RECURSE(l->node);
            RECURSE(n->data.function.body); break;
        case AST_BLOCK:
            for (ASTNodeList *l = n->data.block.statements; l; l = l->next) RECURSE(l->node); break;
        case AST_STATEMENT:
            RECURSE(n->data.statement.stmt); break;
        case AST_IF:
            RECURSE(n->data.if_stmt.cond); RECURSE(n->data.if_stmt.then_branch); RECURSE(n->data.if_stmt.else_branch); break;
        case AST_WHILE:
            RECURSE(n->data.while_stmt.cond); RECURSE(n->data.while_stmt.body); break;
        case AST_FOR:
            RECURSE(n->data.for_stmt.init); RECURSE(n->data.for_stmt.cond); RECURSE(n->data.for_stmt.step); RECURSE(n->data.for_stmt.body); break;
        case AST_RETURN:
            RECURSE(n->data.ret.expr); break;
        case AST_ASSIGN:
            RECURSE(n->data.assign.var); RECURSE(n->data.assign.expr); break;
        case AST_BINOP:
            RECURSE(n->data.binop.left); RECURSE(n->data.binop.right); break;
        case AST_UNOP:
            RECURSE(n->data.unop.expr); break;
        case AST_CALL:
            for (ASTNodeList *l = n->data.call.args; l; l = l->next) RECURSE(l->node);
            if (n->data.call.left) RECURSE(n->data.call.left); break;
        case AST_INDEX:
            RECURSE(n->data.index.array); RECURSE(n->data.index.index); break;
        case AST_GLOBAL:
            RECURSE(n->data.global.init); break;
        default: break;
    }
    #undef RECURSE
}

// Emit .data section for globals before functions
void generate_x86(ASTNode *ast, FILE *out) {
    num_globals = 0;
    num_strings = 0;
    collect_globals(ast);
    collect_strings(ast);
    // Emit .intel_syntax noprefix at the top
    fprintf(out, ".intel_syntax noprefix\n");
    if (num_globals > 0 || num_strings > 0) {
        fprintf(out, ".data\n");
        for (int i = 0; i < num_globals; ++i) {
            fprintf(out, "%s: .long %d\n", global_names[i], global_inits[i]);
        }
        emit_string_literals(out);
        fprintf(out, ".text\n");
    }
    if (!ast) return;
    if (ast->type == AST_PROGRAM) {
        for (ASTNodeList *l = ast->data.program.functions; l; l = l->next) {
            if (l->node->type == AST_FUNCTION)
                gen_function(l->node, out);
        }
    } else if (ast->type == AST_FUNCTION) {
        gen_function(ast, out);
    } else {
        fprintf(out, "; x86 code generation expects a program or function node\n");
    }
} 