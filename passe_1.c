
#include <stdio.h>

#include "defs.h"
#include "passe_1.h"
#include "miniccutils.h"

extern int trace_level;

static context_t current_context = NULL;


void analyse_passe_1(node_t root) {
    if (!root) return;
    switch (root->nature) {

        case NODE_PROGRAM:
            current_context = create_context();
            analyse_passe_1(root->opr[0]);
            analyse_passe_1(root->opr[1]);
            free_context(current_context);
            current_context = NULL;
            break;
            
        case NODE_BLOCK:
            context_t saved = current_context;
            current_context = create_context();
            analyse_passe_1(root->opr[0]);
            analyse_passe_1(root->opr[1]);
            free_context(current_context);
            current_context = saved;
            break;

        case NODE_LIST:
            analyse_passe_1(root->opr[0]);
            analyse_passe_1(root->opr[1]);
            break;

        case NODE_DECLS:
            analyse_passe_1(root->opr[0]); // inutile ?, j'hésite à le supp, à voir XX
            analyse_passe_1(root->opr[1]);
            break;

        case NODE_DECL:
            analyse_passe_1(root->opr[0]);
            if (!context_add_element(current_context, root->opr[1]->ident, root)) {
                printf("Erreur ligne %d: identificateur \"%s\" redéfini\n", root->lineno, root->opr[1]->ident);
                exit(1);
            }

            if (root->opr[2]) {
                analyse_passe_1(root->opr[2]); 
                if (root->opr[0]->type != root->opr[2]->type) {
                    printf("Erreur ligne %d: type incompatible dans l'initialisation\n", root->lineno);
                    exit(1);
                }
            }
            context_add_element(root->opr[1]->ident, root->opr[0]->type); // XX
            break;

        case NODE_IDENT:
            {
                node_t decl = get_data(current_context, root->ident);
                if (!decl) {
                    printf("Erreur ligne %d: identificateur \"%s\" non défini\n", root->lineno, root->ident);
                    exit(1);
                }
                root->type = decl->opr[0]->type;
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
            analyse_passe_1(root->opr[0]); // inutile ?, j'hésite à le supp, à voir XX
            if (!context_add_element(current_context, root->opr[1]->ident, root)) {
                printf("Erreur ligne %d: fonction \"%s\" redéfinie\n", root->lineno, root->opr[1]->ident);
                exit(1);
            }
            context_t saved = current_context;
            current_context = create_context();
            analyse_passe_1(root->opr[2]);
            analyse_passe_1(root->opr[3]);
            free_context(current_context);
            current_context = saved;
            break;

        case NODE_IF:
            analyse_passe_1(root->opr[0]);
            if (root->opr[0]->type != TYPE_BOOL) {
                printf("Erreur ligne %d: condition de if non booléenne\n", root->lineno);
                exit(1);
            }
            analyse_passe_1(root->opr[1]);
            if (root->opr[2]) {
              analyse_passe_1(root->opr[2]);
            }
            break;

        case NODE_WHILE:
            analyse_passe_1(root->opr[0]);
            if (root->opr[0]->type != TYPE_BOOL) {
                printf("Erreur ligne %d: condition de while non booléenne\n", root->lineno);
                exit(1);
            }
            analyse_passe_1(root->opr[1]);
            break;

        case NODE_FOR:
            if (root->opr[0]) {
              analyse_passe_1(root->opr[0]);
            }
            if (root->opr[1]) {
                analyse_passe_1(root->opr[1]);
                if (root->opr[1]->type != TYPE_BOOL) {
                    printf("Erreur ligne %d: condition de for non booléenne\n", root->lineno);
                    exit(1);
                }
            }
            if (root->opr[2]) {
              analyse_passe_1(root->opr[2]);
            }
            analyse_passe_1(root->opr[3]);
            break;

        case NODE_DOWHILE:
            analyse_passe_1(root->opr[0]);
            analyse_passe_1(root->opr[1]);
            if (root->opr[1]->type != TYPE_BOOL) {
                printf("Erreur ligne %d: condition de do-while non booléenne\n", root->lineno);
                exit(1);
            }
            break;

        case NODE_PRINT: {
            node_t p = root->opr[0];
            while (p) {
                node_t param = p->opr[1];
                analyse_passe_1(param);
                if (param->type == TYPE_VOID) {
                    printf("Erreur ligne %d: print d'une expression void\n",
                           param->lineno);
                    exit(1);
                }
                p = p->opr[0];
            }
            root->type = TYPE_VOID;
            break;
        }

        case NODE_PLUS:
        case NODE_MINUS:
        case NODE_MUL:
        case NODE_DIV:
        case NODE_MOD:
        case NODE_SLL:
        case NODE_SRA:
        case NODE_SRL:
            analyse_passe_1(root->opr[0]);
            analyse_passe_1(root->opr[1]);
            if (root->opr[0]->type != TYPE_INT || root->opr[1]->type != TYPE_INT) {
                printf("Erreur ligne %d: opérande non entier pour %s\n", root->lineno, get_op_name(root->nature));
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
                printf("Erreur ligne %d: opérande non booléen pour logique\n", root->lineno);
                exit(1);
            }
            root->type = TYPE_BOOL;
            break;
            
        case NODE_BAND:
        case NODE_BOR:
        case NODE_BXOR:
            analyse_passe_1(root->opr[0]);
            analyse_passe_1(root->opr[1]);
            if (root->opr[0]->type != root->opr[1]->type || (root->opr[0]->type != TYPE_INT && root->opr[0]->type != TYPE_BOOL)) {
                printf("Erreur ligne %d: opérateur binaire incompatible\n", root->lineno);
                exit(1);
            }
            root->type = root->opr[0]->type;
            break;

        case NODE_NOT:
            analyse_passe_1(root->opr[0]);
            if (root->opr[0]->type != TYPE_BOOL) {
                printf("Erreur ligne %d: opérande non booléen pour NOT\n", root->lineno);
                exit(1);
            }
            root->type = TYPE_BOOL;
            break;

        case NODE_BNOT:
            analyse_passe_1(root->opr[0]);
            if (root->opr[0]->type != TYPE_INT) {
                printf("Erreur ligne %d: opérande non entier pour ~\n", root->lineno);
                exit(1);
            }
            root->type = TYPE_INT;
            break;

        case NODE_UMINUS:
            analyse_passe_1(root->opr[0]);
            if (root->opr[0]->type != TYPE_INT) {
                printf("Erreur ligne %d: opérande non entier pour unary -\n", root->lineno);
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
            if (root->opr[0]->nature != NODE_IDENT) {
                printf("Erreur ligne %d: affectation sur une valeur non assignable\n", root->lineno);
                exit(1);
            }
            root->type = root->opr[0]->type;
            break;

        default:
            printf("Erreur interne: nœud inattendu de nature %d\n", root->nature);
            exit(1);
    }
}
