#!/usr/bin/env python3
"""rv32sim: an assembly-level RV32IM simulator for teaching.

It executes RISC-V *assembly text* (not machine code), so the output of
tinyc.py, or hand-written code, can be run and traced without a toolchain.
Supported: the RV32I base integer instructions plus the M extension, the
common pseudo-instructions, `.text/.data/.word/.zero/.space/.align/.globl`,
and RARS-style environment calls:

    a7 = 1   print_int(a0)
    a7 = 11  print_char(a0)
    a7 = 10  exit()        (status 0)
    a7 = 93  exit(a0)

Memory is byte-addressable and little-endian, as a real RV32 core would be.
Misaligned or unmapped accesses stop the machine with an error instead of
silently returning garbage: finding these early is part of the exercise.

Usage: rv32sim.py program.s [--trace] [--stats] [--check-align] [--max-steps N]

`--check-align` enforces the psABI rule that sp is 16-byte aligned at every
call, which hardware never checks but library code may silently depend on.
"""
from __future__ import annotations

import re
import sys
from typing import Dict, List, Optional, Tuple

TEXT_BASE = 0x00400000
DATA_BASE = 0x10010000
STACK_TOP = 0x7FFFFFF0
PAGE = 4096

ABI = ["zero", "ra", "sp", "gp", "tp", "t0", "t1", "t2", "s0", "s1",
       "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7",
       "s2", "s3", "s4", "s5", "s6", "s7", "s8", "s9", "s10", "s11",
       "t3", "t4", "t5", "t6"]
REGNUM = {name: i for i, name in enumerate(ABI)}
REGNUM.update({f"x{i}": i for i in range(32)})
REGNUM["fp"] = 8


class SimError(Exception):
    pass


def u32(x: int) -> int:
    return x & 0xFFFFFFFF


def s32(x: int) -> int:
    x &= 0xFFFFFFFF
    return x - (1 << 32) if x & 0x80000000 else x


class Memory:
    """Sparse byte memory: pages are created on first write and on data load."""

    def __init__(self) -> None:
        self.pages: Dict[int, bytearray] = {}

    def _page(self, addr: int, create: bool) -> Tuple[bytearray, int]:
        base, off = addr & ~(PAGE - 1), addr & (PAGE - 1)
        page = self.pages.get(base)
        if page is None:
            if not create:
                raise SimError(f"access to unmapped address 0x{addr:08x}")
            page = self.pages[base] = bytearray(PAGE)
        return page, off

    def load(self, addr: int, size: int, signed: bool) -> int:
        if addr % size:
            raise SimError(f"misaligned {size}-byte load at 0x{addr:08x}")
        page, off = self._page(addr, create=False)
        val = int.from_bytes(page[off:off + size], "little", signed=signed)
        return val

    def store(self, addr: int, size: int, val: int) -> None:
        if addr % size:
            raise SimError(f"misaligned {size}-byte store at 0x{addr:08x}")
        if not (TEXT_BASE <= addr < TEXT_BASE + 0x100000 or addr >= DATA_BASE):
            raise SimError(f"store to unmapped address 0x{addr:08x}")
        page, off = self._page(addr, create=True)
        page[off:off + size] = (val & ((1 << (8 * size)) - 1)).to_bytes(size, "little")


# --------------------------------------------------------------------------
# Assembler front half: text -> (instructions with resolved labels, data image)
# --------------------------------------------------------------------------

class Instr:
    __slots__ = ("op", "args", "line", "addr", "text")

    def __init__(self, op: str, args: List[str], line: int, text: str) -> None:
        self.op, self.args, self.line, self.text = op, args, line, text
        self.addr = 0


def split_operands(s: str) -> List[str]:
    return [p.strip() for p in s.split(",")] if s.strip() else []


