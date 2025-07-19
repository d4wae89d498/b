#ifndef B_H
#define B_H

#include <stdio.h>

// --- AST Node Types ---
typedef enum {
    AST_PROGRAM,
    AST_FUNCTION,
    AST_BLOCK,
    AST_STATEMENT,
    AST_IF,
    AST_WHILE,
    AST_FOR,
    AST_RETURN,
    AST_BREAK,
    AST_CONTINUE,
    AST_ASSIGN,
    AST_BINOP,
    AST_UNOP,
    AST_CALL,
    AST_VAR,
    AST_NUM,
    AST_CHAR,
    AST_STRING,
    AST_EMPTY,
    AST_EXTERN,
    AST_INDEX,
    AST_GLOBAL,
    AST_LABEL,
    AST_GOTO
} ASTNodeType;

struct ASTNode;

typedef struct ASTNodeList {
    struct ASTNode *node;
    struct ASTNodeList *next;
} ASTNodeList;

typedef struct ASTNode {
    ASTNodeType type;
    union {
        struct { ASTNodeList *functions; } program;
        struct { char *name; ASTNodeList *params; struct ASTNode *body; } function;
        struct { ASTNodeList *statements; } block;
        struct { struct ASTNode *stmt; } statement;
        struct { struct ASTNode *cond, *then_branch, *else_branch; } if_stmt;
        struct { struct ASTNode *cond, *body; } while_stmt;
        struct { struct ASTNode *init, *cond, *step, *body; } for_stmt;
        struct { struct ASTNode *expr; } ret;
        struct { struct ASTNode *var, *expr; } assign;
        struct { char *op; struct ASTNode *left, *right; } binop;
        struct { char *op; struct ASTNode *expr; } unop;
        struct { char *name; ASTNodeList *args; struct ASTNode *left; } call;
        struct { char *name; } var;
        struct { int value; } num;
        struct { char value; } char_lit;
        struct { char *value; } string_lit;
        struct { char *name; int is_func; } ext;
        struct { struct ASTNode *array, *index; } index;
        struct { char *name; struct ASTNode *init; } global;
        struct { char *label; } label;
        struct { char *label; } go;
    } data;
} ASTNode;

#endif // B_PARSER_H 