# cpu8 — Instruction Set Architecture

A minimal 8-bit CPU. This document is the contract between a program (bytes in memory) and the CPU that runs it.

## 1. Registers and memory

| Register | Size | Role |
|----------|------|------|
| `A`      | 8 bits | general register, values 0 to 255 |
| `B`      | 8 bits | general register, values 0 to 255 |
| `pc`     | 8 bits | **address** of the next byte to fetch (not an instruction number) |
| `flag`   | 1 bit (stored in a `uint8_t`) | zero flag, see below |

- Arithmetic wraps modulo 256: `255 + 1 = 0` and `3 - 10 = 249`. There is no overflow error.
- Memory: 256 bytes of 8 bits each, addresses 0 to 255. An 8-bit `pc` can address every byte exactly, which is why the sizes fit together.
- Register encoding in instructions: `0` = A, `1` = B.

### The zero flag
- `flag = 1`: the last `ADD` or `SUB` result was 0.
- `flag = 0`: the last result was not 0 (also the value at startup).
- Only `ADD` and `SUB` set the flag. `JZ` is the only instruction that reads it.
- `LOAD`, `STORE` and `PRINT` leave the flag untouched. A `JZ` after a `LOAD` therefore still looks at the last `ADD`/`SUB`.

## 2. Instruction format

Every instruction is **2 bytes** (fixed length).

```
byte 1:  7 6 5 | 4   | 3   | 2    | 1 0
         opcode| dst | src | mode | reserved (always 0)

byte 2:  operand (value or address) or padding
```

| Bits | Name | Meaning |
|------|------|---------|
| 7-5  | opcode | which instruction (8 possibilities) |
| 4    | dst | destination register. For `ADD`/`SUB` it is also the first operand. For `LOAD`, `STORE`, `PRINT` it is the only register |
| 3    | src | source register. Only used by `ADD` and `SUB` |
| 2    | mode | `LOAD` only: 0 = byte 2 is a value, 1 = byte 2 is an address |
| 1-0  | reserved | must be 0. The CPU ignores them, but they may get a meaning later |

Byte 2 is padding for `ADD`, `SUB`, `PRINT` and `HALT`. Its value does not matter, but the byte must be there,
otherwise everything after it shifts.

Example: `SUB B A` = `01110000`: opcode `011`, dst = B (1), src = A (0). The result is B − A.

## 3. Instructions

| Opcode | Name  | Byte 2 | Effect | Flag |
|--------|-------|--------|--------|------|
| `000`  | LOAD  | value or address (bit 2) | mode 0: `reg = byte 2`. Mode 1: `reg = mem[byte 2]` | unchanged |
| `001`  | ADD   | padding | `dst = dst + src` | `flag = (result == 0)` |
| `010`  | STORE | address (>= 128) | `mem[byte 2] = reg`. An address below 128 is an error | unchanged |
| `011`  | SUB   | padding | `dst = dst - src` | `flag = (result == 0)` |
| `100`  | JMP   | address | `pc = byte 2` | unchanged |
| `101`  | JZ    | address | if `flag == 1`: `pc = byte 2`, otherwise nothing | unchanged |
| `110`  | PRINT | padding | print the value of the register | unchanged |
| `111`  | HALT  | padding | the CPU stops | unchanged |

### The program counter
- Every instruction fetches 2 bytes, so the `pc` ends up 2 further.
- `JMP` always overwrites the `pc`, `JZ` only when the flag is 1. Byte 2 has already been fetched by then, so on a
  `JZ` that is not taken you leave the `pc` alone.
- `HALT` also advances the `pc` by 2, even though the CPU stops anyway. This way the `pc` always ends up on the byte
  after the last instruction.
- Without a `HALT` the CPU runs on through the (empty) memory and the `pc` wraps from 255 to 0.

## 4. Memory layout

| Addresses | Use |
|-----------|-----|
| 0 to 127   | program |
| 128 to 255 | data |

- The assembler rejects a program that does not fit below address 128.
- A `STORE` to an address below 128 is an error: the CPU stops with an error message and exit code 1. This protects
  the program against silent corruption.
- Program and data live in the same memory (von Neumann). Beyond that the CPU does not enforce the split: `LOAD`
  reads anywhere, and a `JMP` into the data area is not blocked.

## 5. Assembly syntax

```
; a comment starts with a semicolon
label:  INSTRUCTION  operand, operand   ; comment
```

- One instruction per line. A label (`name:`) may precede an instruction or sit on its own line.
- Instruction and register names are case-insensitive.
- Operands are separated by spaces or commas.
- Numbers are decimal (`42`) or hexadecimal (`0x2a`) and must be between 0 and 255.
- Registers are `A` and `B`.

| Line | Meaning |
|------|---------|
| `LOAD A 42`     | A = the value 42 |
| `LOAD A #42`    | A = the contents of address 42 (`#` sets the mode bit) |
| `STORE A 142`   | write A to address 142 |
| `ADD A B`       | A = A + B |
| `SUB A B`       | A = A − B |
| `PRINT A`       | print A |
| `JMP loop`      | jump to the label `loop` (or to a number) |
| `JZ end`        | jump to `end` if the flag is 1 (label or number) |
| `HALT`          | stop |

Labels are resolved in two passes: the first pass records the address of every label, the second pass writes the
bytes. That is why you can also jump to a label further down. A label is just an address: the CPU does not know it
ever had a name.

### Example

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
        LOAD  B #142
        PRINT B
        HALT
```

Output: 9 down to 0, then 100. When the program ends, `pc = 22`.