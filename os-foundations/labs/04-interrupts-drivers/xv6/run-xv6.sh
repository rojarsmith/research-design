#!/bin/sh
# run-xv6.sh - Stage 04: build the xv6 parts of this lab and run them
# without typing anything.
#
# usage: [SOLUTION_DIR=dir] sh run-xv6.sh <xv6-riscv checkout> [work dir]
#
# Same rules as Lab 03's run-xv6.sh, whose expect drivers this uses: the
# checkout is copied to <work dir>/<name> (default /tmp/lab04-xv6) and
# never modified. Everything runs with one hart.
#
# Your patch, against the pinned commit, applies with patch -p1 and lives
# in SOLUTION_DIR (default: xv6/solution/ in this lab, which git ignores):
#   0001-intrstat.patch  exercises B and C: the counters, intrstat(), the
#                        per-process preemption count, both programs in UPROGS
# A step that needs the patch is skipped with a message when it is missing.
#
#   1. intrcount (typing "hello xv6" when it asks) and preempt   [patch]
#   2. uart.gdb: a received character, from the PLIC to consoleintr()
#      (runs on the stock tree when the patch is missing)
#   3. timer.gdb: the timer interrupt that preempts "preempt 10" [patch]
#   4. break it: usertrap() no longer yields on a timer interrupt [patch]
set -eu

LAB=$(cd "$(dirname "$0")" && pwd)
DRV=$(cd "$LAB/../../03-virtual-memory/xv6" && pwd)
SRC=$(cd "$1" && pwd)
WORK=${2:-/tmp/lab04-xv6}
SOLUTION_DIR=${SOLUTION_DIR:-$LAB/solution}
PATCH=0001-intrstat.patch
mkdir -p "$WORK"

if [ -f "$SOLUTION_DIR/$PATCH" ]; then
  have_patch=1
else
  have_patch=0
fi

skip() { # skip <step>
  echo "SKIPPED: $1 needs $PATCH in SOLUTION_DIR ($SOLUTION_DIR)."
  echo "         Reference solutions are not shipped. Write your own patch, save"
  echo "         it under that name, or point SOLUTION_DIR at your patches."
}

prepare() { # prepare <name> [patched]
  dir=$WORK/$1
  rm -rf "$dir"
  cp -r "$SRC" "$dir"
  cd "$dir"
  make clean > /dev/null
  if [ "${2:-}" = patched ]; then
    patch -p1 --quiet < "$SOLUTION_DIR/$PATCH"
    cp "$LAB/intrcount.c" "$LAB/preempt.c" user/
  fi
  make -s kernel/kernel fs.img > /dev/null
}

gdbrun() { # gdbrun <script> <xv6 command>...
  script=$1
  shift
  cat > "$WORK/run.gdb" << EOF
set architecture riscv:rv64
file kernel/kernel
target remote 127.0.0.1:$(make -s print-gdbport)
source $LAB/$script
EOF
  expect "$DRV/xv6gdb.exp" 180 "$WORK/run.gdb" "$WORK/gdb.log" "$@"
  echo "--- GDB output ---"
  cat "$WORK/gdb.log"
}

echo "=== 1. interrupt counts and preemption ==="
if [ $have_patch = 1 ]; then
  prepare intr patched
  expect "$DRV/xv6run.exp" 180 1 intrcount "<hello xv6" preempt
else
  skip "step 1 (intrcount and preempt call intrstat())"
  prepare stock
fi

echo
echo "=== 2. a received character, under GDB ==="
gdbrun uart.gdb "echo x"

echo
echo "=== 3. the timer interrupt that preempts, under GDB ==="
if [ $have_patch = 1 ]; then
  gdbrun timer.gdb "preempt 10"
else
  skip "step 3 (timer.gdb runs preempt and reads p->preempted)"
fi

echo
echo "=== 4. break it: no yield() on a timer interrupt from user mode ==="
if [ $have_patch = 1 ]; then
  prepare noyield patched
  if grep -q '^    yield(); // lab 04: the preemption$' kernel/trap.c; then
    sed -i -e '/^    yield(); \/\/ lab 04: the preemption$/d' \
      -e '/^    p->preempted++;$/d' kernel/trap.c
    echo "usertrap() after the edit:"
    grep -n -A4 "if (which_dev == 2) {" kernel/trap.c
    make -s kernel/kernel > /dev/null
    expect "$DRV/xv6run.exp" 180 1 preempt
  else
    echo "SKIPPED: step 4 deletes the line '    yield(); // lab 04: the preemption'"
    echo "         from usertrap() in kernel/trap.c, and your patch has no such line."
    echo "         Remove the yield() (and your preemption count) by hand instead."
  fi
else
  skip "step 4"
fi
