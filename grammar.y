%{
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>

#include "defs.h"
#include "common.h"
#include "miniccutils.h"
#include "passe_1.h"
#include "passe_2.h"



/* Global variables */
extern bool stop_after_syntax;
extern bool stop_after_verif;
extern char * outfile;


/* prototypes */
int yylex(void);
extern int yylineno;

void yyerror(node_t * program_root, char * s);
void analyse_tree(node_t root);
node_t make_node(node_nature nature, int nops, ...);
/* A completer */

// declaration sinon erreur

static node_t make_node_1(node_nature nature, node_t op0);
static node_t make_node_2(node_nature nature, node_t op0, node_t op1);
static node_t make_node_3(node_nature nature, node_t op0, node_t op1, node_t op2);
static node_t make_node_4(node_nature nature, node_t op0, node_t op1, node_t op2, node_t op3);

static node_t noed_LIST(node_t a, node_t b);
static node_t noed_DECLS(node_t type, node_t decl_list);
static node_t noed_INT(void);
static node_t noed_BOOL(void);
static node_t noed_VOID(void);
static node_t noed_COMMA(node_t a, node_t b);
static node_t noed_AFFECT(node_t a, node_t b);
static node_t noed_SEMICAL(node_t e);
static node_t noed_WHILE(node_t cond, node_t body);
static node_t noed_DOWHILE(node_t body, node_t cond);
static node_t noed_PRINT(node_t params);
static node_t noed_LACC(node_t decls, node_t insts);
static node_t noed_INTVAL(int32_t v);
static node_t noed_TRUE(void);
static node_t noed_FALSE(void);
static node_t noed_ident(char *s);
static node_t noed_stringval(char *s);
static node_t noed_type(node_type t);
static node_t noed_LPAR(node_t e);
static char *decode_chaine(const char *s);

%}

%parse-param { node_t * program_root }

%union {
    int32_t intval;
    char * strval;
    node_t ptr;
};


/* Definir les token ici avec leur associativite, dans le bon ordre */
/* A completer */

%token TOK_VOID TOK_INT TOK_BOOL TOK_TRUE TOK_FALSE TOK_IF TOK_DO TOK_WHILE TOK_FOR
%token TOK_PRINT TOK_SEMICOL TOK_COMMA TOK_LPAR TOK_RPAR TOK_LACC TOK_RACC

%nonassoc TOK_THEN
%nonassoc TOK_ELSE
/* a = b = c + d <=> b = c + d; a = b; */
%right TOK_AFFECT
%left TOK_OR
%left TOK_AND
%left TOK_BOR
%left TOK_BXOR
%left TOK_BAND
%nonassoc TOK_EQ TOK_NE
%nonassoc TOK_GT TOK_LT TOK_GE TOK_LE
%nonassoc TOK_SRL TOK_SRA TOK_SLL
/* a / b / c = (a / b) / c et a - b - c = (a - b) - c */
%left TOK_PLUS TOK_MINUS
%left TOK_MUL TOK_DIV TOK_MOD
%nonassoc TOK_UMINUS TOK_NOT TOK_BNOT

%token <intval> TOK_INTVAL;
%token <strval> TOK_IDENT TOK_STRING;
%type <ptr> program listdecl listdeclnonnull vardecl ident type listtypedecl decl maindecl
%type <ptr> listinst listinstnonnull inst block expr listparamprint paramprint


%%

/* Completer les regles et la creation de l'arbre */
program:
    listdeclnonnull maindecl
    {
        $$ = make_node(NODE_PROGRAM, 2, $1, $2);
        *program_root = $$;
    }
    | maindecl
    {
        $$ = make_node(NODE_PROGRAM, 2, NULL, $1);
        *program_root = $$;
    }
    ;

/*listdeclnonnull:
            { $$ = NULL; }
        ;

maindecl:
            { $$ = NULL; }
        ;
*/
listdecl : listdeclnonnull                  { $$ = $1; }
        |                                   { $$ = NULL; }
        ;

listdeclnonnull : vardecl                   { $$ = $1; }
        | listdeclnonnull vardecl           { $$ = noed_LIST($1, $2); }
        ;

