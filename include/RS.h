#ifndef __RS_H__
#define __RS_H__
#include <stdbool.h>
#include <stdint.h>
#include "util.h"
#define F_REG_IDX(idx) idx >> 1
#define SET_Q_FIELD_EMPTY 255

/*
 * Nine indexes to find the corresponding RS.
 * */
enum RS_idx {
    LOAD0,
    LOAD1,
    STORE0,
    STORE1,
    ADDER0,
    ADDER1,
    ADDER2,
    MUL0,
    MUL1,
};

typedef struct reservation_station {
    bool busy;
    uint8_t opcode;
    double vj;
    double vk;
    uint8_t qj;
    uint8_t qk;
    uint32_t addr;
    uint32_t cycle;
    uint32_t instruction_idx;
} res_stat;

typedef struct register_status {
    uint8_t qi;
} reg_status;

/*
 * @RS: An array with reservation_station for simulating the reservation station
 * in hardware. RS[0] ~ RS[1] --> Load Buffer RS[2] ~ RS[3] --> Store Buffer
 *      RS[4] ~ RS[6] --> Adder
 *      RS[7] ~ RS[8] --> Multiplier
 *
 *      You can use the enumeration instance called 'RS_idx' to index to the
 * certain RS conponent.
 *
 * @float_reg_state: Array with reg_status type that each of them represent
 *                      the Qi field of a floating-point register, and
 *                      will record the functional unit of a cetrain RS
 *                      conponent.
 * @int_reg_state: It is similar to float_reg_state, but it is for integer
 * registers.
 * */
extern res_stat RS[9];
extern double RS_result[9];
extern bool RS_ready_broadcast[9];
extern bool RS_result_ready[9];
extern reg_status float_reg_state[16];
extern reg_status int_reg_state[32];

/*
 * Given two lower and higher indexes, finding an empty RS conponent for a
 * certain class. ex: find_empty_RS(ADDER0, ADDER2) --> find an empty RS, which
 * are adders.
 * */
uint8_t find_empty_RS(uint8_t, uint8_t);

/*
 * Setting Qi field for a certain register.
 * */
void set_RS_field(uint8_t, reg *, char);

void set_Qi_field(uint8_t, reg *);

void fill_ins_info(res_stat *);

#endif