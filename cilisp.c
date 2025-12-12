#include "cilisp.h"

#define RED             "\033[31m"
#define RESET_COLOR     "\033[0m"

// Build shenanigans, define the extern variable here
FILE* read_target;
FILE* flex_bison_log_file;

// yyerror:
// Something went so wrong that the whole program should crash.
// You should basically never call this unless an allocation fails.
// (see the "yyerror("Memory allocation failed!")" calls and do the same.
// This is basically printf, but red, with "\nERROR: " prepended, "\n" appended,
// and an "exit(1);" at the end to crash the program.
// It's called "yyerror" instead of "error" so the parser will use it for errors too.
void yyerror(char *format, ...)
{
    char buffer[256];
    va_list args;
    va_start (args, format);
    vsnprintf (buffer, 255, format, args);

    printf(RED "\nERROR: %s\nExiting...\n" RESET_COLOR, buffer);
    fflush(stdout);

    va_end (args);
    exit(1);
}

// warning:
// Something went mildly wrong (on the user-input level, probably)
// Let the user know what happened and what you're doing about it.
// Then, move on. No big deal, they can enter more inputs. ¯\_(ツ)_/¯
// You should use this pretty often:
//      too many arguments, let them know and ignore the extra
//      too few arguments, let them know and return NAN
//      invalid arguments, let them know and return NAN
//      many more uses to be added as we progress...
// This is basically printf, but red, and with "\nWARNING: " prepended and "\n" appended.
void warning(char *format, ...)
{
    char buffer[256];
    va_list args;
    va_start (args, format);
    vsnprintf (buffer, 255, format, args);

    printf(RED "WARNING: %s\n" RESET_COLOR, buffer);
    fflush(stdout);

    va_end (args);
}

FUNC_TYPE resolveFunc(char *funcName)
{
    // Array of string values for function names.
    // Must be in sync with members of the FUNC_TYPE enum in order for resolveFunc to work.
    // For example, funcNames[NEG_FUNC] should be "neg"
    char *funcNames[] = {
            "neg",
            "abs",
            "add",
            "sub",
            "mult",
            "div",
            "remainder",
            "exp",
            "exp2",
            "pow",
            "log",
            "sqrt",
            "cbrt",
            "hypot",
            "max",
            "min",
            ""
    };
    int i = 0;
    while (funcNames[i][0] != '\0')
    {
        if (strcmp(funcNames[i], funcName) == 0)
        {
            return i;
        }
        i++;
    }
    return CUSTOM_FUNC;
}

char* resolveSymbol(char *symbol) {
    char *copy = (char *) malloc(strlen(symbol) + 1);
    if (copy != NULL) {
      strcpy(copy, symbol);
    }
    return copy;
}

AST_NODE *createNumberNode(double value, NUM_TYPE type)
{
    AST_NODE *node;
    size_t nodeSize;

    nodeSize = sizeof(AST_NODE);
    if ((node = calloc(nodeSize, 1)) == NULL)
    {
        yyerror("Memory allocation failed!");
        exit(1);
    }

    node->data.number = (AST_NUMBER){type, value};
    node->type = NUM_NODE_TYPE;

    return node;
}

SYMBOL_TABLE_NODE *createSymbolNode(char* value, AST_NODE *s_expr)
{
    SYMBOL_TABLE_NODE *node;
    size_t nodeSize;

    nodeSize = sizeof(SYMBOL_TABLE_NODE);
    if ((node = calloc(nodeSize, 1)) == NULL)
    {
        yyerror("Memory allocation failed!");
        exit(1);
    }

    node->id = value;
    node->value = s_expr;
    return node;
}

SYMBOL_TABLE_NODE *addSymbolToList(SYMBOL_TABLE_NODE *newSymbol, SYMBOL_TABLE_NODE *symbolList) {
    if (!newSymbol)
    {
        yyerror("NULL ast node passed into evalFuncNode for newExpr!");
        return symbolList; // unreachable but kills a clang-tidy warning
    }

    newSymbol->next = symbolList;

    return newSymbol;
}

AST_NODE *createSymbolReferenceNode(char* id) {
    AST_NODE *node;
    size_t nodeSize;

    nodeSize = sizeof(AST_NODE);
    if ((node = calloc(nodeSize, 1)) == NULL)
    {
        yyerror("Memory allocation failed!");
        exit(1);
    }

    node->type = SYM_NODE_TYPE;
    node->data.symbol.id = id;

    return node;
}


