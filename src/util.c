#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "util.h"
#include "RS.h"

FILE *fptr = NULL;
bool running = true;
uint32_t clock = 0;
char const *instruction_types[6] = {"L.D",   "S.D",   "ADD.D",
                                    "SUB.D", "MUL.D", "DIV.D"};
uint8_t ins_type_cost[6] = {2, 1, 2, 2, 10, 40};
double float_reg[16] = {[0 ... 15] = 1};
int32_t int_reg[32] = {[1] = 16};
double memory[8] = {[0 ... 7] = 1};

instruction_cycle *ins_cycle;
uint32_t ins_cycle_len = 0;

void write_RS_result_to_reg_res_stat(uint8_t rs_idx) {
    uint8_t idx = 0;
    for (idx = 0; idx < 16; idx++) {
        if (float_reg_state[idx].qi == rs_idx) {
            float_reg[idx] = RS_result[rs_idx];
            float_reg_state[idx].qi = SET_Q_FIELD_EMPTY;
        }
    }
    for (idx = 0; idx < 32; idx++) {
        if (int_reg_state[idx].qi == rs_idx) {
            int_reg[idx] = RS_result[rs_idx];
            int_reg_state[idx].qi = SET_Q_FIELD_EMPTY;
        }
    }
    for (idx = 0; idx < 9; idx++) {
        if (idx == rs_idx)
            continue;
        if (RS[idx].qj == rs_idx) {
            RS[idx].vj = RS_result[rs_idx];
            RS[idx].qj = SET_Q_FIELD_EMPTY;
        }
        if (RS[idx].qk == rs_idx) {
            RS[idx].vk = RS_result[rs_idx];
            RS[idx].qk = SET_Q_FIELD_EMPTY;
        }
    }
}

void write_back_broadcast(void) {
    uint8_t rs_idx;
    for (rs_idx = 0; rs_idx < 9; rs_idx++) {
        if (RS_ready_broadcast[rs_idx] == true) {
            RS[rs_idx].busy = false;
            RS_ready_broadcast[rs_idx] = false;
            memset(RS + rs_idx, 0, sizeof(res_stat));
            write_RS_result_to_reg_res_stat(rs_idx);
            RS_result_ready[rs_idx] = false;
        }
    }
}

bool write_result(void) {
    bool any_execute;
    char *mem_ptr = (char *)memory;
    uint8_t rs_idx;
    any_execute = 0;
    for (rs_idx = 0; rs_idx < 9; rs_idx++) {
        if (RS[rs_idx].busy == false || RS[rs_idx].cycle != 0)
            continue;
        // Store
        if ((rs_idx == STORE0 || rs_idx == STORE1) &&
            RS[rs_idx].qk == SET_Q_FIELD_EMPTY) {
            RS[rs_idx].addr += (int)RS[rs_idx].vj;
            *(double *)(mem_ptr + RS[rs_idx].addr) = RS[rs_idx].vk;
        }
        else if (RS_result_ready[rs_idx] == false) {
            continue;
        }
        ins_cycle[RS[rs_idx].instruction_idx - 1].write = clock + 1;
        RS_ready_broadcast[rs_idx] = true; // RS[rs_idx].busy = false;
        RS[rs_idx].instruction_idx = 0;
        any_execute = true;
    }

    return any_execute;
}

