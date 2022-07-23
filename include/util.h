#ifndef __UTIL_H__
#define __UTIL_H__
#include <stdbool.h>
#include <stdint.h>

typedef struct reg {
    char type;
    uint8_t num;
} reg;

typedef struct instruction {
    uint8_t opcode;
    reg rs;
    reg rt;
    reg rd;
    int32_t offset;
} instruction;

typedef struct instruction_cycle {
    char instruction[32];
    uint32_t issue; // Record the clock when the instruction is completed.
    uint32_t execution;
    uint32_t write;
} instruction_cycle;

/*
 * @fptr: A FILE pointer open a file with MIPS instructions.
 * @running: A flag to check whether the entire emulator is running or not.
 * @clock: A variable to simulate the CPU clock.
 * @instrction_types: Six constant string containing six different MIPS
 * operations.
 * @float_reg: An array with float type to represent 16 floating-point
 * registers.
 * @int_reg: An array with int type to simulate 32 integer registers.
 * */
extern FILE *fptr;
extern bool running;
extern instruction_cycle *ins_cycle;
extern uint32_t clock;
extern char const *instruction_types[6];
extern double float_reg[16];
extern int32_t int_reg[32];
extern double memory[8];

bool write_result(void);

bool execute(void);

bool issue(char *);

void write_back_broadcast(void);

void show_all_resource(void);

void system_init(void);

void system_terminate(void);

#endif