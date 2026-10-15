#ifndef CPU_H
#define CPU_H

#include <stdint.h>

enum {
	LOAD,
	ADD,
	STORE,
	SUB,
	JMP,
	JZ,
	PRINT,
	HALT
};

enum {
	REG_A,
	REG_B
};

#define MODE_MEM (1 << 2)
#define INSTR(opcode, dest, source) ((opcode << 5) | (dest << 4) | (source << 3))

struct cpu {
	uint8_t A, B, pc, flag;
	uint8_t mem[256];
};

uint8_t fetch(struct cpu *c);

#endif
