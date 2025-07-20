#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "b.h"
#include "targets/x86/b2as.c"

ASTNodeList *top_level_funcs = NULL;
// Add a global variable for the current filename
const char *current_filename = NULL;
// --- Utility Functions ---
ASTNode *make_node(ASTNodeType type);
ASTNodeList *append_node(ASTNodeList *list, ASTNode *node);
void free_ast(ASTNode *node);
void print_ast(ASTNode *node, int indent);

// --- Parser State ---
typedef struct {
    const char *src;
    size_t pos;
    int line;
    int col;
    int cur;
} Parser;

void parser_init(Parser *p, const char *src);
int parser_peek(Parser *p);
int parser_next(Parser *p);
void parser_skip_ws(Parser *p);
int parser_match(Parser *p, const char *kw);
void parser_error(Parser *p, const char *msg);

// --- Parser Functions (to be implemented) ---
ASTNode *parse_program(Parser *p);

// --- Main ---
int main(int argc, char **argv) {
    int dump_asm = 0;
    const char *filename = NULL;
    if (argc < 2) {
        fprintf(stderr, "Usage: %s [-S] <file.b>\n", argv[0]);
        return 1;
    }
    if (argc == 3 && strcmp(argv[1], "-S") == 0) {
        dump_asm = 1;
        filename = argv[2];
    } else if (argc == 2) {
        filename = argv[1];
    } else {
        fprintf(stderr, "Usage: %s [-S] <file.b>\n", argv[0]);
        return 1;
    }
    FILE *f = fopen(filename, "r");
    if (!f) {
        fprintf(stderr, "Could not open %s\n", filename);
        return 1;
    }
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *src = (char*)malloc(len + 1);
    if (!src) { fclose(f); fprintf(stderr, "Out of memory\n"); return 1; }
    fread(src, 1, len, f);
    src[len] = 0;
    fclose(f);
    Parser parser;
    parser_init(&parser, src);
    current_filename = filename;
    ASTNode *ast = parse_program(&parser);
    if (ast) {
        generate_x86(ast, stdout);
        if (!dump_asm) {
            print_ast(ast, 0);
        }
        free_ast(ast);
    }
    free(src);
    return 0;
}

