#!/usr/bin/env python3
"""Differential test: every example is compiled twice and must behave the same.

  path A: tinyc.py  -> RV32IM assembly -> rv32sim.py      (our toy machine)
  path B: host C compiler (cc) + runtime.c -> native binary (the real machine)
  path C: clang --target=riscv32 -O2 -S + runtime_rv32.s -> rv32sim.py
          (a production compiler targeting our toy machine; skipped when the
          installed clang has no riscv32 backend)

Identical stdout and exit status for both paths is the observable meaning of
"C is a portable contract": the source said what should happen, and two very
different compiler + processor pairs agreed.
"""
import os
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
PY = sys.executable


def run(cmd, **kw):
    return subprocess.run(cmd, capture_output=True, text=True, **kw)


def main() -> int:
    cc = shutil.which("cc") or shutil.which("gcc") or shutil.which("clang")
    clang = shutil.which("clang")
    if clang and "riscv32" not in run([clang, "-print-targets"]).stdout:
        clang = None
    examples = sorted(f for f in os.listdir(os.path.join(HERE, "examples")) if f.endswith(".c"))
    failures = 0
    with tempfile.TemporaryDirectory() as tmp:
        for name in examples:
            src = os.path.join(HERE, "examples", name)
            asm = os.path.join(tmp, name[:-2] + ".s")
            comp = run([PY, os.path.join(HERE, "tinyc.py"), src, "-o", asm])
            if comp.returncode:
                print(f"FAIL {name}: tinyc\n{comp.stderr}")
                failures += 1
                continue
            sim = run([PY, os.path.join(HERE, "rv32sim.py"), asm, "--stats"])
            if cc is None:
                print(f"SIM  {name}: exit {sim.returncode}  {sim.stderr.strip()}")
                print(sim.stdout, end="")
                continue
            exe = os.path.join(tmp, name[:-2])
            build = run([cc, "-std=c11", "-w", "-include", os.path.join(HERE, "runtime.h"),
                         src, os.path.join(HERE, "runtime.c"), "-o", exe])
            if build.returncode:
                print(f"FAIL {name}: host cc\n{build.stderr}")
                failures += 1
                continue
            host = run([exe])
            same = (sim.stdout, sim.returncode) == (host.stdout, host.returncode)
            status = "PASS" if same else "FAIL"
            failures += not same
            stats = sim.stderr.strip().replace("[rv32sim] ", "")
            print(f"{status} {name:16} exit sim={sim.returncode} host={host.returncode}  {stats}")
            if not same:
                print("  sim :", repr(sim.stdout))
                print("  host:", repr(host.stdout))
                if sim.stderr:
                    print("  ", sim.stderr.strip())
            if clang is None:
                continue
            casm = os.path.join(tmp, name[:-2] + "_clang.s")
            cb = run([clang, "--target=riscv32", "-march=rv32im", "-mabi=ilp32", "-O2", "-S",
                      "-fno-asynchronous-unwind-tables", "-w", "-include",
                      os.path.join(HERE, "runtime.h"), src, "-o", casm])
            if cb.returncode:
                print(f"FAIL {name}: clang riscv32\n{cb.stderr}")
                failures += 1
                continue
            with open(casm, "a") as fh, open(os.path.join(HERE, "runtime_rv32.s")) as rt:
                fh.write(rt.read())
            csim = run([PY, os.path.join(HERE, "rv32sim.py"), casm, "--stats"])
            same = (csim.stdout, csim.returncode) == (host.stdout, host.returncode)
            failures += not same
            stats = csim.stderr.strip().replace("[rv32sim] ", "")
            print(f"{'PASS' if same else 'FAIL'} {name:16} clang -O2 on rv32sim: {stats}")
            if not same:
                print("  clang-sim:", repr(csim.stdout), csim.stderr.strip())
    if cc is None:
        print("note: no host C compiler found; simulator outputs shown without comparison")
    print(f"{len(examples) - failures}/{len(examples)} examples agree")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