AST_NODE *createFunctionNode(FUNC_TYPE func, AST_NODE *opList)
{
    AST_NODE *node;
    size_t nodeSize;

    nodeSize = sizeof(AST_NODE);
    if ((node = calloc(nodeSize, 1)) == NULL)
    {
        yyerror("Memory allocation failed!");
        exit(1);
    }

    node->data.function.func = func;
    node->data.function.opList = opList;
    node->type = FUNC_NODE_TYPE;

    // Set parent pointers for all nodes in list 
    // so nested functions can access scope symbols
    AST_NODE *current = opList;
    while (current != NULL) {
        current->parent = node;
        current = current->next;
    }

    return node;
}

AST_NODE *createScopeNode(SYMBOL_TABLE_NODE *symbol, AST_NODE *child) {
    AST_NODE *node;
    size_t nodeSize;

    nodeSize = sizeof(AST_NODE);
    if ((node = calloc(nodeSize, 1)) == NULL)
    {
        yyerror("Memory allocation failed!");
        exit(1);
    }

    // Set the node type, the scope node has a single child
    node->type = SCOPE_NODE_TYPE;
    node->data.scope.child = child;

    // Set the child's parent to this new scope node
    child->parent = node;

    // Assign the child scopes symbol table to the symbol (let section in the grammar)
    child->symbolTable = symbol;

    // All symbol siblings in the symbol table must have their parent set to the new child scope 
    SYMBOL_TABLE_NODE *current = symbol;
    while (current != NULL) {
        if (current->value != NULL) {
            current->value->parent = child;
        }
        current = current->next;
    }

    return node;
}

AST_NODE *addExpressionToList(AST_NODE *newExpr, AST_NODE *exprList)
{
    if (!newExpr)
    {
        yyerror("NULL ast node passed into evalFuncNode for newExpr!");
        return exprList; // unreachable but kills a clang-tidy warning
    }

    newExpr->next = exprList;

    return newExpr;
}

RET_VAL evalNegFuncNode(AST_NODE *node) {
    if (!node)
    {
        yyerror("NULL ast node passed into evalNegFuncNode!");
        return NAN_RET_VAL; 
    }

    if (node->data.function.opList == NULL)
    {
        warning("No operands passed into neg");
        return NAN_RET_VAL;
    }

    RET_VAL r = eval(node->data.function.opList);
    r.value *= -1.0;
    return r;
}

RET_VAL evalAbsFuncNode(AST_NODE *node) {
    if (!node)
    {
        yyerror("NULL ast node passed into evalAbsFuncNode!");
        return NAN_RET_VAL; 
    }

    if (node->data.function.opList == NULL)
    {
        warning("No operands passed into abs");
        return NAN_RET_VAL;
    }

    RET_VAL r = eval(node->data.function.opList);
    r.value = fabs(r.value);
    return r;
}

RET_VAL evalAddFuncNode(AST_NODE *node) {
    if (!node)
    {
        yyerror("NULL ast node passed into evalAddFuncNode!");
        return NAN_RET_VAL; 
    }

    AST_NODE *current = node->data.function.opList;

    if (current == NULL)
    {
        warning("No operands passed into add!");
        return ZERO_RET_VAL;
    }

    RET_VAL result = eval(current);

    while (current->next != NULL) {
        RET_VAL newVal = eval(current->next);

        // convert overall type to double if there is any double operand
        if (result.type == INT_TYPE && newVal.type == DOUBLE_TYPE) {
            result.type = DOUBLE_TYPE;
        }

        result.value += newVal.value;
        current = current->next;
    }

    return result;
}

RET_VAL evalSubFuncNode(AST_NODE *node) {
    if (!node)
    {
        yyerror("NULL ast node passed into evalDivFuncNode!");
        return NAN_RET_VAL; 
    }

    if (node->data.function.opList == NULL)
    {
        warning("No operands passed into sub!");
        return NAN_RET_VAL;
    }

    if (node->data.function.opList->next == NULL)
    {
        warning("Only one operand passed into sub!");
        return NAN_RET_VAL;
    }

    if (node->data.function.opList->next->next != NULL)
    {
        warning("sub called with extra (ignored) operands!!");
    }

    RET_VAL left = eval(node->data.function.opList);
    RET_VAL right = eval(node->data.function.opList->next);

    if (right.type == DOUBLE_TYPE ) {
        left.type = DOUBLE_TYPE;
    } else {
        left.type = INT_TYPE;
    }

    left.value = left.value - right.value;

    return left;
}