def parse_program(src: str) -> Tuple[List[Instr], Dict[str, int], Memory]:
    instrs: List[Instr] = []
    labels: Dict[str, int] = {}
    mem = Memory()
    section = "text"
    data_ptr = DATA_BASE
    pending_data: List[Tuple[int, int, int]] = []     # (addr, size, value-or-label-index)
    data_label_refs: List[Tuple[int, str]] = []

    for lineno, raw in enumerate(src.splitlines(), 1):
        line = raw.split("#", 1)[0].strip()
        if not line:
            continue
        while True:
            m = re.match(r"([A-Za-z_.$][\w.$]*)\s*:\s*(.*)", line)
            if not m:
                break
            name, line = m.group(1), m.group(2)
            if name in labels:
                raise SimError(f"line {lineno}: duplicate label {name}")
            labels[name] = TEXT_BASE + 4 * len(instrs) if section == "text" else data_ptr
            if not line:
                break
        if not line:
            continue
        parts = line.split(None, 1)
        op = parts[0].lower()
        rest = parts[1] if len(parts) > 1 else ""
        if op.startswith("."):
            if op == ".text":
                section = "text"
            elif op in (".data", ".bss", ".sbss", ".sdata", ".rodata"):
                section = "data"
            elif op == ".section":
                name = rest.split(",")[0].strip().strip('"')
                section = "text" if name.startswith(".text") else "data"
            elif op in (".globl", ".global", ".type", ".size", ".file", ".ident", ".option",
                        ".attribute", ".addrsig", ".addrsig_sym", ".local", ".weak") or op.startswith(".cfi"):
                pass
            elif op in (".align", ".p2align", ".balign"):
                v = int(rest.split(",")[0], 0)
                n = v if op == ".balign" else 1 << v
                data_ptr = (data_ptr + n - 1) // n * n
            elif op in (".comm", ".lcomm"):
                name, size = [x.strip() for x in rest.split(",")[:2]]
                data_ptr = (data_ptr + 3) // 4 * 4
                labels[name] = data_ptr
                for a in range(data_ptr, data_ptr + int(size, 0)):
                    mem.store(a, 1, 0)
                data_ptr += int(size, 0)
            elif op in (".zero", ".space", ".skip"):
                n = int(rest, 0)
                for a in range(data_ptr, data_ptr + n):
                    mem.store(a, 1, 0)
                data_ptr += n
            elif op == ".word":
                for v in split_operands(rest):
                    if re.fullmatch(r"-?\d+|0[xX][0-9a-fA-F]+", v):
                        mem.store(data_ptr, 4, int(v, 0))
                    else:
                        data_label_refs.append((data_ptr, v))
                        mem.store(data_ptr, 4, 0)
                    data_ptr += 4
            elif op == ".byte":
                for v in split_operands(rest):
                    mem.store(data_ptr, 1, int(v, 0))
                    data_ptr += 1
            elif op in (".asciz", ".string", ".ascii"):
                s = bytes(rest.strip()[1:-1], "utf-8").decode("unicode_escape").encode("latin-1")
                if op != ".ascii":
                    s += b"\0"
                for b in s:
                    mem.store(data_ptr, 1, b)
                    data_ptr += 1
            else:
                raise SimError(f"line {lineno}: unsupported directive {op}")
            continue
        if section != "text":
            raise SimError(f"line {lineno}: instruction in .data section")
        ins = Instr(op, split_operands(rest), lineno, line)
        ins.addr = TEXT_BASE + 4 * len(instrs)
        instrs.append(ins)

    for addr, name in data_label_refs:
        if name not in labels:
            raise SimError(f"undefined label {name} in .word")
        mem.store(addr, 4, labels[name])
    return instrs, labels, mem


# --------------------------------------------------------------------------
# CPU
# --------------------------------------------------------------------------

