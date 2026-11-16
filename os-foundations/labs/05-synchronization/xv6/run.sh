#!/bin/sh
# run.sh - Lab 05: build each xv6 variant and run its exercise with no one
# at the console. Every variant starts from a fresh copy of a clean
# xv6-riscv tree at commit fa2ddcb7ca87a77af579380a6eafd5aff163b44f.
#
# Needs MIT's 6.1810 tool packages (riscv64-linux-gnu-gcc,
# qemu-system-misc), plus make, perl, patch, sed and expect.
#
# usage: sh run.sh [xv6-tree] [variant ...]
#   xv6-tree  a clean checkout; default /opt/xv6-riscv. It is not modified.
#   variant   base window lostwake-a pushoff-nointr pushoff-nonest
#             pushoff-nointr-nocheck pushoff-nonest-nocheck
#             and, with SOLUTION_DIR set, percpu lostwake-b
#             (default: all of them)
# Build trees go to $WORK (default: a new directory under /tmp).
#
# The solutions to exercises B and D (your per-CPU allocator, your own
# broken pair) are not part of this folder. To run those variants, put
# your patches in a directory as kalloc-percpu.patch and lostwake-b.patch
# and set SOLUTION_DIR to it.
set -eu
HERE=$(cd "$(dirname "$0")" && pwd)
SRC=${1:-/opt/xv6-riscv}
[ $# -gt 0 ] && shift
VARIANTS=${*:-base percpu window lostwake-a lostwake-b pushoff-nointr pushoff-nonest pushoff-nointr-nocheck pushoff-nonest-nocheck}
WORK=${WORK:-$(mktemp -d /tmp/lab05.XXXXXX)}
SOLUTION_DIR=${SOLUTION_DIR:-}

# patchfile <name>: the path of a shipped patch, or of one in SOLUTION_DIR.
patchfile() {
  if [ -f "$HERE/$1" ]; then
    echo "$HERE/$1"
  else
    echo "$SOLUTION_DIR/$1"
  fi
}

# need_solution <patch>: true if the patch is available in SOLUTION_DIR.
need_solution() {
  if [ -n "$SOLUTION_DIR" ] && [ -f "$SOLUTION_DIR/$1" ]; then
    return 0
  fi
  echo "--- skipped: this variant needs your own $1; set SOLUTION_DIR to the directory that holds it"
  return 1
}

# build <name> <patch>...: copy the clean tree, apply the patches in order,
# add the user programs, and build the kernel and file system image.
build() {
  dir="$WORK/$1"
  shift
  rm -rf "$dir"
  cp -r "$SRC" "$dir"
  for p in "$@"; do
    echo "--- applying $p"
    patch -s -d "$dir" -p1 < "$(patchfile "$p")"
  done
  cp "$HERE/kmemstress.c" "$HERE/lostwake.c" "$dir/user/"
  if ! make -s -C "$dir" kernel/kernel fs.img > "$dir/build.log" 2>&1; then
    cat "$dir/build.log"
    exit 1
  fi
}

# boot <name> <cpus> <timeout> <cmd>...: boot that tree and run the
# commands. A boot that fails is reported and does not stop the script.
boot() {
  dir="$WORK/$1"
  cpus=$2
  t=$3
  shift 3
  echo "--- booting with CPUS=$cpus"
  (cd "$dir" && expect "$HERE/xv6run.exp" "$t" "$cpus" "$@") || echo "--- boot ended with status $?"
}

# Delete the "if (intr_get()) panic(...)" check from pop_off().
nocheck() {
  sed -i '/^  if (intr_get())$/{N;/pop_off - interruptible/d}' "$WORK/$1/kernel/spinlock.c"
  make -s -C "$WORK/$1" kernel/kernel > /dev/null 2>&1
  echo "--- removed the intr_get() check from pop_off()"
}

for v in $VARIANTS; do
  echo "===== $v ====="
  case $v in
  base)
    build base lab05-lockstat.patch
    boot base 3 300 "kmemstress" "kmemstress" "kmemstress 1 60000" "lostwake"
    boot base 1 300 "kmemstress"
    ;;
  percpu)
    need_solution kalloc-percpu.patch || continue
    build percpu lab05-lockstat.patch kalloc-percpu.patch
    boot percpu 3 300 "kmemstress" "kmemstress" "kmemstress 1 60000"
    ;;
  window)
    build window lab05-lockstat.patch lostwake-window.patch
    boot window 3 300 "lostwake" "lostwake" "lostwake"
    ;;
  lostwake-a)
    build lostwake-a lab05-lockstat.patch lostwake-a.patch
    boot lostwake-a 3 300 "lostwake" "lostwake" "lostwake"
    ;;
  lostwake-b)
    need_solution lostwake-b.patch || continue
    build lostwake-b lab05-lockstat.patch lostwake-b.patch
    boot lostwake-b 3 300 "lostwake" "lostwake" "lostwake"
    ;;
  pushoff-nointr | pushoff-nonest)
    build "$v" "$v.patch" lab05-lockstat.patch
    boot "$v" 1 30 "kmemstress"
    boot "$v" 3 30 "kmemstress"
    ;;
  pushoff-nointr-nocheck | pushoff-nonest-nocheck)
    base=${v%-nocheck}
    build "$v" "$base.patch" lab05-lockstat.patch
    nocheck "$v"
    boot "$v" 1 30 "kmemstress"
    boot "$v" 3 30 "kmemstress"
    ;;
  *)
    echo "unknown variant: $v"
    exit 1
    ;;
  esac
done
echo "build trees are in $WORK"
