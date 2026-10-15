#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdint.h>

#include "cpu.h"
#include "assembler.h"

#define MAX_LINE	256
#define MAX_TOKENS	8
#define MAX_LABELS	64
#define MAX_NAME	32
#define PROG_MAX	128		/* program below 128, data above */

struct label {
	char		name[MAX_NAME];
	uint8_t		addr;
};

struct asm_ctx {
	struct label	labels[MAX_LABELS];
	int				n_labels;
};

struct op_def {
	const char	*name;
	int			opcode;
};

static const struct op_def g_ops[] = {
	{"LOAD", LOAD}, {"ADD", ADD}, {"STORE", STORE}, {"SUB", SUB},
	{"JMP", JMP}, {"JZ", JZ}, {"PRINT", PRINT}, {"HALT", HALT}
};

static int find_op(const char *name)
{
	size_t i = 0;

	while (i < sizeof(g_ops) / sizeof(g_ops[0])) {
		if (strcasecmp(name, g_ops[i].name) == 0)
			return (g_ops[i].opcode);
		i++;
	}
	return (-1);
}

static int parse_reg(const char *s)
{
	if (strcasecmp(s, "A") == 0)
		return (REG_A);
	if (strcasecmp(s, "B") == 0)
		return (REG_B);
	return (-1);
}

static int parse_num(const char *s, uint8_t *out)
{
	char *end;
	long v;

	if (*s == '\0')
		return (-1);
	v = strtol(s, &end, 0);		/* base 0: 0x2a works too */
	if (*end != '\0' || v < 0 || v > 255)
		return (-1);
	*out = (uint8_t)v;
	return (0);
}

static int find_label(struct asm_ctx *ctx, const char *name)
{
	int i = 0;

	while (i < ctx->n_labels) {
		if (strcmp(ctx->labels[i].name, name) == 0)
			return (i);
		i++;
	}
	return (-1);
}

static int add_label(struct asm_ctx *ctx, const char *name, int addr)
{
	if (name[0] == '\0' || strlen(name) >= MAX_NAME)
		return (-1);
	if (ctx->n_labels >= MAX_LABELS || find_label(ctx, name) != -1)
		return (-1);
	strcpy(ctx->labels[ctx->n_labels].name, name);
	ctx->labels[ctx->n_labels].addr = (uint8_t)addr;
	ctx->n_labels++;
	return (0);
}

/* jump target: a number or a label. In pass 0 a label may still be unknown. */
static int parse_target(struct asm_ctx *ctx, const char *s, int emit, uint8_t *out)
{
	int i;

	if (parse_num(s, out) == 0)
		return (0);
	i = find_label(ctx, s);
	if (i != -1) {
		*out = ctx->labels[i].addr;
		return (0);
	}
	if (!emit) {
		*out = 0;
		return (0);
	}
	return (-1);
}

static int encode(struct asm_ctx *ctx, char *tok[], int n, int emit, uint8_t out[2])
{
	int op = find_op(tok[0]);
	int dst, src, is_mem;

	out[1] = 0;
	switch (op) {
	case LOAD:
		if (n != 3)
			return (-1);
		dst = parse_reg(tok[1]);
		is_mem = (tok[2][0] == '#');
		if (dst < 0 || parse_num(tok[2] + is_mem, &out[1]) == -1)
			return (-1);
		out[0] = INSTR(LOAD, dst, 0) | (is_mem ? MODE_MEM : 0);
		return (0);
	case STORE:
		if (n != 3)
			return (-1);
		dst = parse_reg(tok[1]);
		if (dst < 0 || parse_num(tok[2], &out[1]) == -1)
			return (-1);
		out[0] = INSTR(STORE, dst, 0);
		return (0);
	case ADD:
	case SUB:
		if (n != 3)
			return (-1);
		dst = parse_reg(tok[1]);
		src = parse_reg(tok[2]);
		if (dst < 0 || src < 0)
			return (-1);
		out[0] = INSTR(op, dst, src);
		return (0);
	case PRINT:
		if (n != 2)
			return (-1);
		dst = parse_reg(tok[1]);
		if (dst < 0)
			return (-1);
		out[0] = INSTR(PRINT, dst, 0);
		return (0);
	case JMP:
	case JZ:
		if (n != 2 || parse_target(ctx, tok[1], emit, &out[1]) == -1)
			return (-1);
		out[0] = INSTR(op, 0, 0);
		return (0);
	case HALT:
		if (n != 1)
			return (-1);
		out[0] = INSTR(HALT, 0, 0);
		return (0);
	default:
		return (-1);
	}
}

static int tokenize(char *line, char *tok[], int max)
{
	int n = 0;
	char *p;

	p = strchr(line, ';');		/* ; starts a comment */
	if (p)
		*p = '\0';
	p = strtok(line, " \t\r\n,");
	while (p && n < max) {
		tok[n++] = p;
		p = strtok(NULL, " \t\r\n,");
	}
	return (n);
}

static int process_line(struct asm_ctx *ctx, char *line, int lineno,
						struct cpu *c, int *addr, int emit)
{
	char *tok[MAX_TOKENS];
	uint8_t out[2];
	int n, first = 0;
	size_t len;

	n = tokenize(line, tok, MAX_TOKENS);
	if (n == 0)
		return (0);
	len = strlen(tok[0]);
	if (tok[0][len - 1] == ':') {
		tok[0][len - 1] = '\0';
		if (!emit && add_label(ctx, tok[0], *addr) == -1) {
			fprintf(stderr, "line %d: invalid or duplicate label '%s'\n",
					lineno, tok[0]);
			return (-1);
		}
		first = 1;
	}
	if (first == n)
		return (0);
	if (encode(ctx, tok + first, n - first, emit, out) == -1) {
		fprintf(stderr, "line %d: invalid instruction '%s'\n",
				lineno, tok[first]);
		return (-1);
	}
	if (*addr + 2 > PROG_MAX) {
		fprintf(stderr, "line %d: program does not fit below address %d\n",
				lineno, PROG_MAX);
		return (-1);
	}
	if (emit) {
		c->mem[*addr] = out[0];
		c->mem[*addr + 1] = out[1];
	}
	*addr += 2;
	return (0);
}

/*
** Two passes: pass 0 only collects the labels (so you can jump to a label
** further down), pass 1 actually writes the bytes into memory.
*/
int assemble(const char *path, struct cpu *c)
{
	struct asm_ctx ctx = {0};
	char line[MAX_LINE];
	FILE *f;
	int pass = 0;
	int addr, lineno;

	f = fopen(path, "r");
	if (!f) {
		perror(path);
		return (-1);
	}
	while (pass < 2) {
		rewind(f);
		addr = 0;
		lineno = 0;
		while (fgets(line, sizeof(line), f)) {
			lineno++;
			if (process_line(&ctx, line, lineno, c, &addr, pass) == -1) {
				fclose(f);
				return (-1);
			}
		}
		pass++;
	}
	fclose(f);
	return (0);
}