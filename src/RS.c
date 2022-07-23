#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include "RS.h"

res_stat RS[9];
reg_status float_reg_state[16] = {[0 ... 15] = SET_Q_FIELD_EMPTY};
reg_status int_reg_state[32] = {[0 ... 31] = SET_Q_FIELD_EMPTY};
double RS_result[9] = {0};
bool RS_result_ready[9] = {0};
bool RS_ready_broadcast[9] = {0};

uint8_t find_empty_RS(uint8_t low_rs_idx, uint8_t high_rs_idx) {
    uint8_t rs_idx;
    for (rs_idx = low_rs_idx; rs_idx <= high_rs_idx; rs_idx++) {
        if (RS[rs_idx].busy == false)
            break;
    }
    return rs_idx == high_rs_idx + 1 ? SET_Q_FIELD_EMPTY : rs_idx;
}

void set_RS_Q_field(uint8_t rs_idx, reg *target_reg, char field) {
    if (field == 'j') {
        if (target_reg->type == 'F')
            RS[rs_idx].qj = float_reg_state[F_REG_IDX(target_reg->num)].qi;
        else
            RS[rs_idx].qj = int_reg_state[target_reg->num].qi;
    }
    else {
        if (target_reg->type == 'F')
            RS[rs_idx].qk = float_reg_state[F_REG_IDX(target_reg->num)].qi;
        else
            RS[rs_idx].qk = int_reg_state[target_reg->num].qi;
    }
}

void set_RS_V_field(uint8_t rs_idx, reg *target_reg, char field) {
    if (field == 'j') {
        if (target_reg->type == 'F') {
            RS[rs_idx].vj = float_reg[F_REG_IDX(target_reg->num)];
        }
        else {
            RS[rs_idx].vj = int_reg[target_reg->num];
        }
        RS[rs_idx].qj = SET_Q_FIELD_EMPTY;
    }
    else {
        if (target_reg->type == 'F') {
            RS[rs_idx].vk = float_reg[F_REG_IDX(target_reg->num)];
        }
        else {
            RS[rs_idx].vk = int_reg[target_reg->num];
        }
        RS[rs_idx].qk = SET_Q_FIELD_EMPTY;
    }
}

void set_Qi_field(uint8_t rs_idx, reg *target_reg) {
    if (target_reg->type == 'F')
        float_reg_state[F_REG_IDX(target_reg->num)].qi = rs_idx;
    else
        int_reg_state[target_reg->num].qi = rs_idx;
}

void set_RS_field(uint8_t rs_idx, reg *target_reg, char field) {
    if (target_reg->type == 'F') {
        if (float_reg_state[F_REG_IDX(target_reg->num)].qi != SET_Q_FIELD_EMPTY)
            set_RS_Q_field(rs_idx, target_reg, field);
        else {
            set_RS_V_field(rs_idx, target_reg, field);
        }
    }
    else {
        if (int_reg_state[target_reg->num].qi != SET_Q_FIELD_EMPTY)
            set_RS_Q_field(rs_idx, target_reg, field);
        else {
            set_RS_V_field(rs_idx, target_reg, field);
        }
    }
}