bool execute(void) {
    bool any_execute;
    char *mem_ptr = (char *)memory;
    uint8_t rs_idx;
    any_execute = 0;
    for (rs_idx = 0; rs_idx < 9; rs_idx++) {
        if (RS[rs_idx].busy == false)
            continue;
        if ((rs_idx == LOAD0 || rs_idx == LOAD1) &&
            RS[rs_idx].qj == SET_Q_FIELD_EMPTY) {
            if (RS[rs_idx].cycle == 2)
                RS[rs_idx].addr += RS[rs_idx].vj;
            else {
                RS_result[rs_idx] = *(double *)(mem_ptr + RS[rs_idx].addr);
                RS_result_ready[rs_idx] = true;
                if (RS[rs_idx].instruction_idx)
                    ins_cycle[RS[rs_idx].instruction_idx - 1].execution = clock + 1;
            }
            if (RS[rs_idx].cycle > 0)
                RS[rs_idx].cycle--;
            any_execute = true;
        }
        else if ((rs_idx == STORE0 || rs_idx == STORE1)) {
            if (RS[rs_idx].cycle > 0) {
                RS[rs_idx].cycle--;
                if (RS[rs_idx].instruction_idx)
                    ins_cycle[RS[rs_idx].instruction_idx - 1].execution = clock + 1;
                any_execute = true;
            }
        }
        else if ((rs_idx == ADDER0 || rs_idx == ADDER1 || rs_idx == ADDER2) &&
                 RS[rs_idx].qj == SET_Q_FIELD_EMPTY &&
                 RS[rs_idx].qk == SET_Q_FIELD_EMPTY) {
            if (RS[rs_idx].cycle > 0)
                RS[rs_idx].cycle--;
            if (RS[rs_idx].cycle == 0) {
                if (RS[rs_idx].opcode == 2)
                    RS_result[rs_idx] = RS[rs_idx].vj + RS[rs_idx].vk;
                else
                    RS_result[rs_idx] = RS[rs_idx].vj - RS[rs_idx].vk;
                RS_result_ready[rs_idx] = true;
                if (RS[rs_idx].instruction_idx)
                    ins_cycle[RS[rs_idx].instruction_idx - 1].execution = clock + 1;
            }
            any_execute = true;
        }
        else if ((rs_idx == MUL0 || rs_idx == MUL1) &&
                 RS[rs_idx].qj == SET_Q_FIELD_EMPTY &&
                 RS[rs_idx].qk == SET_Q_FIELD_EMPTY) {
            if (RS[rs_idx].cycle > 0)
                RS[rs_idx].cycle--;
            if (RS[rs_idx].cycle == 0) {
                if (RS[rs_idx].opcode == 4)
                    RS_result[rs_idx] = RS[rs_idx].vj * RS[rs_idx].vk;
                else
                    RS_result[rs_idx] = RS[rs_idx].vj / RS[rs_idx].vk;
                RS_result_ready[rs_idx] = true;
                if (RS[rs_idx].instruction_idx)
                    ins_cycle[RS[rs_idx].instruction_idx - 1].execution = clock + 1;
            }
            any_execute = true;
        }
    }

    return any_execute;
}

bool put_ins_into_RS(struct instruction *ins_field) {
    uint8_t rs_idx;

    // Load or Store
    if (ins_field->opcode < 2) {
        if (ins_field->opcode == 0)
            rs_idx = find_empty_RS(LOAD0, LOAD1);
        else
            rs_idx = find_empty_RS(STORE0, STORE1);

        if (rs_idx == SET_Q_FIELD_EMPTY)
            return false;
        RS[rs_idx].opcode = ins_field->opcode;
        set_RS_field(rs_idx, &ins_field->rs, 'j');
        RS[rs_idx].addr = ins_field->offset;
        RS[rs_idx].busy = true;
        if (ins_field->opcode == 0)
            set_Qi_field(rs_idx, &ins_field->rt);
        else
            set_RS_field(rs_idx, &ins_field->rt, 'k');
    }
    // R-Format
    else {
        if (ins_field->opcode == 2 || ins_field->opcode == 3)
            rs_idx = find_empty_RS(ADDER0, ADDER2);
        else
            rs_idx = find_empty_RS(MUL0, MUL1);

        if (rs_idx == SET_Q_FIELD_EMPTY)
            return false;
        RS[rs_idx].opcode = ins_field->opcode;
        set_RS_field(rs_idx, &ins_field->rs, 'j');
        set_RS_field(rs_idx, &ins_field->rt, 'k');
        RS[rs_idx].busy = true;
        set_Qi_field(rs_idx, &ins_field->rd);
    }
    RS[rs_idx].cycle += ins_type_cost[ins_field->opcode];
    ins_cycle = realloc(ins_cycle, sizeof(instruction_cycle) * ++ins_cycle_len);
    RS[rs_idx].instruction_idx = ins_cycle_len;
    ins_cycle[RS[rs_idx].instruction_idx - 1].issue = clock + 1;
    return true;
}

