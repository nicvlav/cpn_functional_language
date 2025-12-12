#ifndef __cilisp_h_
#define __cilisp_h_

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>


#define NAN_RET_VAL (RET_VAL){DOUBLE_TYPE, NAN}
#define ZERO_RET_VAL (RET_VAL){INT_TYPE, 0}


#define BISON_FLEX_LOG_PATH "bison_flex.log"
// Use extern to work with Makefile building
extern FILE* read_target;
extern FILE* flex_bison_log_file;
size_t yyreadline(char **lineptr, size_t *n, FILE *stream, size_t n_terminate);


int yyparse(void);
int yylex(void);
void yyerror(char *, ...);
void warning(char*, ...);


typedef enum func_type {
    NEG_FUNC,
    ABS_FUNC,
    ADD_FUNC,
    SUB_FUNC,
    MULT_FUNC,
    DIV_FUNC,
    REM_FUNC,
    EXP_FUNC,
    EXP2_FUNC,
    POW_FUNC,
    LOG_FUNC,
    SQRT_FUNC,
    CBRT_FUNC,
    HYPOT_FUNC,
    MAX_FUNC,
    MIN_FUNC,
    CUSTOM_FUNC
} FUNC_TYPE;


FUNC_TYPE resolveFunc(char *);

// helper to copy a string to a new dynamically allocated char array
char * cloneString(char *);

typedef enum num_type {
    INT_TYPE,
    DOUBLE_TYPE,
    NO_TYPE
} NUM_TYPE;

NUM_TYPE resolveType(char *);


typedef struct {
    NUM_TYPE type;
    double value;
} AST_NUMBER;

typedef AST_NUMBER RET_VAL;


typedef struct ast_function {
    FUNC_TYPE func;
    struct ast_node *opList;
} AST_FUNCTION;


typedef enum {
    NUM_NODE_TYPE,
    FUNC_NODE_TYPE,
    SYM_NODE_TYPE,
    SCOPE_NODE_TYPE
} AST_NODE_TYPE;

typedef struct {
    char* id;
} AST_SYMBOL;

typedef struct {
    struct ast_node *child;
} AST_SCOPE;

typedef struct ast_node {
    AST_NODE_TYPE type;
    struct ast_node *parent;
    struct symbol_table_node *symbolTable;
    union {
        AST_NUMBER number;
        AST_FUNCTION function;
        AST_SYMBOL symbol;
        AST_SCOPE scope;
    } data;
    struct ast_node *next;
} AST_NODE;

typedef struct symbol_table_node {
    char *id;
    AST_NODE *value;
    NUM_TYPE type;
    struct symbol_table_node *next;
} SYMBOL_TABLE_NODE;

AST_NODE *createNumberNode(double value, NUM_TYPE type);
AST_NODE *createFunctionNode(FUNC_TYPE func, AST_NODE *opList);
AST_NODE *addExpressionToList(AST_NODE *newExpr, AST_NODE *exprList);
AST_NODE *createSymbolReferenceNode(char* id);

AST_NODE *createScopeNode(SYMBOL_TABLE_NODE *symbol, AST_NODE *child);
SYMBOL_TABLE_NODE *createSymbolNode(char* value, AST_NODE *s_expr);
SYMBOL_TABLE_NODE *createTypecastSymbolNode(char* value, AST_NODE *s_expr, NUM_TYPE type);
SYMBOL_TABLE_NODE *addSymbolToList(SYMBOL_TABLE_NODE *newSymbol, SYMBOL_TABLE_NODE *symbolList);

RET_VAL eval(AST_NODE *node);

void printRetVal(RET_VAL val);

void freeNode(AST_NODE *node);

#endif
