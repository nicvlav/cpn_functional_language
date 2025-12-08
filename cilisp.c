#include "cilisp.h"

#define RED             "\033[31m"
#define RESET_COLOR     "\033[0m"

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

    node->data.number.type = type;
    node->data.number.value = value;
    node->type = NUM_NODE_TYPE;

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
        yyerror("No op data passed into evalNegFuncNode");
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
        yyerror("No operands passed into evalAddFuncNode!");
        return NAN_RET_VAL;
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
        yyerror("NULL ast node passed into evalSubFuncNode!");
        return NAN_RET_VAL; 
    }

    AST_NODE *current = node->data.function.opList;

    if (current == NULL)
    {
        yyerror("No operands passed into evalSubFuncNode!");
        return NAN_RET_VAL;
    }

    RET_VAL result = eval(current);

    while (current->next != NULL) {
        RET_VAL newVal = eval(current->next);

        // convert overall type to double if there is any double operand
        if (result.type == INT_TYPE && newVal.type == DOUBLE_TYPE) {
            result.type = DOUBLE_TYPE;
        }

        result.value -= newVal.value;
        current = current->next;
    }

    return result;
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
        yyerror("No operands passed into evalMultFuncNode!");
        return NAN_RET_VAL;
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

    AST_NODE *current = node->data.function.opList;

    if (current == NULL)
    {
        yyerror("No operands passed into evalDivFuncNode!");
        return NAN_RET_VAL;
    }

    RET_VAL result = eval(current);

    while (current->next != NULL) {
        RET_VAL newVal = eval(current->next);

        // convert overall type to double if there is any double operand
        if (result.type == INT_TYPE && newVal.type == DOUBLE_TYPE) {
            result.type = DOUBLE_TYPE;
        }

        result.value /= newVal.value;
        current = current->next;
    }

    // Dividing an integer by and integer can still give decimal points
    if (result.type == INT_TYPE) {
        result.value = round(result.value);
    }

    return result;
}

RET_VAL evalRemainderFuncNode(AST_NODE *node) {
    if (!node)
    {
        yyerror("NULL ast node passed into evalRemainderFuncNode!");
        return NAN_RET_VAL; 
    }

    AST_NODE *left = node->data.function.opList;

    if (left == NULL)
    {
        yyerror("No operands passed into evalRemainderFuncNode!");
        return NAN_RET_VAL;
    }

    AST_NODE *right = left->next;

    if (right == NULL)
    {
        yyerror("Only one operand passed into evalRemainderFuncNode!");
        return NAN_RET_VAL;
    }

    RET_VAL result;

    result.value = fmod(left->data.number.value, right->data.number.value);

    if (left->data.number.type == DOUBLE_TYPE || right->data.number.type == DOUBLE_TYPE ) {
        result.type = DOUBLE_TYPE;
    } else {
        result.type = INT_TYPE;
        result.value = round(result.value);
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

    // TODO complete the function

    // look through the AST_NODE struct, decide what
    // referenced data should have freeNode called on it
    // (hint: it might be part of an s_expr_list, with more
    // nodes following it in the list)

    // if this node is FUNC_TYPE, it might have some operands
    // to free as well (but this should probably be done in
    // a call to another function, named something like
    // freeFunctionNode)

    // and, finally,
    free(node);
}