vardecl : type listtypedecl TOK_SEMICOL     { $$ = noed_DECLS($1, $2); }
        ;

type    : TOK_INT                           { $$ = noed_INT(); }
        | TOK_BOOL                          { $$ = noed_BOOL(); }
        | TOK_VOID                          { $$ = noed_VOID(); }
        ;

listtypedecl : decl                         { $$ = $1; }
        | listtypedecl TOK_COMMA decl       { $$ = noed_COMMA($1, $3); }
        ;

decl    : ident                             { $$ = $1; }
        | ident TOK_AFFECT expr             { $$ = noed_AFFECT($1, $3); }
        ;

maindecl : type ident TOK_LPAR TOK_RPAR block
        { $$ = make_node_3(NODE_FUNC, $1, $2, $5); }
        ;

listinst : listinstnonnull                  { $$ = $1; }
        |                                   { $$ = NULL; }
        ;

listinstnonnull
        : inst                              { $$ = $1; }
        | listinstnonnull inst              { $$ = noed_LIST($1, $2); }
        ;

inst    : expr TOK_SEMICOL                                                      { $$ = noed_SEMICAL($1); }
        | TOK_IF TOK_LPAR expr TOK_RPAR inst TOK_ELSE inst                      { $$ = make_node_3(NODE_IF, $3, $5, $7); }
        | TOK_IF TOK_LPAR expr TOK_RPAR inst %prec TOK_THEN                     { $$ = make_node_2(NODE_IF, $3, $5); }
        | TOK_WHILE TOK_LPAR expr TOK_RPAR inst                                 { $$ = noed_WHILE($3, $5); }
        | TOK_FOR TOK_LPAR expr TOK_SEMICOL expr TOK_SEMICOL expr TOK_RPAR inst { $$ = make_node_4(NODE_FOR, $3, $5, $7, $9); }
        | TOK_DO inst TOK_WHILE TOK_LPAR expr TOK_RPAR TOK_SEMICOL              { $$ = noed_DOWHILE($2, $5); }
        | block                                                                 { $$ = $1; }
        | TOK_SEMICOL                                                           { $$ = NULL; }
        | TOK_PRINT TOK_LPAR listparamprint TOK_RPAR TOK_SEMICOL                { $$ = noed_PRINT($3); }
        ;

block   : TOK_LACC listdecl listinst TOK_RACC { $$ = noed_LACC($2, $3); }
        ;

expr    : expr TOK_MUL expr         { $$ = make_node_2(NODE_MUL, $1, $3); }
        | expr TOK_DIV expr         { $$ = make_node_2(NODE_DIV, $1, $3); }
        | expr TOK_PLUS expr        { $$ = make_node_2(NODE_PLUS, $1, $3); }
        | expr TOK_MINUS expr       { $$ = make_node_2(NODE_MINUS, $1, $3); }
        | expr TOK_MOD expr         { $$ = make_node_2(NODE_MOD, $1, $3); }
        | expr TOK_LT expr          { $$ = make_node_2(NODE_LT, $1, $3); }
        | expr TOK_GT expr          { $$ = make_node_2(NODE_GT, $1, $3); }
        | TOK_MINUS expr %prec TOK_UMINUS { $$ = make_node_1(NODE_UMINUS, $2); }
        | expr TOK_GE expr          { $$ = make_node_2(NODE_GE, $1, $3); }
        | expr TOK_LE expr          { $$ = make_node_2(NODE_LE, $1, $3); }
        | expr TOK_EQ expr          { $$ = make_node_2(NODE_EQ, $1, $3); }
        | expr TOK_NE expr          { $$ = make_node_2(NODE_NE, $1, $3); }
        | expr TOK_AND expr         { $$ = make_node_2(NODE_AND, $1, $3); }
        | expr TOK_OR expr          { $$ = make_node_2(NODE_OR, $1, $3); }
        | expr TOK_BAND expr        { $$ = make_node_2(NODE_BAND, $1, $3); }
        | expr TOK_BOR expr         { $$ = make_node_2(NODE_BOR, $1, $3); }
        | expr TOK_BXOR expr        { $$ = make_node_2(NODE_BXOR, $1, $3); }
        | expr TOK_SRL expr         { $$ = make_node_2(NODE_SRL, $1, $3); }
        | expr TOK_SRA expr         { $$ = make_node_2(NODE_SRA, $1, $3); }
        | expr TOK_SLL expr         { $$ = make_node_2(NODE_SLL, $1, $3); }
        | TOK_NOT expr              { $$ = make_node_1(NODE_NOT, $2); }
        | TOK_BNOT expr             { $$ = make_node_1(NODE_BNOT, $2); }
        | TOK_LPAR expr TOK_RPAR    { $$ = noed_LPAR($2); }
        | ident TOK_AFFECT expr     { $$ = noed_AFFECT($1, $3); }
        | TOK_INTVAL                { $$ = noed_INTVAL($1); }
        | TOK_TRUE                  { $$ = noed_TRUE(); }
        | TOK_FALSE                 { $$ = noed_FALSE(); }
        | ident                     { $$ = $1; }
        ;

