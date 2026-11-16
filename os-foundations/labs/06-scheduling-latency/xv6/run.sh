#!/bin/sh
# run.sh - Lab 06: build the round-robin kernel and, if you have written it,
# your priority kernel, and run schedtest on each with no one at the
# console. Every build starts from a fresh copy of a clean xv6-riscv tree
# at commit fa2ddcb7ca87a77af579380a6eafd5aff163b44f.
#
# Needs MIT's 6.1810 tool packages (riscv64-linux-gnu-gcc,
# qemu-system-misc), plus make, perl, patch and expect. Uses the boot
# script from lab 05 (../../05-synchronization/xv6/xv6run.exp).
#
# usage: sh run.sh [xv6-tree] [variant ...]
#   xv6-tree  a clean checkout; default /opt/xv6-riscv. It is not modified.
#   variant   rr, and prio with SOLUTION_DIR set (default: both)
# Build trees go to $WORK (default: a new directory under /tmp).
#
# Your scheduler from exercise B is not part of this folder. To run the
# prio variant, save it as a patch on top of lab06-schedstat.patch, named
# priority-sched.patch, in a directory, and set SOLUTION_DIR to it.
set -eu
HERE=$(cd "$(dirname "$0")" && pwd)
EXP="$HERE/../../05-synchronization/xv6/xv6run.exp"
SRC=${1:-/opt/xv6-riscv}
[ $# -gt 0 ] && shift
VARIANTS=${*:-rr prio}
WORK=${WORK:-$(mktemp -d /tmp/lab06.XXXXXX)}
SOLUTION_DIR=${SOLUTION_DIR:-}

build() {
  dir="$WORK/$1"
  shift
  rm -rf "$dir"
  cp -r "$SRC" "$dir"
  for p in "$@"; do
    echo "--- applying $(basename "$p")"
    patch -s -d "$dir" -p1 < "$p"
  done
  cp "$HERE/schedtest.c" "$dir/user/"
  if ! make -s -C "$dir" kernel/kernel fs.img > "$dir/build.log" 2>&1; then
    cat "$dir/build.log"
    exit 1
  fi
}

boot() {
  dir="$WORK/$1"
  cpus=$2
  shift 2
  echo "--- booting with CPUS=$cpus"
  (cd "$dir" && expect "$EXP" 300 "$cpus" "$@") || echo "--- boot ended with status $?"
}

for v in $VARIANTS; do
  echo "===== $v ====="
  case $v in
  rr)
    build rr "$HERE/lab06-schedstat.patch"
    ;;
  prio)
    if [ -z "$SOLUTION_DIR" ] || [ ! -f "$SOLUTION_DIR/priority-sched.patch" ]; then
      echo "--- skipped: this variant needs your own priority-sched.patch; set SOLUTION_DIR to the directory that holds it"
      continue
    fi
    build prio "$HERE/lab06-schedstat.patch" "$SOLUTION_DIR/priority-sched.patch"
    ;;
  *)
    echo "unknown variant: $v"
    exit 1
    ;;
  esac
  boot "$v" 1 "schedtest 3 50 same" "schedtest 3 50 inter" "schedtest 3 50 starve"
  boot "$v" 3 "schedtest 3 50 same" "schedtest 3 50 inter"
done
echo "build trees are in $WORK"