// --- Implementations ---
ASTNode *make_node(ASTNodeType type) {
    ASTNode *n = (ASTNode*)calloc(1, sizeof(ASTNode));
    n->type = type;
    return n;
}
ASTNodeList *append_node(ASTNodeList *list, ASTNode *node) {
    ASTNodeList *item = (ASTNodeList*)calloc(1, sizeof(ASTNodeList));
    item->node = node;
    if (!list) return item;
    ASTNodeList *cur = list;
    while (cur->next) cur = cur->next;
    cur->next = item;
    return list;
}
void free_ast(ASTNode *node) {
    if (!node) return;
    switch (node->type) {
        case AST_PROGRAM:
            for (ASTNodeList *l = node->data.program.functions; l;) {
                ASTNodeList *n = l->next;
                free_ast(l->node); free(l); l = n;
            }
            break;
        case AST_FUNCTION:
            free(node->data.function.name);
            for (ASTNodeList *l = node->data.function.params; l;) {
                ASTNodeList *n = l->next;
                free_ast(l->node); free(l); l = n;
            }
            free_ast(node->data.function.body);
            break;
        case AST_BLOCK:
            for (ASTNodeList *l = node->data.block.statements; l;) {
                ASTNodeList *n = l->next;
                free_ast(l->node); free(l); l = n;
            }
            break;
        case AST_STATEMENT:
            free_ast(node->data.statement.stmt);
            break;
        case AST_IF:
            free_ast(node->data.if_stmt.cond);
            free_ast(node->data.if_stmt.then_branch);
            free_ast(node->data.if_stmt.else_branch);
            break;
        case AST_WHILE:
            free_ast(node->data.while_stmt.cond);
            free_ast(node->data.while_stmt.body);
            break;
        case AST_RETURN:
            free_ast(node->data.ret.expr);
            break;
        case AST_ASSIGN:
            free_ast(node->data.assign.var);
            free_ast(node->data.assign.expr);
            break;
        case AST_BINOP:
            free(node->data.binop.op);
            free_ast(node->data.binop.left);
            free_ast(node->data.binop.right);
            break;
        case AST_UNOP:
            free(node->data.unop.op);
            free_ast(node->data.unop.expr);
            break;
        case AST_CALL:
            free(node->data.call.name);
            for (ASTNodeList *l = node->data.call.args; l;) {
                ASTNodeList *n = l->next;
                free_ast(l->node); free(l); l = n;
            }
            free_ast(node->data.call.left);
            break;
        case AST_VAR:
            free(node->data.var.name);
            break;
        case AST_STRING:
            free(node->data.string_lit.value);
            break;
        case AST_EXTERN:
            free(node->data.ext.name);
            break;
        case AST_INDEX:
            free_ast(node->data.index.array);
            free_ast(node->data.index.index);
            break;
        case AST_GLOBAL:
            free(node->data.global.name);
            free_ast(node->data.global.init);
            break;
        case AST_LABEL:
            free(node->data.label.label);
            break;
        case AST_GOTO:
            free(node->data.go.label);
            break;
        case AST_META:
            free(node->data.meta.content);
            break;
        case AST_VAR_DECL:
            free(node->data.var_decl.name);
            break;
        default: break;
    }
    free(node);
}
void print_indent(int n) { while (n--) putchar(' '); }
void print_ast(ASTNode *node, int indent) {
    if (!node) return;
    print_indent(indent);
    switch (node->type) {
        case AST_PROGRAM:
            printf("Program\n");
            for (ASTNodeList *l = node->data.program.functions; l; l = l->next)
                print_ast(l->node, indent+2);
            break;
        case AST_FUNCTION:
            printf("Function: %s\n", node->data.function.name);
            print_ast(node->data.function.body, indent+2);
            break;
        case AST_BLOCK:
            printf("Block\n");
            for (ASTNodeList *l = node->data.block.statements; l; l = l->next)
                print_ast(l->node, indent+2);
            break;
        case AST_STATEMENT:
            printf("Statement\n");
            print_ast(node->data.statement.stmt, indent+2);
            break;
        case AST_IF:
            printf("If\n");
            print_indent(indent+2); printf("Cond:\n");
            print_ast(node->data.if_stmt.cond, indent+4);
            print_indent(indent+2); printf("Then:\n");
            print_ast(node->data.if_stmt.then_branch, indent+4);
            if (node->data.if_stmt.else_branch) {
                print_indent(indent+2); printf("Else:\n");
                print_ast(node->data.if_stmt.else_branch, indent+4);
            }
            break;
        case AST_WHILE:
            printf("While\n");
            print_indent(indent+2); printf("Cond:\n");
            print_ast(node->data.while_stmt.cond, indent+4);
            print_indent(indent+2); printf("Body:\n");
            print_ast(node->data.while_stmt.body, indent+4);
            break;
        case AST_RETURN:
            printf("Return\n");
            print_ast(node->data.ret.expr, indent+2);
            break;
        case AST_ASSIGN:
            printf("Assign\n");
            print_indent(indent+2); printf("Var:\n");
            print_ast(node->data.assign.var, indent+4);
            print_indent(indent+2); printf("Expr:\n");
            print_ast(node->data.assign.expr, indent+4);
            break;
        case AST_BINOP:
            printf("BinOp: %s\n", node->data.binop.op);
            print_ast(node->data.binop.left, indent+2);
            print_ast(node->data.binop.right, indent+2);
            break;
        case AST_UNOP:
            printf("UnOp: %s\n", node->data.unop.op);
            print_ast(node->data.unop.expr, indent+2);
            break;
        case AST_CALL:
            if (node->data.call.left) {
                printf("Call (pexpr):\n");
                print_ast(node->data.call.left, indent+2);
                print_indent(indent+2); printf("Args:\n");
                for (ASTNodeList *l = node->data.call.args; l; l = l->next)
                    print_ast(l->node, indent+4);
            } else {
                printf("Call: %s\n", node->data.call.name ? node->data.call.name : "(pexpr)");
                for (ASTNodeList *l = node->data.call.args; l; l = l->next)
                    print_ast(l->node, indent+2);
            }
            break;
        case AST_VAR:
            printf("Var: %s\n", node->data.var.name);
            break;
        case AST_NUM:
            printf("Num: %d\n", node->data.num.value);
            break;
        case AST_CHAR:
            printf("Char: '%c'\n", node->data.char_lit.value);
            break;
        case AST_STRING:
            printf("String: \"%s\"\n", node->data.string_lit.value);
            break;
        case AST_BREAK:
            printf("Break\n");
            break;
        case AST_CONTINUE:
            printf("Continue\n");
            break;
        case AST_EMPTY:
            printf("Empty\n");
            break;
        case AST_EXTERN:
            printf("Extern: %s%s\n", node->data.ext.name, node->data.ext.is_func ? "()" : "");
            break;
        case AST_INDEX:
            printf("Index\n");
            print_indent(indent+2); printf("Array:\n");
            print_ast(node->data.index.array, indent+4);
            print_indent(indent+2); printf("Index:\n");
            print_ast(node->data.index.index, indent+4);
            break;
        case AST_GLOBAL:
            printf("Global: %s", node->data.global.name);
            if (node->data.global.init) {
                printf(" = ");
                print_ast(node->data.global.init, 0);
            } else {
                printf(" (uninitialized)");
            }
            printf("\n");
            break;
        case AST_LABEL:
            printf("Label: %s\n", node->data.label.label);
            break;
        case AST_GOTO:
            printf("Goto: %s\n", node->data.go.label);
            break;
        case AST_META:
            printf("Meta: %s\n", node->data.meta.content);
            break;
        case AST_VAR_DECL:
            printf("VarDecl: %s\n", node->data.var_decl.name);
            break;
        default:
            printf("(Unknown node, type=%d)\n", node->type);
    }
}

