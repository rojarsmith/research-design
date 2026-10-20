#!/usr/bin/env python3
"""compare_targets: run the processor C-readiness protocol across clang backends.

For each target it records, without needing any cross libc or hardware:

  * the implementation-defined contract from the preprocessor:
    CHAR_BIT, sizeof int / long / pointer / double, byte order
  * for each test program compiled with -O2 -ffreestanding -c:
    .text size, instruction count, and the undefined symbols that start with
    `__` or `__aeabi_` (compiler runtime calls: missing hardware made visible)
  * compile errors, which are the clearest "cannot run this C" verdict of all

Usage: compare_targets.py [--json out.json] [--markdown] [--targets name,name]
Requires: clang with the relevant backends, llvm-size / llvm-nm (or -NN variants).
"""
from __future__ import annotations

import json
import os
import re
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
TINYC_EXAMPLES = os.path.join(HERE, "..", "tinyc", "examples")
RUNTIME_H = os.path.join(HERE, "..", "tinyc", "runtime.h")
FEATURES = os.path.join(HERE, "features")

# name, clang triple, extra flags, one-line description
TARGETS = [
    ("avr",        "avr",               ["-mmcu=atmega328p"],            "8-bit AVR, Harvard, 32 registers"),
    ("msp430",     "msp430",            [],                               "16-bit MSP430"),
    ("cortex-m0",  "thumbv6m-none-eabi", [],                              "Armv6-M Thumb, no divide, no FPU"),
    ("cortex-m4",  "thumbv7em-none-eabi", [],                             "Armv7E-M, hardware divide, soft float"),
    ("cortex-m4f", "thumbv7em-none-eabi", ["-mfpu=fpv4-sp-d16", "-mfloat-abi=hard"], "same core, single-precision FPU enabled"),
    ("rv32e",      "riscv32",           ["-march=rv32e", "-mabi=ilp32e"], "RISC-V, 16 registers, no multiply"),
    ("rv32i",      "riscv32",           ["-march=rv32i", "-mabi=ilp32"],  "RISC-V base integer only"),
    ("rv32im",     "riscv32",           ["-march=rv32im", "-mabi=ilp32"], "RISC-V with multiply/divide (tinyc's target)"),
    ("rv32imf",    "riscv32",           ["-march=rv32imf", "-mabi=ilp32f"], "RISC-V with single-precision FPU"),
    ("rv64gc",     "riscv64",           ["-march=rv64gc", "-mabi=lp64d"], "64-bit RISC-V, general purpose, double FPU"),
    ("i386",       "i386-linux-gnu",    [],                               "32-bit x86"),
    ("x86-64",     "x86_64-linux-gnu",  [],                               "64-bit x86 (host)"),
    ("aarch64",    "aarch64-linux-gnu", [],                               "64-bit Arm"),
    ("mips32",     "mips-linux-gnu",    [],                               "32-bit MIPS, big endian"),
    ("m68k",       "m68k-linux-gnu",    [],                               "Motorola 68000 family"),
    ("sparc",      "sparc-linux-gnu",   [],                               "32-bit SPARC, big endian, register windows"),
    ("wasm32",     "wasm32",            [],                               "WebAssembly: a virtual ISA, not a CPU"),
    ("nvptx64",    "nvptx64-nvidia-cuda", [],                             "NVIDIA PTX: GPU virtual ISA"),
    ("bpf",        "bpf",               [],                               "Linux eBPF: deliberately restricted"),
]

MACROS = ["__CHAR_BIT__", "__SIZEOF_SHORT__", "__SIZEOF_INT__", "__SIZEOF_LONG__",
          "__SIZEOF_LONG_LONG__", "__SIZEOF_POINTER__", "__SIZEOF_FLOAT__",
          "__SIZEOF_DOUBLE__", "__BYTE_ORDER__"]


def tool(name: str) -> str | None:
    for cand in (name, f"{name}-18", f"{name}-19", f"{name}-17", f"{name}-20"):
        p = shutil.which(cand)
        if p:
            return p
    return None


def run(cmd: list[str]) -> subprocess.CompletedProcess:
    return subprocess.run(cmd, capture_output=True, text=True)


def macros(clang: str, triple: str, flags: list[str]) -> dict:
    r = run([clang, f"--target={triple}", *flags, "-dM", "-E", "-x", "c", os.devnull])
    found = {}
    for m in MACROS:
        mm = re.search(rf"#define {m} (\S+)", r.stdout)
        if mm:
            v = mm.group(1)
            found[m] = v.replace("__ORDER_LITTLE_ENDIAN__", "little").replace("__ORDER_BIG_ENDIAN__", "big")
    return found


def count_instructions(asm: str) -> int:
    n = 0
    for line in asm.splitlines():
        s = line.strip()
        if not s or s.startswith((".", "#", "//", ";")) or s.endswith(":") or s.startswith("@"):
            continue
        n += 1
    return n