bool issue(char *single_ins) {
    char ins_type[8];
    bool success;
    instruction ins_field;
    if (single_ins[0] == '\0')
        return false;

    uint8_t idx;
    for (idx = 5; idx >= 0; idx--) {
        if (strstr(single_ins, instruction_types[idx]) != NULL) {
            ins_field.opcode = idx;
            break;
        }
    }

    // No instruction
    if (idx == 6)
        return false;
    // Load or Store
    else if (ins_field.opcode < 2) {
        sscanf(single_ins, "%s %c%hhd, %d(%c%hhd)\n", ins_type,
               &ins_field.rt.type, &ins_field.rt.num, &ins_field.offset,
               &ins_field.rs.type, &ins_field.rs.num);
    }
    // R-format
    else {
        sscanf(single_ins, "%s %c%hhd, %c%hhd, %c%hhd\n", ins_type,
               &ins_field.rd.type, &ins_field.rd.num, &ins_field.rs.type,
               &ins_field.rs.num, &ins_field.rt.type, &ins_field.rt.num);
    }
    success = put_ins_into_RS(&ins_field);

    if (success == false) {
        long offset = strlen(single_ins) + 1;
        fseek(fptr, -1 * offset, SEEK_CUR);
    }
    else
        strcpy(ins_cycle[ins_cycle_len - 1].instruction, single_ins);
    return success;
}

void show_all_resource(void) {
    uint8_t rs_idx;
    uint8_t reg_idx;
    static char *RS_name[] = {
        "Load0",
        "Load1",
        "Store0",
        "Store1",
        "Add0",
        "Add1",
        "Add2",
        "Mul0",
        "Mul1"
    };
    printf("cycle - %d\n", clock);
    for (int i = 0; i < ins_cycle_len; i++) {
        printf("%-20s : %d - %d - %d\n", ins_cycle[i].instruction, ins_cycle[i].issue,
               ins_cycle[i].execution, ins_cycle[i].write);
    }
    printf("\n");
    printf("%8s|%8s|%8s|%10s|%10s|%8s|%8s|%8s|%8s|\n", "FU", "busy",
           "opcode", "vj", "vk", "qj", "qk", "addr", "cycle");
    for (rs_idx = 0; rs_idx < 9; rs_idx++) {
        printf("%8s|%8d|%8s|%10lf|%10lf|%8d|%8d|%8d|%8d|\n", RS_name[rs_idx],
               RS[rs_idx].busy, (RS[rs_idx].busy == false)? NULL : instruction_types[RS[rs_idx].opcode],
               RS[rs_idx].vj, RS[rs_idx].vk, RS[rs_idx].qj, RS[rs_idx].qk,
               RS[rs_idx].addr, RS[rs_idx].cycle);
    }
    printf("Floating Point Registers : \n");
    for (reg_idx = 0; reg_idx < 16; reg_idx++) {
        printf(" F%2d : %s", reg_idx << 1, RS_name[float_reg_state[reg_idx].qi]);
        if (reg_idx % 8 == 7)
            printf("\n");
    }
    printf("\n");
}

void system_init(void) {
    ins_cycle = NULL;
}


void system_terminate(void) {
    printf("== Result ==\n");
    for (int i = 0; i < ins_cycle_len; i++) {
        printf("%-20s : %d - %d - %d\n", ins_cycle[i].instruction, ins_cycle[i].issue,
               ins_cycle[i].execution, ins_cycle[i].write);
    }
    uint8_t idx;
    printf("\n");
    for (idx = 0; idx < 32; idx += 2) {
        printf(" F%2d : %lf", idx, float_reg[F_REG_IDX(idx)]);
        if (idx % 8 == 6)
            printf("\n");
    }
    printf("\n");
    for (idx = 0; idx < 32; idx++) {
        printf(" R%2d : %4d", idx, int_reg[idx]);
        if (idx % 4 == 3)
            printf("\n");
    }
    printf("\n");
    for (idx = 0; idx < 8; idx++) {
        printf(" M%d : %lf", idx, memory[idx]);
        if (idx % 4 == 3)
            printf("\n");
    }
    printf("\n");
    for (idx = 0; idx < 64; idx++) {
        printf(" M%2d : %d", idx, ((char *)(memory))[idx]);
        if (idx % 8 == 7)
            printf("\n");
    }
    printf("\n");
    free(ins_cycle);
}
