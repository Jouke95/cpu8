# cpu8

A simulator of a minimal 8-bit computer in C, with its own instruction set and a mini assembler that turns text
programs into machine code. Built to understand how a CPU really works: the fetch-decode-execute cycle, registers,
a program counter, flags, and how program and data share a single memory.

## What is in it

- 2 registers (`A`, `B`) and a `pc`, all 8 bits wide, plus a zero flag
- 256 bytes of memory
- 8 instructions: `LOAD`, `STORE`, `ADD`, `SUB`, `JMP`, `JZ`, `PRINT`, `HALT`
- `LOAD` has two modes: load a value, or load the contents of a memory address
- an assembler with labels, comments, and decimal or hexadecimal numbers
- memory protection: a `STORE` into the program area (address < 128) stops the CPU with an error

The full contract between program and CPU is in [ISA.md](ISA.md): instruction bit layout, registers, flag rules
and memory layout.

## Build and run

```
make
./cpu8 countdown.asm
```

`make re` rebuilds everything, `make fclean` cleans up.

## Example

`countdown.asm`:

```
; count down from 10 to 0, and keep a value in memory
        LOAD  A 10
        LOAD  B 1
loop:   SUB   A B
        PRINT A
        JZ    end
        JMP   loop
end:    LOAD  A 100
        STORE A 142
        LOAD  B #142        ; # = the contents of address 142
        PRINT B
        HALT
```

Output:

```
Register A has the value: 9
Register A has the value: 8
...
Register A has the value: 0
Register B has the value: 100
```

## Code layout

| File | Contents |
|------|----------|
| `cpu.c` | `main`, `fetch()` and `decode_execute()` |
| `cpu.h` | `struct cpu`, opcodes, registers and the `INSTR` macro |
| `instructions.c` | the execution of each instruction (included by `cpu.c`) |
| `assembler.c`, `assembler.h` | the assembler: two passes, so you can also jump to a label further down |
| `ISA.md` | instruction set specification |
| `countdown.asm` | example program |

## Limitations

- Only a zero flag, so no "greater than" or "less than". Comparing is done with `SUB`.
- No stack, so no `CALL`/`RET`.
- `LOAD` reads anywhere, and a `JMP` into the data area is not blocked.