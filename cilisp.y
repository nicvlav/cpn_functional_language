%{
    #include "cilisp.h"
    #define ylog(r, p) { /*printf("BISON: %s ::= %s \n", #r, #p); */}
    int yylex();
    void yyerror(char*, ...);
%}

%union {
    double dval;
    int ival;
    char *sval;
    struct ast_node *astNode;
    struct symbol_table_node *symbolNode;
};

%token <ival> FUNC TYPE
%token <dval> INT DOUBLE
%token <sval> SYMBOL
%token QUIT EOL EOFT LPAREN RPAREN LET COND LAMBDA

%type <astNode> s_expr f_expr s_expr_section s_expr_list number 
%type <symbolNode> let_section let_list let_elem arg_list

%%

program:
    s_expr EOL {
        ylog(program, s_expr EOL);
        if ($1) {
            printRetVal(eval($1));
            freeNode($1);
        }
        YYACCEPT;
    }
    | s_expr EOFT {
        ylog(program, s_expr EOFT);
        if ($1) {
            printRetVal(eval($1));
            freeNode($1);
        }
        exit(EXIT_SUCCESS);
    }
    | EOL {
        ylog(program, EOL);
        YYACCEPT;  // paranoic; main skips blank lines
    }
    | EOFT {
        ylog(program, EOFT);
        exit(EXIT_SUCCESS);
    };

s_expr:
    LPAREN COND s_expr s_expr s_expr RPAREN  {
        ylog(s_expr, LPAREN COND s_expr s_expr s_expr RPAREN);
        $$ = createCondNode($3, $4, $5); 
    } | f_expr {
        ylog(s_expr, f_expr);
        $$ = $1; 
    } | number {
        ylog(s_expr, number);
        $$ = $1; 
    } | SYMBOL {
        ylog(s_expr, SYMBOL);
        $$ = createSymbolReferenceNode($1);
    } | LPAREN let_section s_expr RPAREN  {
        ylog(s_expr, LPAREN let_section s_expr RPAREN);
        $$ = createScopeNode($2, $3); 
    } | QUIT {
        ylog(s_expr, QUIT);
        exit(EXIT_SUCCESS);
    } | error {
        ylog(s_expr, error);
        yyerror("unexpected token");
        $$ = NULL;
    };
    
f_expr:
    LPAREN FUNC s_expr_section RPAREN  { 
        ylog(f_expr, LPAREN FUNC s_expr_section RPAREN);
        $$ = createCoreFunctionNode($2, $3); 
    } |  LPAREN SYMBOL s_expr_section RPAREN  { 
        ylog(f_expr, LPAREN FUNC s_expr_section RPAREN);
        $$ = createLamdaFunctionNode($2, $3); 
    };

arg_list:
    SYMBOL { 
        ylog(arg_list, SYMBOL);
        $$ = createSymbolArgNode($1);
    } 
     | SYMBOL arg_list {
        ylog(arg_list, SYMBOL arg_list);
        $$ = addSymbolToList(createSymbolArgNode($1), $2);
    }  
    |  {
        ylog(arg_list, );
        $$ = NULL;
    }; 

let_section:
    LPAREN LET let_list RPAREN  { 
        ylog(f_expr, LPAREN FUNC s_expr_section RPAREN);
        $$ = $3; 
    };

let_list:
    let_elem { 
        ylog(let_list, let_elem);
        $$ = $1;
    } | let_elem let_list {
        ylog(let_list, s_expr let_list);
        $$ = addSymbolToList($1, $2);
    };

let_elem:
    LPAREN SYMBOL s_expr RPAREN  { 
        ylog(LPAREN, SYMBOL s_expr RPAREN);
        $$ = createSymbolVarNode($2, $3);
    } | LPAREN TYPE SYMBOL s_expr RPAREN  { 
        ylog(LPAREN, SYMBOL s_expr RPAREN);
        $$ = createTypecastSymbolVarNode($3, $4, $2);
    } | LPAREN SYMBOL LAMBDA LPAREN arg_list RPAREN s_expr RPAREN {
        ylog(LPAREN, PAREN SYMBOL LAMBDA s_expr LPAREN arg_list RPAREN RPAREN);
        $$ = createSymbolLamdaNode($2, $5, $7) ;
    } | LPAREN TYPE SYMBOL LAMBDA LPAREN arg_list RPAREN s_expr RPAREN {
        ylog(LPAREN, PAREN TYPE SYMBOL LAMBDA s_expr LPAREN arg_list RPAREN RPAREN);
        $$ = createTypecastSymbolLamdaNode($3, $6, $8, $2);
    };

s_expr_section:
    s_expr_list { 
        ylog(s_expr_section, s_expr_section);
          $$ = $1;
    } | {
        ylog(s_expr_section, );
        $$ = NULL;
    };

s_expr_list:
    s_expr { 
        ylog(s_expr_list, s_expr);
        $$ = $1;
    } | s_expr s_expr_list {
        ylog(s_expr_list, s_expr s_expr_list);
        $$ = addExpressionToList($1, $2);
    }; 

number:
    INT {
        ylog(number, INT);
        $$ = createNumberNode($1, INT_TYPE);
    };
    | DOUBLE {
        ylog(number, DOUBLE);
        $$ = createNumberNode($1, DOUBLE_TYPE);
    };
%%