// --- Parser State Functions ---
void parser_init(Parser *p, const char *src) {
    p->src = src;
    p->pos = 0;
    p->line = 1;
    p->col = 1;
    p->cur = src[0];
}
int parser_peek(Parser *p) {
    return p->cur;
}
int parser_next(Parser *p) {
    if (p->cur == '\n') { p->line++; p->col = 1; }
    else p->col++;
    if (p->src[p->pos]) p->pos++;
    p->cur = p->src[p->pos];
    return p->cur;
}
void parser_skip_ws(Parser *p) {
    while (1) {
        // Skip whitespace
        while (isspace(p->cur)) parser_next(p);
        // Skip C-style comments /* ... */
        if (p->cur == '/' && p->src[p->pos+1] == '*') {
            parser_next(p); parser_next(p);
            while (p->cur && !(p->cur == '*' && p->src[p->pos+1] == '/')) {
                parser_next(p);
            }
            if (p->cur) { parser_next(p); parser_next(p); }
            continue;
        }
        // Skip C++-style comments // ...
        if (p->cur == '/' && p->src[p->pos+1] == '/') {
            parser_next(p); parser_next(p);
            while (p->cur && p->cur != '\n') parser_next(p);
            continue;
        }
        break;
    }
}
int parser_match(Parser *p, const char *kw) {
    parser_skip_ws(p);
    size_t len = strlen(kw);
    if (strncmp(p->src + p->pos, kw, len) == 0) {
        for (size_t i = 0; i < len; i++) parser_next(p);
        return 1;
    }
    return 0;
}
void parser_error(Parser *p, const char *msg) {
    if (current_filename)
        fprintf(stderr, "Parse error at %s:%d:%d: %s\n", current_filename, p->line, p->col, msg);
    else
        fprintf(stderr, "Parse error at <input>:%d:%d: %s\n", p->line, p->col, msg);
    exit(1);
}

