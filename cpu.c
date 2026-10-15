#include <stdint.h>
#include <stdio.h>

#include "cpu.h"
#include "instructions.c"
#include "assembler.h"

uint8_t fetch(struct cpu *c) {
	uint8_t byte = c->mem[c->pc];
	c->pc++;
	return (byte);
}

int decode_execute(uint8_t byte, struct cpu *c)
{
	uint8_t opcode = byte >> 5;
	uint8_t reg_dst = (byte >> 4) & 1;
	uint8_t reg_src = (byte >> 3) & 1;
	uint8_t modus_bit = (byte >> 2) & 1;

	switch (opcode) {
	case LOAD:
		load(reg_dst, modus_bit, c);
		break;
	case ADD:
		add(reg_dst, reg_src, c);
		break;
	case STORE:
		if (store(reg_dst, c))
			return (-2);
		break;
	case SUB:
		sub(reg_dst, reg_src, c);
		break;
	case JZ:
		jz(c);
		break;
	case JMP:
		jmp(c);
		break;
	case PRINT:
		print(reg_dst, c);
		break;
	case HALT:
		c->pc++;
		return (-1);
	}

	return (0);
}

int main(int argc, char **argv)
{
	struct cpu cpu = {0};

	if (argc != 2) {
		fprintf(stderr, "usage: %s prog.asm\n", argv[0]);
		return (1);
	}
	if (assemble(argv[1], &cpu) == -1)
		return (1);
	while (1)
	{
		uint8_t instruction = fetch(&cpu);
		int result;
		result = decode_execute(instruction, &cpu);
		if (result == -1)
			break;
		if (result == -2) {
			fprintf(stderr, "Error: STORE below address 128 is not allowed.\n");
			return (1);
		}
	}

	return (0);
}