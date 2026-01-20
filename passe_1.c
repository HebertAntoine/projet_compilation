
#include <stdio.h>

#include "defs.h"
#include "passe_1.h"
#include "miniccutils.h"


void analyse_passe_1(node_t root) {

extern int trace_level;

void analyse_passe_1(node_t root) {
    if (!root) return;
    switch (root->nature) {

        case NODE_PROGRAM:
            push_symbol_table();
            analyse_passe_1(root->opr[0]);
            pop_symbol_table();  
            break;

        case NODE_LIST:
            analyse_passe_1(root->opr[0]);
            analyse_passe_1(root->opr[1]);
            break;

        case NODE_DECLS:
            analyse_passe_1(root->opr[0]);
            analyse_passe_1(root->opr[1]);
            break;

        case NODE_DECL:
            analyse_passe_1(root->opr[0]);
            if (symbol_defined_in_current_scope(root->opr[1]->id)) {
                printf("Erreur ligne %d: identificateur \"%s\" red\u00e9fini\n", root->lineno, root->opr[1]->id);
                exit(1);
            }
            symbol_insert(root->opr[1]->id, root->opr[0]->type);
            if (root->opr[2]) analyse_passe_1(root->opr[2]);
            break;

        case NODE_IDENT
            // Utilisation d'un identificateur
            {
                symbol_t *sym = symbol_lookup(root->id);
                if (!sym) {
                    printf("Erreur ligne %d: identificateur \"%s\" non d\u00e9fini\n", root->lineno, root->id);
                    exit(1);
                }
                root->type = sym->type;
            }
            break;

        case NODE_TYPE:
            break;

        case NODE_INTVAL:
            root->type = TYPE_INT;
            break;

        case NODE_BOOLVAL:
            root->type = TYPE_BOOL;
            break;

        case NODE_STRINGVAL:
            root->type = TYPE_STRING;
            break;

        case NODE_FUNC:
            analyse_passe_1(root->opr[0]); // type de retour
            if (symbol_defined_in_current_scope(root->opr[1]->id)) {
                printf("Erreur ligne %d: fonction \"%s\" red\u00e9finie\n", root->lineno, root->opr[1]->id);
                exit(1);
            }
            symbol_insert(root->opr[1]->id, root->opr[0]->type);
            push_symbol_table();
            analyse_passe_1(root->opr[2]);
            analyse_passe_1(root->opr[3]);
            pop_symbol_table();
            break;

        case NODE_IF:
            analyse_passe_1(root->opr[0]);
            if (root->opr[0]->type != TYPE_BOOL) {
                printf("Erreur ligne %d: condition de if non bool\u00e9enne\n", root->lineno);
                exit(1);
            }
            analyse_passe_1(root->opr[1]);
            if (root->opr[2]) analyse_passe_1(root->opr[2]);
            break;

        case NODE_WHILE:
            analyse_passe_1(root->opr[0]);
            if (root->opr[0]->type != TYPE_BOOL) {
                printf("Erreur ligne %d: condition de while non bool\u00e9enne\n", root->lineno);
                exit(1);
            }
            analyse_passe_1(root->opr[1]);
            break;

        case NODE_FOR:
            if (root->opr[0]) analyse_passe_1(root->opr[0]);
            if (root->opr[1]) {
                analyse_passe_1(root->opr[1]);
                if (root->opr[1]->type != TYPE_BOOL) {
                    printf("Erreur ligne %d: condition de for non bool\u00e9enne\n", root->lineno);
                    exit(1);
                }
            }
            if (root->opr[2]) analyse_passe_1(root->opr[2]);
            analyse_passe_1(root->opr[3]);
            break;

        case NODE_DOWHILE:
            analyse_passe_1(root->opr[0]);
            analyse_passe_1(root->opr[1]);
            if (root->opr[1]->type != TYPE_BOOL) {
                printf("Erreur ligne %d: condition de do-while non bool\u00e9enne\n", root->lineno);
                exit(1);
            }
            break;

        case NODE_PRINT:
            analyse_passe_1(root->opr[0]);
            break;

        case NODE_PLUS:
        case NODE_MINUS:
        case NODE_MUL:
        case NODE_DIV:
        case NODE_MOD:
        case NODE_SLL:
        case NODE_SRA:
        case NODE_SRL:
        case NODE_BAND:
        case NODE_BOR:
        case NODE_BXOR:
            analyse_passe_1(root->opr[0]);
            analyse_passe_1(root->opr[1]);
            if (root->opr[0]->type != TYPE_INT || root->opr[1]->type != TYPE_INT) {
                printf("Erreur ligne %d: op\u00e9rande non entier pour %s\n", root->lineno, get_op_name(root->nature));
                exit(1);
            }
            root->type = TYPE_INT;
            break;

        case NODE_LT:
        case NODE_GT:
        case NODE_LE:
        case NODE_GE:
            analyse_passe_1(root->opr[0]);
            analyse_passe_1(root->opr[1]);
            if (root->opr[0]->type != TYPE_INT || root->opr[1]->type != TYPE_INT) {
                printf("Erreur ligne %d: comparateur sur type non entier\n", root->lineno);
                exit(1);
            }
            root->type = TYPE_BOOL;
            break;

        case NODE_EQ:
        case NODE_NE:
            analyse_passe_1(root->opr[0]);
            analyse_passe_1(root->opr[1]);
            if (root->opr[0]->type != root->opr[1]->type) {
                printf("Erreur ligne %d: types incompatibles pour ==/!=\n", root->lineno);
                exit(1);
            }
            root->type = TYPE_BOOL;
            break;

        case NODE_AND:
        case NODE_OR:
            analyse_passe_1(root->opr[0]);
            analyse_passe_1(root->opr[1]);
            if (root->opr[0]->type != TYPE_BOOL || root->opr[1]->type != TYPE_BOOL) {
                printf("Erreur ligne %d: op\u00e9rande non bool\u00e9en pour logique\n", root->lineno);
                exit(1);
            }
            root->type = TYPE_BOOL;
            break;

        case NODE_NOT:
            analyse_passe_1(root->opr[0]);
            if (root->opr[0]->type != TYPE_BOOL) {
                printf("Erreur ligne %d: op\u00e9rande non bool\u00e9en pour NOT\n", root->lineno);
                exit(1);
            }
            root->type = TYPE_BOOL;
            break;

        case NODE_BNOT:
            analyse_passe_1(root->opr[0]);
            if (root->opr[0]->type != TYPE_INT) {
                printf("Erreur ligne %d: op\u00e9rande non entier pour ~\n", root->lineno);
                exit(1);
            }
            root->type = TYPE_INT;
            break;

        case NODE_UMINUS:
            analyse_passe_1(root->opr[0]);
            if (root->opr[0]->type != TYPE_INT) {
                printf("Erreur ligne %d: op\u00e9rande non entier pour unary -\n", root->lineno);
                exit(1);
            }
            root->type = TYPE_INT;
            break;

        case NODE_AFFECT:
            analyse_passe_1(root->opr[0]);
            analyse_passe_1(root->opr[1]);
            if (root->opr[0]->type != root->opr[1]->type) {
                printf("Erreur ligne %d: type gauche/droite incompatibles dans affectation\n", root->lineno);
                exit(1);
            }
            root->type = root->opr[0]->type;
            break;

        default:
            printf("Erreur interne: nœud inattendu de nature %d\n", root->nature);
            exit(1);
    }
}
