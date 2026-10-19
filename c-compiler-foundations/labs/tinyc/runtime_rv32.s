# Minimal bare-metal runtime for rv32sim: the part of "crt0 + libc" that a
# compiler's output assumes exists. Link this (by concatenation) after the
# assembly produced by a real compiler, e.g.
#   clang --target=riscv32 -march=rv32im -mabi=ilp32 -O2 -S prog.c -o prog.s
#   cat prog.s runtime_rv32.s > all.s && python3 rv32sim.py all.s
    .text
    .globl _start
_start:
    call main          # C: execution begins at main...
    li a7, 93          # ...and main's return value becomes the exit status.
    ecall

    .globl print_int
print_int:             # int print_int(int v): a0 already holds v
    li a7, 1
    ecall
    li a0, 0
    ret

    .globl print_char
print_char:            # int print_char(int c)
    li a7, 11
    ecall
    li a0, 0
    ret