// --- Parser Functions ---
// Forward declarations for recursive parsing
ASTNode *parse_function(Parser *p);
ASTNode *parse_block(Parser *p);
ASTNode *parse_statement(Parser *p);
ASTNode *parse_expression(Parser *p);
ASTNode *parse_primary(Parser *p);
ASTNode *parse_identifier(Parser *p);
ASTNode *parse_number(Parser *p);
ASTNode *parse_call(Parser *p, char *name);
ASTNode *parse_postfix(Parser *p); // forward declaration
ASTNode *parse_unary(Parser *p); // forward declaration
char *parse_name(Parser *p);

// Helper: allocate and copy string
char *strdup2(const char *src, size_t len) {
    char *s = (char*)malloc(len+1);
    memcpy(s, src, len);
    s[len] = 0;
    return s;
}

// Helper: expect a specific character
void expect(Parser *p, char c) {
    parser_skip_ws(p);
    if (parser_peek(p) != c) {
        char msg[128];
        snprintf(msg, sizeof(msg), "Expected '%c', got '%c' at pos=%zu line=%d col=%d", c, parser_peek(p), p->pos, p->line, p->col);
        parser_error(p, msg);
    }
    parser_next(p);
}

ASTNode *parse_char_literal(Parser *p) {
    expect(p, '\'');
    int c = parser_peek(p);
    if (c == '\\') {
        parser_next(p);
        c = parser_peek(p);
        if (c == 'n') c = '\n';
        else if (c == 't') c = '\t';
        else if (c == '\\') c = '\\';
        else if (c == '\'') c = '\'';
        else parser_error(p, "Unknown escape in char literal");
    }
    parser_next(p);
    expect(p, '\'');
    ASTNode *n = make_node(AST_CHAR);
    n->data.char_lit.value = (char)c;
    return n;
}
ASTNode *parse_string_literal(Parser *p) {
    expect(p, '"');
    char buf[1024];
    int i = 0;
    while (parser_peek(p) && parser_peek(p) != '"') {
        int c = parser_peek(p);
        if (c == '\\') {
            parser_next(p);
            c = parser_peek(p);
            if (c == 'n') c = '\n';
            else if (c == 't') c = '\t';
            else if (c == '\\') c = '\\';
            else if (c == '"') c = '"';
            else parser_error(p, "Unknown escape in string literal");
        }
        if (i < 1023) buf[i++] = (char)c;
        parser_next(p);
    }
    buf[i] = 0;
    expect(p, '"');
    ASTNode *n = make_node(AST_STRING);
    n->data.string_lit.value = strdup2(buf, strlen(buf));
    return n;
}
ASTNode *parse_primary(Parser *p) {
    parser_skip_ws(p);
    int c = parser_peek(p);
    if (c == '\'') return parse_char_literal(p);
    if (c == '"') return parse_string_literal(p);
    return parse_postfix(p);
}

// Parse a name (identifier)
char *parse_name(Parser *p) {
    parser_skip_ws(p);
    if (!isalpha(parser_peek(p)) && parser_peek(p) != '_')
        parser_error(p, "Expected identifier");
    size_t start = p->pos;
    while (isalnum(parser_peek(p)) || parser_peek(p) == '_') parser_next(p);
    return strdup2(p->src + start, p->pos - start);
}

// Parse a number
ASTNode *parse_number(Parser *p) {
    parser_skip_ws(p);
    int val = 0;
    if (!isdigit(parser_peek(p))) parser_error(p, "Expected number");
    while (isdigit(parser_peek(p))) {
        val = val * 10 + (parser_peek(p) - '0');
        parser_next(p);
    }
    ASTNode *n = make_node(AST_NUM);
    n->data.num.value = val;
    return n;
}