class CPU:
    def __init__(self, instrs: List[Instr], labels: Dict[str, int], mem: Memory,
                 out=sys.stdout, trace: bool = False, max_steps: int = 50_000_000,
                 check_align: bool = False) -> None:
        self.instrs, self.labels, self.mem = instrs, labels, mem
        self.regs = [0] * 32
        self.regs[2] = STACK_TOP
        self.pc = labels.get("_start", labels.get("main", TEXT_BASE))
        self.out = out
        self.trace = trace
        self.max_steps = max_steps
        self.check_align = check_align
        self.steps = 0
        self.min_sp = STACK_TOP
        self.exit_code: Optional[int] = None

    # -- operand decoding -----------------------------------------------------
    def reg(self, name: str) -> int:
        try:
            return REGNUM[name.lower()]
        except KeyError:
            raise SimError(f"unknown register {name!r}")

    def imm(self, text: str) -> int:
        text = text.strip()
        m = re.fullmatch(r"%(lo|hi)\((.+)\)", text)
        if m:
            v = self.imm(m.group(2))
            if m.group(1) == "hi":
                return ((v + 0x800) >> 12) & 0xFFFFF
            lo = v & 0xFFF
            return lo - 0x1000 if lo & 0x800 else lo      # %lo is a signed 12-bit field
        if re.fullmatch(r"-?0[xX][0-9a-fA-F]+|-?\d+", text):
            return int(text, 0)
        if re.fullmatch(r"'(\\.|.)'", text):
            body = text[1:-1]
            return {"\\n": 10, "\\t": 9, "\\0": 0}.get(body, ord(body[-1]))
        if text in self.labels:
            return self.labels[text]
        m = re.fullmatch(r"(\w+)\s*([+-])\s*(\d+)", text)
        if m and m.group(1) in self.labels:
            return self.labels[m.group(1)] + int(m.group(2) + m.group(3))
        raise SimError(f"cannot evaluate immediate {text!r}")

    def memop(self, text: str) -> int:
        m = re.fullmatch(r"(-?[\w'%()]*)\s*\(\s*(\w+)\s*\)", text.strip())
        if m:
            off = self.imm(m.group(1)) if m.group(1) else 0
            return u32(self.regs[self.reg(m.group(2))] + off)
        return u32(self.imm(text))          # bare label: absolute address

    def set(self, rd: str, val: int) -> None:
        r = self.reg(rd)
        if r != 0:
            self.regs[r] = u32(val)

    def get(self, rs: str) -> int:
        return s32(self.regs[self.reg(rs)])

    def getu(self, rs: str) -> int:
        return self.regs[self.reg(rs)]

    # -- execution ------------------------------------------------------------
    def run(self) -> int:
        while self.exit_code is None:
            idx = (self.pc - TEXT_BASE) // 4
            if self.pc % 4 or not (0 <= idx < len(self.instrs)):
                raise SimError(f"pc=0x{self.pc:08x} is outside the program")
            ins = self.instrs[idx]
            if self.steps >= self.max_steps:
                raise SimError(f"step limit {self.max_steps} reached at line {ins.line}")
            if self.trace:
                print(f"  [{self.steps:6}] 0x{self.pc:08x}  {ins.text:32} "
                      f"a0={s32(self.regs[10])} sp=0x{self.regs[2]:08x}", file=sys.stderr)
            self.steps += 1
            next_pc = self.pc + 4
            try:
                jump = self.execute(ins)
            except SimError as e:
                raise SimError(f"line {ins.line} ({ins.text}): {e}") from None
            self.pc = jump if jump is not None else next_pc
            if self.regs[2] < self.min_sp:
                self.min_sp = self.regs[2]
        return self.exit_code

    def branch(self, a: List[str], cond: bool) -> Optional[int]:
        return self.imm(a[-1]) if cond else None

    def execute(self, ins: Instr) -> Optional[int]:   # returns new pc or None
        op, a = ins.op, ins.args
        g, gu, st, im = self.get, self.getu, self.set, self.imm
        # ---- arithmetic, register-register
        if op == "add":  st(a[0], g(a[1]) + g(a[2]))
        elif op == "sub": st(a[0], g(a[1]) - g(a[2]))
        elif op == "and": st(a[0], gu(a[1]) & gu(a[2]))
        elif op == "or":  st(a[0], gu(a[1]) | gu(a[2]))
        elif op == "xor": st(a[0], gu(a[1]) ^ gu(a[2]))
        elif op == "sll": st(a[0], gu(a[1]) << (gu(a[2]) & 31))
        elif op == "srl": st(a[0], gu(a[1]) >> (gu(a[2]) & 31))
        elif op == "sra": st(a[0], g(a[1]) >> (gu(a[2]) & 31))
        elif op == "slt": st(a[0], int(g(a[1]) < g(a[2])))
        elif op == "sltu": st(a[0], int(gu(a[1]) < gu(a[2])))
        # ---- M extension (RISC-V defines division by zero; no trap)
        elif op == "mul":  st(a[0], g(a[1]) * g(a[2]))
        elif op == "mulh": st(a[0], (g(a[1]) * g(a[2])) >> 32)
        elif op == "mulhu": st(a[0], (gu(a[1]) * gu(a[2])) >> 32)
        elif op == "div":
            x, y = g(a[1]), g(a[2])
            st(a[0], -1 if y == 0 else (x if (x == -2**31 and y == -1) else int(x / y)))
        elif op == "divu":
            x, y = gu(a[1]), gu(a[2])
            st(a[0], 0xFFFFFFFF if y == 0 else x // y)
        elif op == "rem":
            x, y = g(a[1]), g(a[2])
            st(a[0], x if y == 0 else (0 if (x == -2**31 and y == -1) else x - int(x / y) * y))
        elif op == "remu":
            x, y = gu(a[1]), gu(a[2])
            st(a[0], x if y == 0 else x % y)
        # ---- arithmetic, immediate
        elif op == "addi": st(a[0], g(a[1]) + im(a[2]))
        elif op == "andi": st(a[0], gu(a[1]) & u32(im(a[2])))
        elif op == "ori":  st(a[0], gu(a[1]) | u32(im(a[2])))
        elif op == "xori": st(a[0], gu(a[1]) ^ u32(im(a[2])))
        elif op == "slli": st(a[0], gu(a[1]) << (im(a[2]) & 31))
        elif op == "srli": st(a[0], gu(a[1]) >> (im(a[2]) & 31))
        elif op == "srai": st(a[0], g(a[1]) >> (im(a[2]) & 31))
        elif op == "slti": st(a[0], int(g(a[1]) < im(a[2])))
        elif op == "sltiu": st(a[0], int(gu(a[1]) < u32(im(a[2]))))
        elif op == "lui":  st(a[0], im(a[1]) << 12)
        elif op == "auipc": st(a[0], self.pc + (im(a[1]) << 12))
        # ---- pseudo-instructions
        elif op == "li":  st(a[0], im(a[1]))
        elif op == "la":  st(a[0], im(a[1]))
        elif op == "mv":  st(a[0], g(a[1]))
        elif op == "neg": st(a[0], -g(a[1]))
        elif op == "not": st(a[0], ~gu(a[1]))
        elif op == "seqz": st(a[0], int(g(a[1]) == 0))
        elif op == "snez": st(a[0], int(g(a[1]) != 0))
        elif op == "sltz": st(a[0], int(g(a[1]) < 0))
        elif op == "sgtz": st(a[0], int(g(a[1]) > 0))
        elif op == "nop": pass
        # ---- loads and stores
        elif op == "lw":  st(a[0], self.mem.load(self.memop(a[1]), 4, True))
        elif op == "lh":  st(a[0], self.mem.load(self.memop(a[1]), 2, True))
        elif op == "lhu": st(a[0], self.mem.load(self.memop(a[1]), 2, False))
        elif op == "lb":  st(a[0], self.mem.load(self.memop(a[1]), 1, True))
        elif op == "lbu": st(a[0], self.mem.load(self.memop(a[1]), 1, False))
        elif op == "sw":  self.mem.store(self.memop(a[1]), 4, gu(a[0]))
        elif op == "sh":  self.mem.store(self.memop(a[1]), 2, gu(a[0]))
        elif op == "sb":  self.mem.store(self.memop(a[1]), 1, gu(a[0]))
        # ---- control flow
        elif op == "beq":  return self.branch(a, g(a[0]) == g(a[1]))
        elif op == "bne":  return self.branch(a, g(a[0]) != g(a[1]))
        elif op == "blt":  return self.branch(a, g(a[0]) < g(a[1]))
        elif op == "bge":  return self.branch(a, g(a[0]) >= g(a[1]))
        elif op == "bltu": return self.branch(a, gu(a[0]) < gu(a[1]))
        elif op == "bgeu": return self.branch(a, gu(a[0]) >= gu(a[1]))
        elif op == "bgt":  return self.branch(a, g(a[0]) > g(a[1]))
        elif op == "ble":  return self.branch(a, g(a[0]) <= g(a[1]))
        elif op == "beqz": return self.branch(a, g(a[0]) == 0)
        elif op == "bnez": return self.branch(a, g(a[0]) != 0)
        elif op == "blez": return self.branch(a, g(a[0]) <= 0)
        elif op == "bgez": return self.branch(a, g(a[0]) >= 0)
        elif op == "bltz": return self.branch(a, g(a[0]) < 0)
        elif op == "bgtz": return self.branch(a, g(a[0]) > 0)
        elif op == "j":    return im(a[0])
        elif op == "jal":
            if len(a) == 1:
                st("ra", self.pc + 4)
                return im(a[0])
            st(a[0], self.pc + 4)
            return im(a[1])
        elif op == "jalr":
            if len(a) == 1:                    # jalr rs  ==  jalr ra, 0(rs)
                target = gu(a[0])
                st("ra", self.pc + 4)
            else:
                target = self.memop(a[1]) if "(" in a[1] else u32(g(a[1]) + im(a[2]))
                st(a[0], self.pc + 4)
            return target & ~1
        elif op == "jr":   return gu(a[0])
        elif op == "call":
            if self.check_align and self.regs[2] % 16:
                raise SimError(f"sp=0x{self.regs[2]:08x} is not 16-byte aligned at call (psABI)")
            st("ra", self.pc + 4)
            return im(a[0])
        elif op == "tail": return im(a[0])
        elif op == "ret":  return gu("ra")
        # ---- environment call: the only door out of the machine
        elif op == "ecall":
            n = self.regs[17]
            if n == 1:
                self.out.write(str(s32(self.regs[10])))
            elif n == 11:
                self.out.write(chr(self.regs[10] & 0xFF))
            elif n == 10:
                self.exit_code = 0
            elif n == 93:
                self.exit_code = self.regs[10] & 0xFF
            else:
                raise SimError(f"unsupported ecall a7={n}")
        elif op == "ebreak":
            raise SimError("ebreak")
        else:
            raise SimError(f"unsupported instruction {op!r}")
        return None


def main(argv: List[str]) -> int:
    if len(argv) < 2 or argv[1] in ("-h", "--help"):
        print(__doc__.strip().splitlines()[-1], file=sys.stderr)
        return 2
    with open(argv[1], encoding="utf-8") as fh:
        src = fh.read()
    max_steps = int(argv[argv.index("--max-steps") + 1]) if "--max-steps" in argv else 50_000_000
    try:
        instrs, labels, mem = parse_program(src)
        cpu = CPU(instrs, labels, mem, trace="--trace" in argv, max_steps=max_steps,
                  check_align="--check-align" in argv)
        code = cpu.run()
    except SimError as e:
        sys.stdout.flush()
        print(f"\nrv32sim: {e}", file=sys.stderr)
        return 70
    sys.stdout.flush()
    if "--stats" in argv:
        print(f"\n[rv32sim] {len(instrs)} instructions in .text, {cpu.steps} executed, "
              f"max stack {STACK_TOP - cpu.min_sp} bytes, exit code {code}", file=sys.stderr)
    return code


if __name__ == "__main__":
    sys.exit(main(sys.argv))