listparamprint : listparamprint TOK_COMMA paramprint { $$ = noed_LIST($1, $3); }
        | paramprint { $$ = $1; }
        ;

paramprint : ident { $$ = $1; }
        | TOK_STRING { $$ = noed_stringval($1); }
        ;

ident   : TOK_IDENT { $$ = noed_ident($1); }
        ;
%%

/* A completer et/ou remplacer avec d'autres fonctions */
node_t make_node(node_nature nature, int nops, ...){
    node_t n = malloc(sizeof(node_s));
    va_list ap;
    n->nature = nature;
    n->type = TYPE_NONE;
    n->value = 0;
    n->offset = 0;
    n->global_decl = 0;
    n->lineno = yylineno;
    n->nops = nops;
    n->opr = (nops > 0) ? malloc(sizeof(node_t) * nops) : NULL;
    va_start(ap, nops);
    for (int i = 0; i < nops; i++) {
        n->opr[i] = va_arg(ap, node_t);
    }
    va_end(ap);

    return n;
}


// cree l'architecture de l'arbre 

static node_t noed_LIST(node_t a, node_t b) { return make_node(NODE_LIST, 2, a, b); }
static node_t noed_DECLS(node_t type, node_t decl_list) { return make_node(NODE_DECLS, 2, type, decl_list); }
static node_t noed_COMMA(node_t a, node_t b) { return make_node(NODE_LIST, 2, a, b); }
static node_t noed_AFFECT(node_t a, node_t b) { return make_node(NODE_AFFECT, 2, a, b); }
static node_t noed_SEMICAL(node_t e) { return make_node(NODE_LIST, 1, e); }
static node_t noed_WHILE(node_t cond, node_t body) { return make_node(NODE_WHILE, 2, cond, body); }
static node_t noed_DOWHILE(node_t body, node_t cond) { return make_node(NODE_DOWHILE, 2, body, cond); }
static node_t noed_PRINT(node_t params) { return make_node(NODE_PRINT, 1, params); }
static node_t noed_LACC(node_t decls, node_t insts) { return make_node(NODE_BLOCK, 2, decls, insts); }

static node_t noed_ident(char *s) { node_t n = make_node(NODE_IDENT, 0); n->ident = s; return n; }
static node_t noed_type(node_type t) { node_t n = make_node(NODE_TYPE, 0); n->type = t; return n; }
static node_t noed_INT(void) { return noed_type(TYPE_INT); }
static node_t noed_BOOL(void) { return noed_type(TYPE_BOOL); }
static node_t noed_VOID(void) { return noed_type(TYPE_VOID); }
static node_t noed_INTVAL(int32_t v) { node_t n = make_node(NODE_INTVAL, 0); n->value = v; return n; }
static node_t noed_TRUE(void) { node_t n = make_node(NODE_BOOLVAL, 0); n->value = 1; return n; }
static node_t noed_FALSE(void) { node_t n = make_node(NODE_BOOLVAL, 0); n->value = 0; return n; }
static node_t noed_stringval(char *s) { node_t n = make_node(NODE_STRINGVAL, 0); n->str = s; return n; }
static node_t noed_LPAR(node_t e) { return e; }