// Helper: check if a name is a function name (for function pointer support)
int is_function_name(const char *name, ASTNodeList *funcs) {
    for (ASTNodeList *l = funcs; l; l = l->next) {
        if (l->node->type == AST_FUNCTION && strcmp(l->node->data.function.name, name) == 0)
            return 1;
    }
    return 0;
}

// Update parse_identifier to allow function names as rvalues
ASTNode *parse_identifier(Parser *p) {
    char *name = parse_name(p);
    parser_skip_ws(p);
    if (parser_peek(p) == '(') {
        return parse_call(p, name);
    } else {
        // Check if this is a function name (function pointer)
        if (is_function_name(name, top_level_funcs)) {
            ASTNode *n = make_node(AST_VAR);
            n->data.var.name = name;
            n->type = AST_VAR; // treat as function pointer
            return n;
        }
        ASTNode *n = make_node(AST_VAR);
        n->data.var.name = name;
        return n;
    }
}

// Parse a function call
ASTNode *parse_call(Parser *p, char *name) {
    expect(p, '(');
    ASTNodeList *args = NULL;
    parser_skip_ws(p);
    if (parser_peek(p) != ')') {
        while (1) {
            ASTNode *arg = parse_expression(p);
            args = append_node(args, arg);
            parser_skip_ws(p);
            if (parser_peek(p) == ',') {
                parser_next(p);
            } else {
                break;
            }
        }
    }
    expect(p, ')');
    ASTNode *n = make_node(AST_CALL);
    n->data.call.name = name;
    n->data.call.args = args;
    return n;
}

// Refactor parse_primary to always call parse_postfix
// Refactor parse_postfix to handle all primary expressions
ASTNode *parse_postfix(Parser *p) {
    parser_skip_ws(p);
    ASTNode *node = NULL;
    int c = parser_peek(p);
    if (isalpha(c) || c == '_') {
        char *name = parse_name(p);
        parser_skip_ws(p);
        if (parser_peek(p) == '(') {
            node = parse_call(p, name);
        } else {
            node = make_node(AST_VAR);
            node->data.var.name = name;
        }
    } else if (c == '(') {
        parser_next(p);
        node = parse_expression(p);
        expect(p, ')');
    } else if (isdigit(c)) {
        node = parse_number(p);
    } else if (c == '\'' || c == '"') {
        // Char or string literal
        if (c == '\'') node = parse_char_literal(p);
        else node = parse_string_literal(p);
    } else if (c == '-' || c == '+' || c == '*' || c == '&') {
        node = parse_unary(p);
    } else {
        parser_error(p, "Unexpected character in expression");
        return NULL;
    }
    parser_skip_ws(p);
    // Allow chains of calls, array accesses, and postfix ++/-- on any expression
    while (1) {
        if (parser_peek(p) == '(') {
            expect(p, '(');
            ASTNodeList *args = NULL;
            parser_skip_ws(p);
            if (parser_peek(p) != ')') {
                while (1) {
                    ASTNode *arg = parse_expression(p);
                    args = append_node(args, arg);
                    parser_skip_ws(p);
                    if (parser_peek(p) == ',') {
                        parser_next(p);
                    } else {
                        break;
                    }
                }
            }
            expect(p, ')');
            ASTNode *call = make_node(AST_CALL);
            call->data.call.name = NULL;
            call->data.call.args = args;
            call->data.call.left = node;
            node = call;
        } else if (parser_peek(p) == '[') {
            parser_next(p);
            ASTNode *idx = parse_expression(p);
            expect(p, ']');
            ASTNode *arr = make_node(AST_INDEX);
            arr->data.index.array = node;
            arr->data.index.index = idx;
            node = arr;
        } else if ((parser_peek(p) == '+' && p->src[p->pos+1] == '+') || (parser_peek(p) == '-' && p->src[p->pos+1] == '-')) {
            char op[3] = { (char)parser_peek(p), (char)parser_peek(p), 0 };
            parser_next(p); parser_next(p);
            ASTNode *n = make_node(AST_UNOP);
            n->data.unop.op = strdup2(op, 2);
            n->data.unop.expr = node;
            n->data.unop.is_postfix = 1; // postfix
            node = n;
        } else {
            break;
        }
        parser_skip_ws(p);
    }
    return node;
}

