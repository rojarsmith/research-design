#!/bin/sh
# run-xv6.sh - Stage 03: build the xv6 parts of this lab and run them
# without typing anything.
#
# usage: [SOLUTION_DIR=dir] sh run-xv6.sh <xv6-riscv checkout> [work dir]
#
# The checkout should be at the pinned commit (see the lab README). It is
# never modified: each run copies it to <work dir>/<name> (default work dir
# /tmp/lab03-xv6), applies your patches, copies in the test programs,
# builds, and drives QEMU with expect. Needs the Lab 00 tools plus expect
# and patch.
#
# Your patches, against the pinned commit, apply with patch -p1 and live in
# SOLUTION_DIR (default: xv6/solution/ in this lab, which git ignores):
#   0001-vmprint-vmstat.patch  exercises A and B: vmprint(), vmstat(), the
#                              fork counters, and both programs in UPROGS
#   0002-cow.patch             exercise C: copy-on-write fork, on top of 0001
# A step whose patches are missing is skipped with a message.
#
#   1. stock fork (0001):             vmprinttest, cowtest, vmprinttest fork
#   2. the same tree under GDB:       translate.gdb, with vmprinttest
#   3. copy-on-write (0001 + 0002):   cowtest, vmprinttest fork
#   4. BREAKIT=1: copy-on-write without the reference-count increment
#   5. USERTESTS=1: usertests -q on the copy-on-write kernel (minutes)
set -eu

LAB=$(cd "$(dirname "$0")" && pwd)
SRC=$(cd "$1" && pwd)
WORK=${2:-/tmp/lab03-xv6}
SOLUTION_DIR=${SOLUTION_DIR:-$LAB/solution}
mkdir -p "$WORK"

have() { # have <patch>...: are all of these in SOLUTION_DIR?
  for p in "$@"; do
    [ -f "$SOLUTION_DIR/$p" ] || return 1
  done
}

skip() { # skip <step> <patch>...
  step=$1
  shift
  echo "SKIPPED: $step needs $* in SOLUTION_DIR ($SOLUTION_DIR)."
  echo "         Reference solutions are not shipped. Write your own patch, save"
  echo "         it under that name, or point SOLUTION_DIR at your patches."
}

prepare() { # prepare <name> <patch>...
  dir=$WORK/$1
  shift
  rm -rf "$dir"
  cp -r "$SRC" "$dir"
  cd "$dir"
  make clean > /dev/null
  for p in "$@"; do
    patch -p1 --quiet < "$SOLUTION_DIR/$p"
  done
  cp "$LAB/vmprinttest.c" "$LAB/cowtest.c" user/
  make -s kernel/kernel fs.img > /dev/null
}

echo "=== 1. stock fork, with vmprint() and vmstat() ==="
if have 0001-vmprint-vmstat.patch; then
  prepare stock 0001-vmprint-vmstat.patch
  expect "$LAB/xv6run.exp" 120 1 vmprinttest cowtest "vmprinttest fork"

  echo
  echo "=== 2. hand translation, confirmed in GDB ==="
  cat > "$WORK/translate-run.gdb" << EOF
set architecture riscv:rv64
file kernel/kernel
target remote 127.0.0.1:$(make -s print-gdbport)
source $LAB/translate.gdb
EOF
  expect "$LAB/xv6gdb.exp" 120 "$WORK/translate-run.gdb" "$WORK/translate.log" vmprinttest
  echo "--- GDB output ---"
  cat "$WORK/translate.log"
else
  skip "step 1" 0001-vmprint-vmstat.patch
  echo
  echo "=== 2. hand translation, confirmed in GDB ==="
  skip "step 2 (vmprinttest calls vmprint())" 0001-vmprint-vmstat.patch
fi

echo
echo "=== 3. copy-on-write fork ==="
if have 0001-vmprint-vmstat.patch 0002-cow.patch; then
  prepare cow 0001-vmprint-vmstat.patch 0002-cow.patch
  expect "$LAB/xv6run.exp" 120 1 cowtest "vmprinttest fork"
else
  skip "step 3" 0001-vmprint-vmstat.patch 0002-cow.patch
fi

if [ "${BREAKIT:-0}" = 1 ]; then
  echo
  echo "=== 4. break it: uvmcopy() forgets krefinc() ==="
  if have 0001-vmprint-vmstat.patch 0002-cow.patch; then
    prepare breakit 0001-vmprint-vmstat.patch 0002-cow.patch
    if grep -q '^    krefinc((void \*)pa);$' kernel/vm.c; then
      sed -i '/^    krefinc((void \*)pa);$/d' kernel/vm.c
      make -s kernel/kernel > /dev/null
      expect "$LAB/xv6run.exp" 30 1 cowtest || echo "(expect exit status $?)"
    else
      echo "SKIPPED: step 4 deletes the line '    krefinc((void *)pa);' from"
      echo "         uvmcopy() in kernel/vm.c, and your patch has no such line."
      echo "         Remove your reference-count increment by hand instead."
    fi
  else
    skip "step 4" 0001-vmprint-vmstat.patch 0002-cow.patch
  fi
fi

if [ "${USERTESTS:-0}" = 1 ]; then
  echo
  echo "=== 5. usertests -q on the copy-on-write kernel ==="
  if [ -d "$WORK/cow" ] && have 0001-vmprint-vmstat.patch 0002-cow.patch; then
    cd "$WORK/cow"
    expect "$LAB/xv6run.exp" 1200 3 "usertests -q"
  else
    skip "step 5" 0001-vmprint-vmstat.patch 0002-cow.patch
  fi
fi
