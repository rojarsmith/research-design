#!/bin/sh
# run.sh - Lab 07: build each xv6 variant from a clean tree and run its
# exercises with no one at the console, crashes included. Every variant
# starts from a fresh copy of xv6-riscv at commit
# fa2ddcb7ca87a77af579380a6eafd5aff163b44f.
#
# Needs MIT's 6.1810 tool packages (riscv64-linux-gnu-gcc,
# qemu-system-misc), plus gcc for the host, make, perl, patch and expect.
# Uses Lab 03's boot driver (../../03-virtual-memory/xv6/xv6run.exp) and
# this folder's xv6crash.exp, which kills QEMU with SIGKILL.
#
# usage: [SOLUTION_DIR=dir] sh run.sh [xv6-tree] [part ...]
#   xv6-tree  a clean checkout; default /opt/xv6-riscv. It is not modified.
#   part      stock crash random break bigfile symlink mmap, and usertests
#             (default: all but usertests)
# Build trees and image copies go to $WORK (default: a new directory
# under /tmp).
#
#   stock     exercise A: ofdtest, bigfile at the stock size limit, and
#             fsinspect on the image
#   crash     exercise E: crash points 1, 2 and 3 in an append, and point 2
#             in a truncate, each followed by a boot that recovers
#   random    exercise E: six kills at fixed delays during crashwrite loop
#   break     exercise F: the same truncate crash with the home blocks
#             installed before the commit (break-install-first.patch)
#   bigfile   exercise B, needs 0001-bigfile.patch            [SOLUTION_DIR]
#   symlink   exercise C, needs 0001 and 0002-symlink.patch   [SOLUTION_DIR]
#   mmap      exercise D, needs 0001, 0002 and 0003-mmap.patch [SOLUTION_DIR]
#   usertests usertests -q on the mmap kernel (slow: writebig alone writes
#             a maximum-size file)
#
# Your solutions are not part of this folder. Save them as patches
# against the pinned commit, each on top of the one before:
# 0001-bigfile.patch, 0002-symlink.patch, 0003-mmap.patch. Put them in
# xv6/solution/ (git ignores it) or point SOLUTION_DIR at them. A part
# whose patches are missing is skipped with a message.
set -eu
HERE=$(cd "$(dirname "$0")" && pwd)
RUNEXP=$(cd "$HERE/../../03-virtual-memory/xv6" && pwd)/xv6run.exp
CRASHEXP=$HERE/xv6crash.exp
SRC=$(cd "${1:-/opt/xv6-riscv}" && pwd)
[ $# -gt 0 ] && shift
PARTS=${*:-stock crash random break bigfile symlink mmap}
WORK=${WORK:-$(mktemp -d /tmp/lab07.XXXXXX)}
mkdir -p "$WORK"
SOLUTION_DIR=${SOLUTION_DIR:-$HERE/solution}
PROGS="ofdtest bigfile crashwrite symlinktest mmaptest"

# need <patch>...: are all of these in SOLUTION_DIR?
need() {
  for p in "$@"; do
    if [ ! -f "$SOLUTION_DIR/$p" ]; then
      echo "--- SKIPPED: needs $* in SOLUTION_DIR ($SOLUTION_DIR)."
      echo "    Reference solutions are not shipped. Write your own, save it"
      echo "    under that name, or point SOLUTION_DIR at your patches."
      return 1
    fi
  done
}

# build <name> <patch>...: copy the clean tree, apply the patches in
# order (a name without a slash is looked up in SOLUTION_DIR), add the
# programs that compile against it, and build the kernel, the file
# system image and fsinspect. fs.pristine keeps the fresh image.
build() {
  dir=$WORK/$1
  shift
  rm -rf "$dir"
  cp -r "$SRC" "$dir"
  for p in "$@"; do
    case $p in */*) ;; *) p=$SOLUTION_DIR/$p ;; esac
    echo "--- applying $(basename "$p")"
    patch -s -d "$dir" -p1 < "$p"
  done
  for prog in $PROGS; do
    case $prog in
    symlinktest) grep -q 'int symlink(' "$dir/user/user.h" || continue ;;
    mmaptest) grep -q 'mmap(' "$dir/user/user.h" || continue ;;
    crashwrite) grep -q 'crashpoint(' "$dir/user/user.h" || continue ;;
    esac
    cp "$HERE/$prog.c" "$dir/user/"
    # Add the program to UPROGS unless the patch already did.
    grep -q "_$prog\\\\" "$dir/Makefile" ||
      sed -i "s|^UPROGS=\\\\|UPROGS=\\\\\n\t\$U/_$prog\\\\|" "$dir/Makefile"
  done
  if ! make -s -C "$dir" kernel/kernel fs.img > "$dir/build.log" 2>&1; then
    cat "$dir/build.log"
    exit 1
  fi
  grep '^nmeta' "$dir/build.log" || true
  gcc -O2 -Wall -I "$dir" -o "$dir/fsinspect" "$HERE/fsinspect.c"
  cp "$dir/fs.img" "$dir/fs.pristine"
}

# boot <name> <cpus> <timeout> <cmd>...: boot and run the commands.
boot() {
  dir=$WORK/$1
  cpus=$2
  t=$3
  shift 3
  echo "--- boot, CPUS=$cpus"
  (cd "$dir" && expect "$RUNEXP" "$t" "$cpus" "$@") || echo "--- boot ended with status $?"
}

# crashboot <name> <when> <cmd>...: boot with one hart, run the commands,
# and kill QEMU in the middle of the last one.
crashboot() {
  dir=$WORK/$1
  when=$2
  shift 2
  echo "--- boot, CPUS=1, then kill QEMU ($when)"
  (cd "$dir" && expect "$CRASHEXP" 120 1 "$when" "$@") || echo "--- crash boot ended with status $?"
}

# inspect <name> <fsinspect arguments>: run fsinspect on that tree's image.
inspect() {
  dir=$WORK/$1
  shift
  echo "--- host: fsinspect fs.img $*"
  (cd "$dir" && ./fsinspect fs.img "$@") || true
}

# The image after "crashwrite prepare", the starting point of every crash.
prepared() {
  dir=$WORK/$1
  if [ ! -f "$dir/fs.prepared" ]; then
    cp "$dir/fs.pristine" "$dir/fs.img"
    boot "$1" 1 60 "crashwrite prepare"
    inspect "$1" file /crashfile
    cp "$dir/fs.img" "$dir/fs.prepared"
  fi
  cp "$dir/fs.prepared" "$dir/fs.img"
}

# scenario <tree> <title> <crashwrite command> [after-recovery command...]
scenario() {
  tree=$1
  title=$2
  cmd=$3
  shift 3
  echo
  echo "=== $title"
  prepared "$tree"
  crashboot "$tree" marker "$cmd"
  inspect "$tree" log
  inspect "$tree" check
  echo "--- next boot: recover_from_log() runs in fsinit(), before init starts"
  boot "$tree" 1 60 "crashwrite check" "$@"
  inspect "$tree" check
}

for part in $PARTS; do
  echo
  echo "===================================================================="
  echo "===== $part"
  echo "===================================================================="
  case $part in
  stock)
    build stock
    inspect stock
    inspect stock file /README
    boot stock 1 120 "ofdtest" "bigfile" "bigfile 268 keep"
    inspect stock file /big.file
    inspect stock check
    ;;
  crash)
    build crash "$HERE/lab07-crashpoint.patch"
    scenario crash "append, crash point 1: the log is written, the header is not" "crashwrite append 1"
    scenario crash "append, crash point 2, k = 3: committed, 3 of 6 blocks installed" "crashwrite append 2 3"
    scenario crash "append, crash point 3: committed and installed, header not cleared" "crashwrite append 3"
    scenario crash "truncate, crash point 2, k = 1: committed, the bitmap block installed" "crashwrite trunc 2 1" "crashwrite other" "crashwrite check"
    ;;
  random)
    [ -d "$WORK/crash" ] || build crash "$HERE/lab07-crashpoint.patch"
    for secs in 2 3 4 5 6 7; do
      echo
      echo "=== kill QEMU ${secs} s into crashwrite loop (no crash point armed)"
      cp "$WORK/crash/fs.pristine" "$WORK/crash/fs.img"
      crashboot crash "$secs" "crashwrite loop"
      inspect crash log
      inspect crash check
      boot crash 1 60 "crashwrite check"
      inspect crash check
    done
    ;;
  break)
    build break "$HERE/lab07-crashpoint.patch" "$HERE/break-install-first.patch"
    scenario break "BROKEN commit(): truncate, crash point 2, k = 1, home blocks before the header" "crashwrite trunc 2 1" "crashwrite other" "crashwrite check"
    ;;
  bigfile)
    need 0001-bigfile.patch || continue
    build bigfile 0001-bigfile.patch
    inspect bigfile
    boot bigfile 1 1800 "bigfile" "bigfile 6000 keep"
    inspect bigfile file /big.file
    inspect bigfile check
    ;;
  symlink)
    need 0001-bigfile.patch 0002-symlink.patch || continue
    build symlink 0001-bigfile.patch 0002-symlink.patch
    boot symlink 1 120 "symlinktest" "symlinktest"
    inspect symlink check
    ;;
  mmap)
    need 0001-bigfile.patch 0002-symlink.patch 0003-mmap.patch || continue
    build mmap 0001-bigfile.patch 0002-symlink.patch 0003-mmap.patch
    boot mmap 1 300 "mmaptest"
    boot mmap 3 300 "mmaptest"
    inspect mmap check
    ;;
  usertests)
    need 0001-bigfile.patch 0002-symlink.patch 0003-mmap.patch || continue
    [ -d "$WORK/mmap" ] || build mmap 0001-bigfile.patch 0002-symlink.patch 0003-mmap.patch
    boot mmap 3 3600 "usertests -q"
    ;;
  *)
    echo "unknown part: $part"
    exit 1
    ;;
  esac
done
echo
echo "build trees are in $WORK"