RET_VAL evalMultFuncNode(AST_NODE *node) {
    if (!node)
    {
        yyerror("NULL ast node passed into evalMultFuncNode!");
        return NAN_RET_VAL; 
    }

    AST_NODE *current = node->data.function.opList;

    if (current == NULL)
    {
        warning("No operands passed into mult!");
        return (RET_VAL){INT_TYPE, 1};
    }

    RET_VAL result = eval(current);

    while (current->next != NULL) {
        RET_VAL newVal = eval(current->next);

        // convert overall type to double if there is any double operand
        if (result.type == INT_TYPE && newVal.type == DOUBLE_TYPE) {
            result.type = DOUBLE_TYPE;
        }

        result.value *= newVal.value;
        current = current->next;
    }

    return result;
}

RET_VAL evalDivFuncNode(AST_NODE *node) {
    if (!node)
    {
        yyerror("NULL ast node passed into evalDivFuncNode!");
        return NAN_RET_VAL; 
    }

    if (node->data.function.opList == NULL)
    {
        warning("No operands passed into div!");
        return NAN_RET_VAL;
    }

    if (node->data.function.opList->next == NULL)
    {
        warning("Only one operand passed into div!");
        return NAN_RET_VAL;
    }

    if (node->data.function.opList->next->next != NULL)
    {
        warning("div called with extra (ignored) operands!!");
    }

    RET_VAL left = eval(node->data.function.opList);
    RET_VAL right = eval(node->data.function.opList->next);

    if (left.type == DOUBLE_TYPE || right.type == DOUBLE_TYPE ) {
        left.type = DOUBLE_TYPE;
        left.value = left.value / right.value;
    } else {
        left.type = INT_TYPE;
        left.value = floor(left.value / right.value);
    }

    return left;
}

RET_VAL evalRemainderFuncNode(AST_NODE *node) {
    if (!node)
    {
        yyerror("NULL ast node passed into evalRemainderFuncNode!");
        return NAN_RET_VAL; 
    }

    if (node->data.function.opList == NULL)
    {
        warning("No operands passed into remainder!");
        return NAN_RET_VAL;
    }

    if (node->data.function.opList->next == NULL)
    {
        warning("Only one operand passed into remainder!");
        return NAN_RET_VAL;
    }

    if (node->data.function.opList->next->next != NULL)
    {
        warning("remainder called with extra (ignored) operands!!");
    }

    RET_VAL left = eval(node->data.function.opList);
    RET_VAL right = eval(node->data.function.opList->next);

    if (right.type == DOUBLE_TYPE ) {
        left.type = DOUBLE_TYPE;
    } 

    left.value = fmod(left.value, right.value);

    return left;
}

RET_VAL evalExpFuncNode(AST_NODE *node) {
    if (!node)
    {
        yyerror("NULL ast node passed into evalExpFuncNode!");
        return NAN_RET_VAL; 
    }

    if (node->data.function.opList == NULL)
    {
        warning("No operands passed into exp!");
        return NAN_RET_VAL;
    }

    if (node->data.function.opList->next != NULL)
    {
        warning("exp called with extra (ignored) operands!!");
    }

    RET_VAL result = eval(node->data.function.opList);
    result.value = expf(result.value);

    // Always make the final type a double
    result.type = DOUBLE_TYPE;

    return result;
}

RET_VAL evalExp2FuncNode(AST_NODE *node) {
    if (!node)
    {
        yyerror("NULL ast node passed into evalExp2FuncNode!");
        return NAN_RET_VAL; 
    }

    if (node->data.function.opList == NULL)
    {
        warning("No operands passed into exp2!");
        return NAN_RET_VAL;
    }

    if (node->data.function.opList->next != NULL)
    {
        warning("exp2 called with extra (ignored) operands!!");
    }

    RET_VAL result = eval(node->data.function.opList);

    // a negative operand means its always a double
    if (result.value < 0) {
        result.type = DOUBLE_TYPE;
    } 

    result.value = exp2f(result.value);

    return result;
}

RET_VAL evalPowFuncNode(AST_NODE *node) {
    if (!node)
    {
        yyerror("NULL ast node passed into evalPowFuncNode!");
        return NAN_RET_VAL; 
    }

    if (node->data.function.opList == NULL)
    {
        warning("No operands passed into pow!");
        return NAN_RET_VAL;
    }

    if (node->data.function.opList->next == NULL)
    {
        warning("Only one operand passed into pow!");
        return NAN_RET_VAL;
    }

    if (node->data.function.opList->next->next != NULL)
    {
        warning("pow called with extra (ignored) operands!!");
    }

    RET_VAL left = eval(node->data.function.opList);
    RET_VAL right = eval(node->data.function.opList->next);

    // if right type is double we ensure left changes to double if needed
    if (right.type == DOUBLE_TYPE ) {
        left.type = DOUBLE_TYPE;
    } 

    left.value = pow(left.value, right.value);

    return left;
}

