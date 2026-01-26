
#include <stdio.h>
#include <stdlib.h>

#include "defs.h"
#include "passe_1.h"
#include "miniccutils.h"


static node_type type_op_unaire(node_nature op, node_type t) {
    switch (op) {
        case NODE_UMINUS: return (t == TYPE_INT)  ? TYPE_INT  : TYPE_NONE;
        case NODE_BNOT:   return (t == TYPE_INT)  ? TYPE_INT  : TYPE_NONE;
        case NODE_NOT:    return (t == TYPE_BOOL) ? TYPE_BOOL : TYPE_NONE;
        default:          return TYPE_NONE;
    }
}

static node_type type_op_binaire(node_nature op, node_type t1, node_type t2) {
    if (t1 == TYPE_INT && t2 == TYPE_INT) {
        switch (op) {
            case NODE_PLUS: case NODE_MINUS: case NODE_MUL: case NODE_DIV: case NODE_MOD:
            case NODE_BAND: case NODE_BOR: case NODE_BXOR:
            case NODE_SLL:  case NODE_SRL:  case NODE_SRA:
                return TYPE_INT;

            case NODE_EQ: case NODE_NE:
            case NODE_LT: case NODE_GT: case NODE_LE: case NODE_GE:
                return TYPE_BOOL;

            default:
                return TYPE_NONE;
        }
    }

    if (t1 == TYPE_BOOL && t2 == TYPE_BOOL) {
        switch (op) {
            case NODE_AND: case NODE_OR:
            case NODE_EQ:  case NODE_NE:
                return TYPE_BOOL;
            default:
                return TYPE_NONE;
        }
    }

    return TYPE_NONE;
}