def compile_one(clang: str, triple: str, flags: list[str], src: str, tmp: str, size_tool, nm_tool) -> dict:
    base = os.path.join(tmp, os.path.basename(src))
    common = [clang, f"--target={triple}", *flags, "-O2", "-ffreestanding", "-w",
              "-fno-asynchronous-unwind-tables", "-include", RUNTIME_H, src]
    rs = run([*common, "-S", "-o", base + ".s"])
    if rs.returncode:
        first = next((l for l in rs.stderr.splitlines() if "error:" in l), rs.stderr.strip().splitlines()[0] if rs.stderr.strip() else "error")
        msg = first.split("error:", 1)[-1].strip()
        msg = re.sub(r"0x[0-9a-f]+:? ?", "", msg)            # drop SelectionDAG node addresses
        return {"error": msg[:110] + ("..." if len(msg) > 110 else "")}
    rc = run([*common, "-c", "-o", base + ".o"])
    out = {"instructions": count_instructions(open(base + ".s", encoding="utf-8", errors="replace").read())}
    if rc.returncode == 0 and size_tool:
        sz = run([size_tool, base + ".o"]).stdout.splitlines()
        if len(sz) >= 2:
            out["text"] = int(sz[1].split()[0])
    if rc.returncode == 0 and nm_tool:
        undef = run([nm_tool, "-u", base + ".o"]).stdout.split()
        calls = sorted({s for s in undef if s not in ("U",) and (s.startswith("__") or s.startswith("_Q") or s.startswith("L___"))})
        calls = [c.lstrip("_") if c.startswith("___") else c for c in calls]
        out["libcalls"] = calls
    elif rc.returncode and triple.startswith("nvptx"):
        out["note"] = "object emission not supported; assembly only"
    return out


def main() -> int:
    clang = shutil.which("clang")
    if not clang:
        print("clang not found", file=sys.stderr)
        return 2
    size_tool, nm_tool = tool("llvm-size"), tool("llvm-nm")
    wanted = None
    if "--targets" in sys.argv:
        wanted = set(sys.argv[sys.argv.index("--targets") + 1].split(","))
    sources = sorted(os.path.join(TINYC_EXAMPLES, f) for f in os.listdir(TINYC_EXAMPLES) if f.endswith(".c"))
    sources += sorted(os.path.join(FEATURES, f) for f in os.listdir(FEATURES) if f.endswith(".c"))
    results = []
    with tempfile.TemporaryDirectory() as tmp:
        for name, triple, flags, desc in TARGETS:
            if wanted and name not in wanted:
                continue
            probe = run([clang, f"--target={triple}", *flags, "-c", "-x", "c", os.devnull, "-o", os.path.join(tmp, "probe.o")])
            if probe.returncode and ("unable to create target" in probe.stderr or "No available targets" in probe.stderr
                                     or "unknown target" in probe.stderr):
                results.append({"name": name, "triple": triple, "desc": desc, "skipped": "backend not in this clang"})
                continue
            entry = {"name": name, "triple": triple, "flags": flags, "desc": desc,
                     "macros": macros(clang, triple, flags), "programs": {}}
            for src in sources:
                entry["programs"][os.path.basename(src)] = compile_one(clang, triple, flags, src, tmp, size_tool, nm_tool)
            results.append(entry)
            print(f"done {name}", file=sys.stderr)
    if "--json" in sys.argv:
        with open(sys.argv[sys.argv.index("--json") + 1], "w", encoding="utf-8") as fh:
            json.dump({"clang": run([clang, "--version"]).stdout.splitlines()[0], "results": results}, fh, indent=1)
    print_markdown(results, run([clang, "--version"]).stdout.splitlines()[0])
    return 0


def print_markdown(results: list[dict], clang_version: str) -> None:
    print(f"clang: {clang_version}\n")
    print("## Implementation-defined contract\n")
    print("| target | CHAR_BIT | short/int/long/long long | pointer | float/double | byte order |")
    print("| --- | --- | --- | --- | --- | --- |")
    for r in results:
        if "skipped" in r:
            print(f"| {r['name']} | skipped: {r['skipped']} | | | | |")
            continue
        m = r["macros"]
        g = lambda k: m.get(k, "?")
        print(f"| {r['name']} | {g('__CHAR_BIT__')} | {g('__SIZEOF_SHORT__')}/{g('__SIZEOF_INT__')}/{g('__SIZEOF_LONG__')}/{g('__SIZEOF_LONG_LONG__')} "
              f"| {g('__SIZEOF_POINTER__')} | {g('__SIZEOF_FLOAT__')}/{g('__SIZEOF_DOUBLE__')} | {g('__BYTE_ORDER__')} |")
    programs = [p for p in next((r for r in results if "programs" in r), {"programs": {}})["programs"]]
    print("\n## .text bytes at -O2 (instructions in parentheses)\n")
    print("| target | " + " | ".join(p.replace(".c", "") for p in programs) + " |")
    print("| --- |" + " --- |" * len(programs))
    for r in results:
        if "skipped" in r:
            continue
        cells = []
        for p in programs:
            x = r["programs"][p]
            if "error" in x:
                cells.append("FAIL")
            elif "text" in x:
                cells.append(f"{x['text']} ({x['instructions']})")
            else:
                cells.append(f"({x['instructions']})")
        print(f"| {r['name']} | " + " | ".join(cells) + " |")
    print("\n## Compiler runtime calls (hardware the target lacks) and errors\n")
    print("| target | program | libcalls or error |")
    print("| --- | --- | --- |")
    for r in results:
        if "skipped" in r:
            continue
        for p in programs:
            x = r["programs"][p]
            if "error" in x:
                print(f"| {r['name']} | {p} | error: {x['error']} |")
            elif x.get("libcalls"):
                print(f"| {r['name']} | {p} | {', '.join('`' + c + '`' for c in x['libcalls'])} |")


if __name__ == "__main__":
    sys.exit(main())