RET_VAL evalLogFuncNode(AST_NODE *node) {
    if (!node)
    {
        yyerror("NULL ast node passed into evalLogFuncNode!");
        return NAN_RET_VAL; 
    }

    if (node->data.function.opList == NULL)
    {
        warning("No operands passed into log!");
        return NAN_RET_VAL;
    }

    if (node->data.function.opList->next != NULL)
    {
        warning("log called with extra (ignored) operands!!");
    }

    RET_VAL result = eval(node->data.function.opList);
    result.value = log(result.value);

    // log always returns a double
    result.type = DOUBLE_TYPE;

    return result;
}

RET_VAL evalSqrtFuncNode(AST_NODE *node) {
    if (!node)
    {
        yyerror("NULL ast node passed into evalSqrtFuncNode!");
        return NAN_RET_VAL; 
    }

    if (node->data.function.opList == NULL)
    {
        warning("No operands passed into sqrt!");
        return NAN_RET_VAL;
    }

    if (node->data.function.opList->next != NULL)
    {
        warning("sqrt called with extra (ignored) operands!!");
    }

    RET_VAL result = eval(node->data.function.opList);
    result.value = sqrt(result.value);

    // log always returns a double
    result.type = DOUBLE_TYPE;

    return result;
}

RET_VAL evalCbrtFuncNode(AST_NODE *node) {
    if (!node)
    {
        yyerror("NULL ast node passed into evalCbrtFuncNode!");
        return NAN_RET_VAL; 
    }

    if (node->data.function.opList == NULL)
    {
        warning("No operands passed into cbrt!");
        return NAN_RET_VAL;
    }

    if (node->data.function.opList->next != NULL)
    {
        warning("cbrt called with extra (ignored) operands!!");
    }

    RET_VAL result = eval(node->data.function.opList);
    result.value = cbrt(result.value);

    // log always returns a double
    result.type = DOUBLE_TYPE;

    return result;
}

RET_VAL evalHypotFuncNode(AST_NODE *node) {
    if (!node)
    {
        yyerror("NULL ast node passed into evalHypotFuncNode!");
        return NAN_RET_VAL; 
    }

    AST_NODE *current = node->data.function.opList;

    if (current == NULL)
    {
        warning("No operands passed into hypot!");
        return ZERO_RET_VAL;
    }

    RET_VAL result;
    result.value = 0.0;
    result.type = DOUBLE_TYPE;

    while (current != NULL) {
        RET_VAL val =  eval(current);
        result.value += pow(val.value, 2.0);
        current = current->next;
    }

    result.value = sqrt(result.value);

    return result;
}

RET_VAL evalMaxFuncNode(AST_NODE *node) {
    if (!node)
    {
        yyerror("NULL ast node passed into evalMaxFuncNode!");
        return NAN_RET_VAL; 
    }

    AST_NODE *current = node->data.function.opList;

    if (current == NULL)
    {
        warning("No operands passed into max!");
        return NAN_RET_VAL;
    }

    RET_VAL result = eval(current);

    while (current->next != NULL) {
        RET_VAL newVal = eval(current->next);

        if (newVal.value > result.value) {
            result = newVal;
        }

        current = current->next;
    }

    return result;
}

RET_VAL evalMinFuncNode(AST_NODE *node) {
    if (!node)
    {
        yyerror("NULL ast node passed into evalMinFuncNode!");
        return NAN_RET_VAL; 
    }

    AST_NODE *current = node->data.function.opList;

    if (current == NULL)
    {
        warning("No operands passed into min!");
        return NAN_RET_VAL;
    }

    RET_VAL result = eval(current);

    while (current->next != NULL) {
        RET_VAL newVal = eval(current->next);

        if (newVal.value < result.value) {
            result = newVal;
        }
        current = current->next;
    }

    return result;
}

