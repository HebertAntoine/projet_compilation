
#include <stdio.h>

#include "defs.h"
#include "passe_2.h"
#include "miniccutils.h"

extern int trace_level;



void gen_code_passe_2(node_t root) {
    if (!root) return;

    switch (root->nature) {

        case NODE_PROGRAM:
            create_program();
            create_data_sec_inst();
            create_text_sec_inst();
            
            reset_temporary_max_offset();
            create_stack_allocation_inst();

            gen_code_passe_2(root->opr[0]);
            gen_code_passe_2(root->opr[1]);
            create_stack_deallocation_inst(get_temporary_max_offset());
            create_syscall_inst();
            break;

        case NODE_BLOCK:
            gen_code_passe_2(root->opr[0]);
            gen_code_passe_2(root->opr[1]);
            break;

        case NODE_LIST:
            gen_code_passe_2(root->opr[0]);
            gen_code_passe_2(root->opr[1]);
            break;

        case NODE_INTVAL:
            allocate_reg();
            create_addiu_inst(get_current_reg(), 0, root->value);
            break;

        case NODE_BOOLVAL:
            allocate_reg();
            create_addiu_inst(get_current_reg(), 0, root->value ? 1 : 0);
            break;

        case NODE_IDENT: {
            node_t decl = get_decl_node(root->ident);
            int32_t offset = decl->offset;

            allocate_reg();
            create_lw_inst(get_current_reg(), offset, 29);
            break;
        }

        case NODE_AFFECT: {
            gen_code_passe_2(root->opr[1]);
            int32_t r = get_current_reg();
            node_t decl = get_decl_node(root->opr[0]->ident);
            create_sw_inst(r, decl->offset, 29);
            release_reg();
            break;
        }
        
        case NODE_PRINT: {
            node_t p = root->opr[0];
            while (p) {
                node_t param = p->opr[1];

                if (param->nature == NODE_STRINGVAL) {
                    int32_t idx = add_string(param->str);
                    char * label = get_global_string(idx);

                    create_label_str_inst(label);
                    create_lui_inst(4, 0x1001);
                    create_ori_inst(4, 4, 0);
                    create_addiu_inst(2, 0, 4);
                    create_syscall_inst();
                } else {
                    gen_code_passe_2(param);
                    int32_t r = get_current_reg();

                    create_addiu_inst(4, r, 0);
                    create_addiu_inst(2, 0, 1);
                    create_syscall_inst();
                    release_reg();
                }

                p = p->opr[0];
            }
            break;
        }



        case NODE_PLUS:
        case NODE_MINUS:
        case NODE_MUL:
        case NODE_DIV:
        case NODE_MOD: {
            gen_code_passe_2(root->opr[0]);
            int32_t r1 = get_current_reg();
            gen_code_passe_2(root->opr[1]);
            int32_t r2 = get_current_reg();

            if (root->nature == NODE_PLUS)
                create_addu_inst(r1, r1, r2);
            else if (root->nature == NODE_MINUS)
                create_subu_inst(r1, r1, r2);
            else if (root->nature == NODE_MUL) {
                create_mult_inst(r1, r2);
                create_mflo_inst(r1);
            } else if (root->nature == NODE_DIV) {
                create_teq_inst(r2, 0);
                create_div_inst(r1, r2);
                create_mflo_inst(r1);
            } else {
                create_teq_inst(r2, 0);
                create_div_inst(r1, r2);
                create_mfhi_inst(r1);
            }

            release_reg();
            break;
        }
        
        case NODE_LT:
        case NODE_GT:
        case NODE_LE:
        case NODE_GE: {
            gen_code_passe_2(root->opr[0]);
            int32_t r1 = get_current_reg();
            gen_code_passe_2(root->opr[1]);
            int32_t r2 = get_current_reg();

            if (root->nature == NODE_LT)
                create_slt_inst(r1, r1, r2);
            else if (root->nature == NODE_GT)
                create_slt_inst(r1, r2, r1);
            else if (root->nature == NODE_LE) {
                create_slt_inst(r1, r2, r1);
                create_xori_inst(r1, r1, 1);
            } else {
                create_slt_inst(r1, r1, r2);
                create_xori_inst(r1, r1, 1);
            }

            release_reg();
            break;
        }

        case NODE_EQ:
        case NODE_NE: {
            gen_code_passe_2(root->opr[0]);
            int32_t r1 = get_current_reg();
            gen_code_passe_2(root->opr[1]);
            int32_t r2 = get_current_reg();

            create_xor_inst(r1, r1, r2);
            if (root->nature == NODE_EQ)
                create_sltiu_inst(r1, r1, 1);
            else
                create_sltu_inst(r1, 0, r1);

            release_reg();
            break;
        }

        case NODE_AND:
        case NODE_OR: {
            gen_code_passe_2(root->opr[0]);
            int32_t r1 = get_current_reg();
            gen_code_passe_2(root->opr[1]);
            int32_t r2 = get_current_reg();

            if (root->nature == NODE_AND)
                create_and_inst(r1, r1, r2);
            else
                create_or_inst(r1, r1, r2);

            release_reg();
            break;
        }
        
        case NODE_NOT: {
            gen_code_passe_2(root->opr[0]);
            int32_t r = get_current_reg();
            create_xori_inst(r, r, 1);
            break;
        }

        case NODE_UMINUS: {
            gen_code_passe_2(root->opr[0]);
            int32_t r = get_current_reg();
            create_subu_inst(r, 0, r);
            break;
        }


        case NODE_IF: {
            int32_t lbl_else = get_new_label();
            int32_t lbl_end  = get_new_label();

            gen_code_passe_2(root->opr[0]);
            create_beq_inst(get_current_reg(), 0, lbl_else);
            release_reg();

            gen_code_passe_2(root->opr[1]);
            create_j_inst(lbl_end);

            create_label_inst(lbl_else);
            if (root->opr[2])
                gen_code_passe_2(root->opr[2]);

            create_label_inst(lbl_end);
            break;
        }

        case NODE_WHILE: {
            int32_t lbl_start = get_new_label();
            int32_t lbl_end   = get_new_label();

            create_label_inst(lbl_start);
            gen_code_passe_2(root->opr[0]);
            create_beq_inst(get_current_reg(), 0, lbl_end);
            release_reg();

            gen_code_passe_2(root->opr[1]);
            create_j_inst(lbl_start);

            create_label_inst(lbl_end);
            break;
        }

        default:
            // Rien à mettre ? tous les noeuds ne sont pas renotés donc on peut avoir un truc inattendu qui n'est pas une erreur
            break;
    }
}