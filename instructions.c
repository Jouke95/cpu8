#include "cpu.h"

void load(uint8_t reg_dst, uint8_t modus_bit, struct cpu *c) {
	uint8_t next_byte = fetch(c);
	if (reg_dst == REG_A) {
		if (!modus_bit)
			c->A = next_byte;
		else
			c->A = c->mem[next_byte];
	}

	else {
		if (!modus_bit)
			c->B = next_byte;
		else
			c->B = c->mem[next_byte];
	}
}

void add(uint8_t reg_dst, uint8_t reg_src, struct cpu *c) {
	uint8_t dst_val, src_val;

	if (reg_dst == REG_A)
		dst_val = c->A;
	else
		dst_val = c->B;

	if (reg_src == REG_A)
		src_val = c->A;
	else
		src_val = c->B;

	uint8_t result = dst_val + src_val;
	if (reg_dst == REG_A)
		c->A = result;
	else
		c->B = result;
	c->flag = (result == 0);
	c->pc++;
}

int store(uint8_t reg_dst, struct cpu *c) {
	uint8_t address = fetch(c);
	if (address <= 127)
		return (-1);

	uint8_t value;
	if (reg_dst == REG_A)
		value = c->A;
	else
		value = c->B;
	c->mem[address] = value;

	return (0);
}

void sub(uint8_t reg_dst, uint8_t reg_src, struct cpu *c) {
	uint8_t dst_val, src_val;

	if (reg_dst == REG_A)
		dst_val = c->A;
	else
		dst_val = c->B;
	if (reg_src == REG_A)
		src_val = c->A;
	else
		src_val = c->B;
	uint8_t result = dst_val - src_val;
	if (reg_dst == REG_A)
		c->A = result;
	else
		c->B = result;
	c->flag = (result == 0);
	c->pc++;
}

void jz(struct cpu *c) {
	uint8_t target = fetch(c);
	if (c->flag)
		c->pc = target;
}

void jmp(struct cpu *c) {
	uint8_t target = fetch(c);
	c->pc = target;
}

void print(uint8_t reg_dst, struct cpu *c) {
	if (reg_dst == REG_A)
		printf("Register A has the value: %i\n", c->A);
	else
		printf("Register B has the value: %i\n", c->B);
	c->pc++;
}