static node_t make_node_1(node_nature nature, node_t op0) {
    node_t n = malloc(sizeof(node_s));
// NODE_NOT, NODE_BNOT, NODE_UMINUS, NODE_PRINT
    n->nature = nature;           
    n->value = 0;
    n->offset = 0;
    n->global_decl = 0;
    n->type = TYPE_NONE;

    n->node_num = 0;
    n->lineno = yylineno;

    n->nops = 1;
    n->opr = malloc(sizeof(node_t) * 1);
    n->opr[0] = op0;

    return n;
}

static node_t make_node_2(node_nature nature, node_t op0, node_t op1) {
    node_t n = malloc(sizeof(node_s));

// NODE_MUL, NODE_DIV, NODE_PLUS, NODE_MINUS, NODE_MOD, NODE_LT, NODE_GT, NODE_LE, NODE_GE, NODE_EQ, NODE_NE, NODE_AND, NODE_OR, NODE_BAND, NODE_BOR, NODE_BXOR, NODE_SLL, NODE_SRA, NODE_SRL

    n->nature = nature;           
    n->value = 0;
    n->offset = 0;
    n->global_decl = 0;
    n->type = TYPE_NONE;
    
    n->node_num = 0;
    n->lineno = yylineno;
    n->nops = 2;
    n->opr = malloc(sizeof(node_t) * 2);
    n->opr[0] = op0;
    n->opr[1] = op1;

    return n;
}

static node_t make_node_3(node_nature nature, node_t op0, node_t op1, node_t op2) {
    node_t n = malloc(sizeof(node_s));

    n->nature = nature;
    n->value = 0;
    n->offset = 0;
    n->global_decl = 0;
    n->type = TYPE_NONE;

    n->node_num = 0;
    n->lineno = yylineno;
    n->nops = 3;
    n->opr = malloc(sizeof(node_t) * 3);
    n->opr[0] = op0;
    n->opr[1] = op1;
    n->opr[2] = op2;

    return n;
}

static node_t make_node_4(node_nature nature, node_t op0, node_t op1, node_t op2, node_t op3) {
    node_t n = malloc(sizeof(node_s));

    n->nature = nature;
    n->value = 0;
    n->offset = 0;
    n->global_decl = 0;
    n->type = TYPE_NONE;

    n->node_num = 0;
    n->lineno = yylineno;

    n->nops = 4;
    n->opr = malloc(sizeof(node_t) * 4);
    n->opr[0] = op0;
    n->opr[1] = op1;
    n->opr[2] = op2;
    n->opr[3] = op3;

    return n;
}
/*
void analyse_tree(node_t root) {
    dump_tree(root, "apres_syntaxe.dot");
    if (!stop_after_syntax) {
        analyse_passe_1(root);
        //dump_tree(root, "apres_passe_1.dot");
        if (!stop_after_verif) {
            create_program(); 
            gen_code_passe_2(root);
            dump_mips_program(outfile);
            free_program();
        }
        free_global_strings();
    }
    free_nodes(root);
}
*/

void analyse_tree(node_t root) {
    if (!root) return;

    dump_tree(root, "apres_syntaxe.dot");

    if (stop_after_syntax) {
        free_nodes(root);
        return;
    }

    analyse_passe_1(root);

    if (!stop_after_verif) {
        create_program();
        gen_code_passe_2(root);
        dump_mips_program(outfile);
        free_program();
    }

    free_global_strings();
    free_nodes(root);
}



/* Cette fonction est appelee automatiquement si une erreur de syntaxe est rencontree
 * N'appelez pas cette fonction vous-meme :  
 * la valeur donnee par yylineno ne sera plus correcte apres l'analyse syntaxique
 */
void yyerror(node_t * program_root, char * s) {
    fprintf(stderr, "Error line %d: %s\n", yylineno, s);
    exit(1);
}