// Parse unary operators
ASTNode *parse_unary(Parser *p) {
    parser_skip_ws(p);
    int c = parser_peek(p);
    // Handle prefix ++ and --
    if ((c == '+' && p->src[p->pos+1] == '+') || (c == '-' && p->src[p->pos+1] == '-')) {
        char op[3] = { (char)c, (char)c, 0 };
        parser_next(p); parser_next(p);
        ASTNode *n = make_node(AST_UNOP);
        n->data.unop.op = strdup2(op, 2);
        n->data.unop.expr = parse_unary(p);
        n->data.unop.is_postfix = 0; // prefix
        return n;
    }
    return parse_primary(p);
}

// --- Operator precedence and recognition ---
typedef struct { const char *op; int prec; } OpPrec;
static const OpPrec op_table[] = {
    {"=", 0},
    {"||", 1},
    {"&&", 2},
    {"==", 3}, {"!=", 3},
    {">=", 4}, {"<=", 4}, {">>", 4}, {"<<", 4},
    {">", 4}, {"<", 4},
    {"+", 5}, {"-", 5},
    {"&", 7},
    {"^", 8},
    {"|", 9},
    {"*", 10}, {"/", 10}, {"%", 10},
    {NULL, 0}
};

// Try to match an operator at the current position, return its precedence and length
int match_op(Parser *p, const char **op_out, int *len_out, int *prec_out) {
    for (int i = 0; op_table[i].op; ++i) {
        size_t len = strlen(op_table[i].op);
        if (strncmp(p->src + p->pos, op_table[i].op, len) == 0) {
            if (op_out) *op_out = op_table[i].op;
            if (len_out) *len_out = (int)len;
            if (prec_out) *prec_out = op_table[i].prec;
            return 1;
        }
    }
    return 0;
}

// --- Updated parse_binop_rhs and get_precedence ---
int get_precedence(Parser *p, const char **op_out, int *len_out) {
    int prec = 0;
    const char *op = NULL;
    int len = 0;
    if (match_op(p, &op, &len, &prec)) {
        if (op_out) *op_out = op;
        if (len_out) *len_out = len;
        return prec;
    }
    return -1;
}

ASTNode *parse_binop_rhs(Parser *p, int prec, ASTNode *lhs) {
    while (1) {
        parser_skip_ws(p);
        const char *op = NULL;
        int op_len = 0;
        int op_prec = get_precedence(p, &op, &op_len);
        if (op_prec < prec) return lhs;
        // Advance past the operator
        for (int i = 0; i < op_len; ++i) parser_next(p);
        ASTNode *rhs = parse_unary(p);
        parser_skip_ws(p);
        const char *next_op = NULL;
        int next_len = 0;
        int next_prec = get_precedence(p, &next_op, &next_len);
        if (strcmp(op, "=") == 0) {
            // Assignment is right-associative
            if (op_prec <= next_prec) {
                rhs = parse_binop_rhs(p, op_prec, rhs);
            }
        } else {
            // All others are left-associative
            if (op_prec < next_prec) {
                rhs = parse_binop_rhs(p, op_prec + 1, rhs);
            }
        }
        ASTNode *node = NULL;
        if (strcmp(op, "=") == 0) {
            node = make_node(AST_ASSIGN);
            node->data.assign.var = lhs;
            node->data.assign.expr = rhs;
        } else {
            node = make_node(AST_BINOP);
            node->data.binop.op = strdup2(op, op_len);
            node->data.binop.left = lhs;
            node->data.binop.right = rhs;
        }
        lhs = node;
    }
}

// Update parse_expression to use new get_precedence
ASTNode *parse_expression(Parser *p) {
    ASTNode *lhs = parse_unary(p);
    return parse_binop_rhs(p, 0, lhs);
}

