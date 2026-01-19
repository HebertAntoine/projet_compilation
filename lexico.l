%{
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "defs.h"
#include "common.h"

#include "y.tab.h"

int yyparse(node_t * program_root);
void analyse_tree(node_t root);


char * infile = NULL;
char * outfile = DEFAULT_OUTFILE;
bool stop_after_syntax = false;
bool stop_after_verif = false;

#if YYDEBUG
extern int yydebug;
#endif

#if LEX_DEBUG
#define RETURN(token) ({ printf("%s \t\"%s\"\n", #token, yytext); return token; })
#else
#define RETURN(token) ({ return token; })
#endif


%}

%option yylineno


LETTRE          [a-zA-Z]
/* A completer ; fausses definitions pour passer la compilation 
IDF             A
ENTIER          0
CHAINE          B
COMMENTAIRE     C

*/ 

%%

"void"          RETURN(TOK_VOID);
"int"           RETURN(TOK_INT);
"bool"          RETURN(TOK_BOOL);
"true"          RETURN(TOK_TRUE);
"false"         RETURN(TOK_FALSE);
"if"            RETURN(TOK_IF);
"else"          RETURN(TOK_ELSE);
"while"         RETURN(TOK_WHILE);
"for"           RETURN(TOK_FOR);
"do"            RETURN(TOK_DO);
"print"         RETURN(TOK_PRINT);

LETTRE          [a-z];
CHIFFRE         [0-9];
IDF             {LETTRE}({LETTRE}|{CHIFFRE}| )*


CHIFFRE_NON_NUL = {’1’, ..., ’9’}
ENTIER_DEC = ’0’ | CHIFFRE_NON_NUL CHIFFRE*
LETTRE_HEXA = {’a’, ..., ’f’, ’A’, ..., ’F’}
ENTIER_HEXA = ’0x’ (CHIFFRE | LETTRE_HEXA)+
ENTIER = ENTIER_DEC | ENTIER_HEXA

WS              [ \t\r\n]+
COMMENT         "//"[^\n]*
STRING_CHAR     [ -!#-\[\]-~]
ESCAPE_SEQ      \\\"|\\n|\\\\
STRING          \"({STRING_CHAR}|{ESCAPE_SEQ})*\"


"+"             RETURN(TOK_PLUS);
"-"             RETURN(TOK_MINUS);
"*"             RETURN(TOK_MUL);
"/"             RETURN(TOK_DIV);
"%"             RETURN(TOK_MOD);
">"             RETURN(TOK_GT);
"<"             RETURN(TOK_LT);
">="            RETURN(TOK_GE);
"<="            RETURN(TOK_LE);
"=="            RETURN(TOK_EQ);
"!="            RETURN(TOK_NE);
"!"             RETURN(TOK_NOT);
"~"             RETURN(TOK_BNOT);
"&"             RETURN(TOK_BAND);
"|"             RETURN(TOK_BOR);
"^"             RETURN(TOK_BXOR);
"&&"            RETURN(TOK_AND);
"||"            RETURN(TOK_OR);
"="             RETURN(TOK_AFFECT);
";"             RETURN(TOK_SEMICOL);
","             RETURN(TOK_COMMA);
"("             RETURN(TOK_LPAR);
")"             RETURN(TOK_RPAR);
"{"             RETURN(TOK_LACC);
"}"             RETURN(TOK_RACC);
">>>"           RETURN(TOK_SRA);
">>"            RETURN(TOK_SRL);
"<<"            RETURN(TOK_SLL);

{IDF} {
                #if !LEX_DEBUG
                yylval.strval = strdupl(yytext);
                #endif
                RETURN(TOK_IDENT);
}

{ENTIER} {
                #if !LEX_DEBUG
                yylval.intval = (int) strtol(yytext, NULL, 0);
                #endif
                RETURN(TOK_INTVAL);
}

{CHAINE} {
                #if !LEX_DEBUG
                yylval.strval = decode_chaine(yytext);
                #endif
                RETURN(TOK_STRING);
}

{COMMENTAIRE} {
}


[ \t\n]+

. {
                fprintf(stderr, "Error line %d: Lexical error\n", yylineno);
                exit(1);
}


%%

int yywrap(void) {
    return 1;
}


int main(int argc, char ** argv) {
    node_t program_root;
    parse_args(argc, argv);
    yyin = fopen(infile, "r");
    #if LEX_DEBUG
        while(yylex());
        fclose(yyin);
    #else
        #if YYDEBUG
        yydebug = 1;
        #endif
        yyparse(&program_root);
        fclose(yyin);
        analyse_tree(program_root);
    #endif
    yylex_destroy();
    return 0;
}