RET_VAL evalFuncNode(AST_NODE *node)
{
    if (!node)
    {
        yyerror("NULL ast node passed into evalFuncNode!");
        return NAN_RET_VAL; 
    }

    if (node->type != FUNC_NODE_TYPE)
    {
        yyerror("Incorrect ast node passed into evalFuncNode!");
        return NAN_RET_VAL;
    }

    switch (node->data.function.func)
    {
    case NEG_FUNC:
        return evalNegFuncNode(node);
    case ABS_FUNC:
        return evalAbsFuncNode(node);    
    case ADD_FUNC:
        return evalAddFuncNode(node);    
    case SUB_FUNC:
        return evalSubFuncNode(node);
    case MULT_FUNC:
        return evalMultFuncNode(node);    
    case DIV_FUNC:
        return evalDivFuncNode(node);
    case REM_FUNC:
        return evalRemainderFuncNode(node);
    case EXP_FUNC:
        return evalExpFuncNode(node);
    case EXP2_FUNC:
        return evalExp2FuncNode(node);    
    case POW_FUNC:
        return evalPowFuncNode(node);    
    case LOG_FUNC:
        return evalLogFuncNode(node);
    case SQRT_FUNC:
        return evalSqrtFuncNode(node);    
    case CBRT_FUNC:
        return evalCbrtFuncNode(node);
    case HYPOT_FUNC:
        return evalHypotFuncNode(node);
    case MAX_FUNC:
        return evalMaxFuncNode(node);
    case MIN_FUNC:
        return evalMinFuncNode(node);
    case CUSTOM_FUNC:
        yyerror("Custom func not available yet but called in evalFuncNode!");
    default:
        yyerror("Invalid function type passed into evalFuncNode!");
    }

    // only reach here if default/error case was hit in switch
    return NAN_RET_VAL;
}

RET_VAL evalNumNode(AST_NODE *node)
{
    if (!node)
    {
        yyerror("NULL ast node passed into evalNumNode!");
        return NAN_RET_VAL;
    }

    if (node->type != NUM_NODE_TYPE)
    {
        yyerror("Incorrect ast node passed into evalNumNode!");
        return NAN_RET_VAL;
    }

    return node->data.number;
}

RET_VAL evalSymbolNode(AST_NODE *node)
{
    if (!node)
    {
        yyerror("NULL ast node passed into evalSymbolNode!");
        return NAN_RET_VAL;
    }

    if (node->type != SYM_NODE_TYPE)
    {
        yyerror("Incorrect ast node passed into evalSymbolNode!");
        return NAN_RET_VAL;
    }

    // Search through scopes to find the cloest symbol defintion
    AST_NODE *currentScope = node;

    while (currentScope != NULL) { 
        if (currentScope->symbolTable != NULL) {
            SYMBOL_TABLE_NODE *symbol = currentScope->symbolTable;
            while (symbol != NULL) {
                if (strcmp(symbol->id, node->data.symbol.id) == 0) {
                    return eval(symbol->value);
                }
                symbol = symbol->next;
            }
        }

        // Look further up the tree next iteration
        currentScope = currentScope->parent;
    }

    // Symbol not found
    warning("Undefined symbol: %s", node->data.symbol.id);
    return NAN_RET_VAL;
}

RET_VAL eval(AST_NODE *node)
{
    if (!node)
    {
        yyerror("NULL ast node passed into eval!");
        return NAN_RET_VAL;
    }

    switch (node->type)
    {
    case NUM_NODE_TYPE:
        return evalNumNode(node);
    case FUNC_NODE_TYPE:
        return evalFuncNode(node);
    case SYM_NODE_TYPE:
        return evalSymbolNode(node);
    case SCOPE_NODE_TYPE:
        return eval(node->data.scope.child);
    default:
        yyerror("Incorrect ast node passed into eval!");
    }

    return NAN_RET_VAL;
}

// prints the type and value of a RET_VAL
void printRetVal(RET_VAL val)
{
    switch (val.type)
    {
        case INT_TYPE:
            printf("Integer : %.lf\n", val.value);
            break;
        case DOUBLE_TYPE:
            printf("Double : %lf\n", val.value);
            break;
        default:
            printf("No Type : %lf\n", val.value);
            break;
    }
}


void freeNode(AST_NODE *node)
{
    if (!node)
    {
        return;
    }

    // Free specialized data for each type
    switch (node->type)
    {
    // Function has special oplist data to free
    case FUNC_NODE_TYPE:
        freeNode(node->data.function.opList);
        break;
    
    // Scope node has a child scope to free
    case SCOPE_NODE_TYPE:
        freeNode(node->data.scope.child);
        break;
    
    // Symbol node has an idstring to free
    case SYM_NODE_TYPE:
        free(node->data.symbol.id);
        break;
    
    // Number node has stack allocated numbers which take care of themselves 
    case NUM_NODE_TYPE:
    default:
        break;
    }

    // Free the possible symbol table
    SYMBOL_TABLE_NODE *symbol = node->symbolTable;
    while (symbol != NULL) {
        SYMBOL_TABLE_NODE *next = symbol->next;
        free(symbol->id);
        freeNode(symbol->value);
        free(symbol);
        symbol = next;
    }

    // Free siblings
    if (node->next != NULL) {
        freeNode(node->next);
    }

    // Free this node
    free(node);
}