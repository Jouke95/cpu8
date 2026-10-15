#ifndef ASSEMBLER_H
# define ASSEMBLER_H

# include "cpu.h"

/*
** Reads an assembly text-file and writes the bytes into c->mem.
** Returns 0 on success, -1 on an error (stderr).
*/
int assemble(const char *path, struct cpu *c);

#endif