// Parse a statement
ASTNode *parse_statement(Parser *p) {
    parser_skip_ws(p);
    // Label definition: label:
    if (isalpha(parser_peek(p)) || parser_peek(p) == '_') {
        size_t save_pos = p->pos;
        int save_cur = p->cur;
        char *name = parse_name(p);
        parser_skip_ws(p);
        if (parser_peek(p) == ':') {
            parser_next(p);
            ASTNode *n = make_node(AST_LABEL);
            n->data.label.label = name;
            return n;
        } else {
            // Not a label, revert and continue
            p->pos = save_pos;
            p->cur = save_cur;
        }
    }
    // Goto statement: goto label;
    if (parser_match(p, "goto")) {
        char *name = parse_name(p);
        expect(p, ';');
        ASTNode *n = make_node(AST_GOTO);
        n->data.go.label = name;
        return n;
    }
    if (parser_match(p, "if")) {
        expect(p, '(');
        ASTNode *cond = parse_expression(p);
        expect(p, ')');
        ASTNode *then_branch = parse_statement(p);
        ASTNode *else_branch = NULL;
        parser_skip_ws(p);
        if (parser_match(p, "else")) {
            else_branch = parse_statement(p);
        }
        ASTNode *n = make_node(AST_IF);
        n->data.if_stmt.cond = cond;
        n->data.if_stmt.then_branch = then_branch;
        n->data.if_stmt.else_branch = else_branch;
        return n;
    } else if (parser_match(p, "while")) {
        expect(p, '(');
        ASTNode *cond = parse_expression(p);
        expect(p, ')');
        ASTNode *body = parse_statement(p);
        ASTNode *n = make_node(AST_WHILE);
        n->data.while_stmt.cond = cond;
        n->data.while_stmt.body = body;
        return n;
    } else if (parser_match(p, "return")) {
        ASTNode *expr = parse_expression(p);
        expect(p, ';');
        ASTNode *n = make_node(AST_RETURN);
        n->data.ret.expr = expr;
        return n;
    } else if (parser_match(p, "break")) {
        expect(p, ';');
        return make_node(AST_BREAK);
    } else if (parser_match(p, "continue")) {
        expect(p, ';');
        return make_node(AST_CONTINUE);
    } else if (parser_match(p, "auto")) {
        char *name = parse_name(p);
        expect(p, ';');
        ASTNode *n = make_node(AST_VAR_DECL);
        n->data.var_decl.name = name;
        return n;
    } else if (parser_match(p, "extern")) {
        parser_skip_ws(p);
        char *name = parse_name(p);
        parser_skip_ws(p);
        expect(p, ';');
        ASTNode *n = make_node(AST_EXTERN);
        n->data.ext.name = name;
        n->data.ext.is_func = 0;
        return n;
    } else if (parser_peek(p) == ';') {
        parser_next(p);
        return make_node(AST_EMPTY);
    } else if (parser_peek(p) == '{') {
        return parse_block(p);
    } else if (isalpha(parser_peek(p)) || parser_peek(p) == '_') {
        size_t save_pos = p->pos;
        int save_cur = p->cur;
        char *name = parse_name(p);
        parser_skip_ws(p);
        if (parser_peek(p) == '(') {
            ASTNode *call = parse_call(p, name);
            expect(p, ';');
            ASTNode *n = make_node(AST_STATEMENT);
            n->data.statement.stmt = call;
            return n;
        } else {
            p->pos = save_pos;
            p->cur = save_cur;
        }
    }
    // Assignment or expression
    ASTNode *expr = parse_expression(p);
    expect(p, ';');
    ASTNode *n = make_node(AST_STATEMENT);
    n->data.statement.stmt = expr;
    return n;
}

