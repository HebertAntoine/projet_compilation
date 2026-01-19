
#include <stdio.h>

#include "defs.h"
#include "passe_1.h"
#include "miniccutils.h"


extern int trace_level;

void analyse_passe_1(node_t root) {
    if (!root) return;
    switch (root->nature) {

        case NODE_PROGRAM:
            // Entrée dans un nouveau scope (programme global)
            push_symbol_table();  
            // Par exemple, le premier fils peut être la liste de déclarations
            analyse_passe_1(root->opr[0]);
            pop_symbol_table();  
            break;

        case NODE_LIST:
            // Liste chaînée (de déclarations ou d'instructions)
            analyse_passe_1(root->opr[0]);
            analyse_passe_1(root->opr[1]);
            break;

        case NODE_DECLS:
            // Suite de déclarations
            analyse_passe_1(root->opr[0]);
            analyse_passe_1(root->opr[1]);
            break;

        case NODE_DECL:
            // Déclaration de variable ou de fonction
            // Supposons: opr[0] = type, opr[1] = ident, opr[2] = éventuellement initialisation / corps
            analyse_passe_1(root->opr[0]);       // analyser le type
            // root->opr[1] est le nom (NODE_IDENT)
            if (symbol_defined_in_current_scope(root->opr[1]->id)) {
                printf("Erreur ligne %d: identificateur \"%s\" red\u00e9fini\n", root->lineno, root->opr[1]->id);
                exit(1);
            }
            // Insérer dans la table des symboles
            symbol_insert(root->opr[1]->id, root->opr[0]->type);
            // Initialisation éventuelle
            if (root->opr[2]) analyse_passe_1(root->opr[2]);
            break;

        case NODE_IDENT:
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
            // Nœud représentant un type (ex: int, bool); 
            // on peut le traiter selon la grammaire, ou ne rien faire car c'est juste un marqueur.
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
            // Déclaration/definition de fonction : nom, paramètres, corps
            // opr[0]=type de retour, opr[1]=ident (nom), opr[2]=params, opr[3]=body
            analyse_passe_1(root->opr[0]); // type de retour
            // Insérer nom de la fonction dans la table globale
            if (symbol_defined_in_current_scope(root->opr[1]->id)) {
                printf("Erreur ligne %d: fonction \"%s\" red\u00e9finie\n", root->lineno, root->opr[1]->id);
                exit(1);
            }
            symbol_insert(root->opr[1]->id, root->opr[0]->type);
            // Nouveau scope pour le corps de la fonction
            push_symbol_table();
            // Gestion des paramètres : ajouter chaque paramètre à la table des symboles
            // (Supposons root->opr[2] est une liste de déclarations de params)
            analyse_passe_1(root->opr[2]);
            // Analyser le corps de la fonction
            analyse_passe_1(root->opr[3]);
            pop_symbol_table();
            break;

        case NODE_IF:
            // opr[0]=condition, opr[1]=then-branch, opc[2]=else-branch (optionnel)
            analyse_passe_1(root->opr[0]);
            if (root->opr[0]->type != TYPE_BOOL) {
                printf("Erreur ligne %d: condition de if non bool\u00e9enne\n", root->lineno);
                exit(1);
            }
            analyse_passe_1(root->opr[1]);
            if (root->opr[2]) analyse_passe_1(root->opr[2]);
            break;

        case NODE_WHILE:
            // opr[0]=condition, opr[1]=body
            analyse_passe_1(root->opr[0]);
            if (root->opr[0]->type != TYPE_BOOL) {
                printf("Erreur ligne %d: condition de while non bool\u00e9enne\n", root->lineno);
                exit(1);
            }
            analyse_passe_1(root->opr[1]);
            break;

        case NODE_FOR:
            // opr[0]=init, opr[1]=condition, opr[2]=increment, opr[3]=body
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
            // opr[0]=body, opr[1]=condition
            analyse_passe_1(root->opr[0]);
            analyse_passe_1(root->opr[1]);
            if (root->opr[1]->type != TYPE_BOOL) {
                printf("Erreur ligne %d: condition de do-while non bool\u00e9enne\n", root->lineno);
                exit(1);
            }
            break;

        case NODE_PRINT:
            // opr[0]=expression à imprimer
            analyse_passe_1(root->opr[0]);
            // (On peut accepter n'importe quel type pour print, ou restreindre int/stri...)
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
            // Opérateurs binaires arithmétiques ou bitwise (entiers)
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
            // Opérateurs de comparaison (int, résultat bool)
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
            // Egalité/inégalité : on peut imposer mêmes types (ici int ou bool, selon le langage)
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
            // Opérateurs logiques (bool)
            analyse_passe_1(root->opr[0]);
            analyse_passe_1(root->opr[1]);
            if (root->opr[0]->type != TYPE_BOOL || root->opr[1]->type != TYPE_BOOL) {
                printf("Erreur ligne %d: op\u00e9rande non bool\u00e9en pour logique\n", root->lineno);
                exit(1);
            }
            root->type = TYPE_BOOL;
            break;

        case NODE_NOT:
            // Opérateur unaire logique
            analyse_passe_1(root->opr[0]);
            if (root->opr[0]->type != TYPE_BOOL) {
                printf("Erreur ligne %d: op\u00e9rande non bool\u00e9en pour NOT\n", root->lineno);
                exit(1);
            }
            root->type = TYPE_BOOL;
            break;

        case NODE_BNOT:
            // Opérateur unaire bitwise
            analyse_passe_1(root->opr[0]);
            if (root->opr[0]->type != TYPE_INT) {
                printf("Erreur ligne %d: op\u00e9rande non entier pour ~\n", root->lineno);
                exit(1);
            }
            root->type = TYPE_INT;
            break;

        case NODE_UMINUS:
            // Négation unaire arithmétique
            analyse_passe_1(root->opr[0]);
            if (root->opr[0]->type != TYPE_INT) {
                printf("Erreur ligne %d: op\u00e9rande non entier pour unary -\n", root->lineno);
                exit(1);
            }
            root->type = TYPE_INT;
            break;

        case NODE_AFFECT:
            // Affection (opr[0]=lvalue, opr[1]=expr à droite)
            analyse_passe_1(root->opr[0]);
            analyse_passe_1(root->opr[1]);
            if (root->opr[0]->type != root->opr[1]->type) {
                printf("Erreur ligne %d: type gauche/droite incompatibles dans affectation\n", root->lineno);
                exit(1);
            }
            root->type = root->opr[0]->type;
            break;

        // Ajouter d'autres nœuds selon la grammaire simplifiée...

        default:
            // Cas non gérés (devrait être exhaustif)
            printf("Erreur interne: nœud inattendu de nature %d\n", root->nature);
            exit(1);
    }
}