void analyse_passe_1(node_t root) {
    if (!root) return;

    switch (root->nature) {

        case NODE_PROGRAM:
            push_global_context();
            analyse_passe_1(root->opr[0]);
            analyse_passe_1(root->opr[1]);
            pop_context();
            break;


        case NODE_BLOCK:
            push_context();
            analyse_passe_1(root->opr[0]);
            analyse_passe_1(root->opr[1]);
            pop_context();
            break;

        case NODE_LIST:
            if (root->nops > 0 && root->opr[0]) analyse_passe_1(root->opr[0]);
            if (root->nops > 1 && root->opr[1]) analyse_passe_1(root->opr[1]);
            break;

        case NODE_DECLS: {
            node_type t = root->opr[0]->type;
            node_t p = root->opr[1];

            while (p) {
                node_t elt;

                if (p->nature == NODE_LIST) {
                    elt = p->opr[1];
                    p   = p->opr[0];
                } else {
                    elt = p;
                    p   = NULL;
                }

                if (elt->nature == NODE_IDENT) {
                    elt->type = t;
                    if (!context_add_element(current_context, elt->ident, elt)) {
                        printf("Erreur ligne %d: identificateur \"%s\" redéfini\n",
                               elt->lineno, elt->ident);
                        exit(1);
                    }
                }
                else if (elt->nature == NODE_AFFECT) {
                    node_t id = elt->opr[0];
                    node_t ex = elt->opr[1];

                    if (id->nature != NODE_IDENT) {
                        printf("Error line %d: déclaration invalide\n", elt->lineno);
                        exit(1);
                    }

                    id->type = t;
                    if (!context_add_element(current_context, id->ident, id)) {
                        printf("Error line %d: identificateur \"%s\" redéfini\n",
                               id->lineno, id->ident);
                        exit(1);
                    }

                    analyse_passe_1(ex);
                    if (ex->type != t) {
                        printf("Error line %d: type incompatible dans l'initialisation\n",
                               elt->lineno);
                        exit(1);
                    }

                    elt->type = t;
                }
                else {
                    printf("Error line %d: élément inattendu dans déclaration\n", elt->lineno);
                    exit(1);
                }
            }
            break;
        }

        case NODE_IDENT: {
            node_t decl = get_decl_node(root->ident);
            if (!decl) {
                printf("Error line %d: identificateur \"%s\" non défini\n",
                       root->lineno, root->ident);
                exit(1);
            }
            root->type = decl->type;
            break;
        }

        case NODE_TYPE:
            break;
        case NODE_INTVAL:
            root->type = TYPE_INT;
            break;
        case NODE_BOOLVAL:
            root->type = TYPE_BOOL;
            break;
        case NODE_STRINGVAL:
            root->type = TYPE_NONE;
            break;

        case NODE_FUNC:
            push_context();
            analyse_passe_1(root->opr[2]);
            pop_context();
            break;

        case NODE_IF:
            analyse_passe_1(root->opr[0]);
            if (root->opr[0]->type != TYPE_BOOL) {
                printf("Error line %d: condition de if non booléenne\n", root->lineno);
                exit(1);
            }
            analyse_passe_1(root->opr[1]);
            if (root->nops > 2 && root->opr[2]) {
                analyse_passe_1(root->opr[2]);
            }
            break;

        case NODE_WHILE:
            analyse_passe_1(root->opr[0]);
            if (root->opr[0]->type != TYPE_BOOL) {
                printf("Error line %d: condition de while non booléenne\n", root->lineno);
                exit(1);
            }
            analyse_passe_1(root->opr[1]);
            break;

        case NODE_FOR:
            if (root->opr[0]) analyse_passe_1(root->opr[0]);
            if (root->opr[1]) {
                analyse_passe_1(root->opr[1]);
                if (root->opr[1]->type != TYPE_BOOL) {
                    printf("Error line %d: condition de for non booléenne\n", root->lineno);
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
                printf("Error line %d: condition de do-while non booléenne\n", root->lineno);
                exit(1);
            }
            break;

        case NODE_PRINT: {
            node_t p = root->opr[0];
            while (p) {
                node_t param = p->opr[1];
                analyse_passe_1(param);
                if (param->type == TYPE_VOID) {
                    printf("Error line %d: print d'une expression void\n",
                           param->lineno);
                    exit(1);
                }
                p = p->opr[0];
            }
            root->type = TYPE_VOID;
            break;
        }

        case NODE_NOT:
        case NODE_BNOT:
        case NODE_UMINUS:
            analyse_passe_1(root->opr[0]);
            root->type = type_op_unaire(root->nature, root->opr[0]->type);
            if (root->type == TYPE_NONE) {
                printf("Error line %d: opérateur unaire incompatible\n", root->lineno);
                exit(1);
            }
            break;

        case NODE_PLUS: case NODE_MINUS: case NODE_MUL: case NODE_DIV: case NODE_MOD:
        case NODE_LT: case NODE_GT: case NODE_LE: case NODE_GE:
        case NODE_EQ: case NODE_NE:
        case NODE_AND: case NODE_OR:
        case NODE_BAND: case NODE_BOR: case NODE_BXOR:
        case NODE_SLL: case NODE_SRA: case NODE_SRL:
            analyse_passe_1(root->opr[0]);
            analyse_passe_1(root->opr[1]);

            root->type = type_op_binaire(root->nature,
                                         root->opr[0]->type,
                                         root->opr[1]->type);
            if (root->type == TYPE_NONE) {
                printf("Error line %d: opérateur binaire incompatible\n", root->lineno);
                exit(1);
            }
            break;

        case NODE_AFFECT:
            analyse_passe_1(root->opr[0]);
            analyse_passe_1(root->opr[1]);

            if (root->opr[0]->nature != NODE_IDENT) {
                printf("Error line %d: affectation sur une valeur non assignable\n", root->lineno);
                exit(1);
            }
            if (root->opr[0]->type != root->opr[1]->type) {
                printf("Error line %d: type gauche/droite incompatibles dans affectation\n", root->lineno);
                exit(1);
            }

            root->type = root->opr[0]->type;
            break;

        default:
            printf("Erreur interne: nœud inattendu de nature %d\n", root->nature);
            exit(1);
    }
}