// Parse a block: { ... }
ASTNode *parse_block(Parser *p) {
    expect(p, '{');
    ASTNodeList *stmts = NULL;
    parser_skip_ws(p);
    while (parser_peek(p) && parser_peek(p) != '}') {
        ASTNode *stmt = parse_statement(p);
        stmts = append_node(stmts, stmt);
        parser_skip_ws(p);
    }
    expect(p, '}');
    ASTNode *n = make_node(AST_BLOCK);
    n->data.block.statements = stmts;
    return n;
}

// Parse a function definition: name ( ) block
ASTNode *parse_function(Parser *p) {
    char *name = parse_name(p);
    parser_skip_ws(p);
    expect(p, '(');
    parser_skip_ws(p);
    ASTNodeList *params = NULL;
    if (parser_peek(p) != ')') {
        while (1) {
            char *param_name = parse_name(p);
            ASTNode *param = make_node(AST_VAR);
            param->data.var.name = param_name;
            params = append_node(params, param);
            parser_skip_ws(p);
            if (parser_peek(p) == ',') {
                parser_next(p);
                parser_skip_ws(p);
            } else {
                break;
            }
        }
    }
    expect(p, ')');
    parser_skip_ws(p);
    ASTNode *body = parse_block(p);
    ASTNode *n = make_node(AST_FUNCTION);
    n->data.function.name = name;
    n->data.function.params = params;
    n->data.function.body = body;
    return n;
}

// Parse the whole program: list of functions
ASTNode *parse_program(Parser *p) {
    ASTNodeList *funcs = NULL;
    parser_skip_ws(p);
    top_level_funcs = funcs;
    while (parser_peek(p)) {
        parser_skip_ws(p);
        if (parser_match(p, "extern")) {
            parser_skip_ws(p);
            char *name = parse_name(p);
            parser_skip_ws(p);
            // Only allow extern name;
            expect(p, ';');
            ASTNode *n = make_node(AST_EXTERN);
            n->data.ext.name = name;
            n->data.ext.is_func = 0;
            funcs = append_node(funcs, n);
        } else if (parser_match(p, "meta")) {
            expect(p, '{');
            // Parse balanced brackets content
            int brace_count = 1;
            size_t start_pos = p->pos;
            while (brace_count > 0 && parser_peek(p)) {
                int c = parser_next(p);
                if (c == '{') brace_count++;
                else if (c == '}') brace_count--;
            }
            if (brace_count > 0) {
                parser_error(p, "Unmatched braces in meta construct");
                return NULL;
            }
            // Extract content (excluding the closing brace)
            size_t end_pos = p->pos - 1;
            size_t content_len = end_pos - start_pos;
            char *content = strdup2(p->src + start_pos, content_len);
            
            parser_next(p); // Advance past the closing brace to sync cur/pos
            
            ASTNode *n = make_node(AST_META);
            n->data.meta.content = content;
            funcs = append_node(funcs, n);
            // The parser position is now at the character after the closing brace
            // No need to skip whitespace here as it will be done at the end of the loop
        } else if (isalpha(parser_peek(p)) || parser_peek(p) == '_') {
            // Could be function definition or global variable
            size_t save_pos = p->pos;
            int save_cur = p->cur;
            char *name = parse_name(p);
            parser_skip_ws(p);
            if (parser_peek(p) == '(') {
                // Function definition
                p->pos = save_pos;
                p->cur = save_cur;
                ASTNode *fn = parse_function(p);
                funcs = append_node(funcs, fn);
                top_level_funcs = funcs;
            } else {
                // Global variable definition: name [= expr]?;
                ASTNode *init = NULL;
                if (parser_peek(p) == '=') {
                    parser_next(p);
                    init = parse_expression(p);
                }
                expect(p, ';');
                ASTNode *n = make_node(AST_GLOBAL);
                n->data.global.name = name;
                n->data.global.init = init;
                funcs = append_node(funcs, n);
            }
        } else {
            parser_error(p, "Expected 'extern', 'meta', function definition, or global variable at top level");
        }
        parser_skip_ws(p);
    }
    ASTNode *n = make_node(AST_PROGRAM);
    n->data.program.functions = funcs;
    return n;
} 