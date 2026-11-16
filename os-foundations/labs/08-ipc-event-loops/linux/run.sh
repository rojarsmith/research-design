#!/bin/sh
# run.sh - Lab 08, Linux part: build every program and run each exercise
# once, with no input. Needs gcc; strace and patch for the optional steps.
#
# usage: sh run.sh
# Build outputs go to $OUT (default: a new directory under /tmp), so the lab
# folder may be read-only.
#
# SOLUTION_DIR: if it holds evserver-fdleak.patch, your fix for exercise C
# (a diff against evserver.c), the script applies it to a copy and runs the
# demo again. Without it, that step says it was skipped.
#
# The strace step needs ptrace. It traces its own child, which worked here
# under podman's default profile; if your container refuses it, add
# --cap-add SYS_PTRACE. If strace cannot run, the step says so.
set -u
HERE=$(cd "$(dirname "$0")" && pwd)
OUT=${OUT:-$(mktemp -d /tmp/lab08-linux.XXXXXX)}
SOLUTION_DIR=${SOLUTION_DIR:-}

echo "== environment"
uname -r
nproc
gcc --version | head -n 1
echo "uid $(id -u); $(grep '^Seccomp:' /proc/self/status | tr -s '\t' ' ')"

cd "$OUT" || exit 1
CC="gcc -O2 -Wall -Wextra"
$CC -o fdserver "$HERE/fdserver.c" "$HERE/fdpass.c" || exit 1
$CC -o sealtest "$HERE/sealtest.c" "$HERE/sealbuf.c" "$HERE/fdpass.c" || exit 1
$CC -pthread -o evserver "$HERE/evserver.c" "$HERE/evloop.c" "$HERE/fdpass.c" "$HERE/sealbuf.c" || exit 1
$CC -o readiness "$HERE/readiness.c" || exit 1

echo
echo "== exercise A: SCM_RIGHTS and SO_PEERCRED (fdserver)"
./fdserver

echo
echo "== exercise B: sealed memfd buffers (sealtest)"
./sealtest

echo
echo "== exercise C: one epoll loop (evserver demo)"
./evserver demo

echo
echo "== exercise C: the server's system calls while one good client talks to it"
if strace -o /dev/null true 2> /dev/null; then
  SOCK="$OUT/strace.sock"
  strace -f -o strace-evserver.txt \
    -e trace=epoll_wait,accept4,recvmsg,sendto,read,write,nanosleep \
    ./evserver serve "$SOCK" > serve.log 2>&1 &
  SPID=$!
  i=0
  while [ ! -S "$SOCK" ] && [ $i -lt 100 ]; do sleep 0.05; i=$((i + 1)); done
  sleep 0.1
  ./evserver client "$SOCK" good > client.log 2>&1
  sleep 0.1
  # Signal the server itself: strace started as "strace -o FILE PROG"
  # blocks fatal signals (its -I 3 default), so a SIGTERM to strace is lost.
  SRV=$(sed -n 's/.*server: pid \([0-9]*\) listening.*/\1/p' serve.log)
  kill -TERM "$SRV"
  wait $SPID
  # From the server accepting the client to the client's BYE (type 5),
  # lines cut to 150 columns.
  sed -n '/accept4(.*) = [0-9]/,/iov_base="\\5\\0/p' strace-evserver.txt | cut -c 1-150
  echo "-- the control data of the BUFFER message, which those lines cut off:"
  grep -m 1 'SCM_RIGHTS' strace-evserver.txt | sed 's/.*\(msg_control=\[{[^}]*}\]\).*/   \1/'
  echo "(full trace: $OUT/strace-evserver.txt; client output: $OUT/client.log)"
else
  echo "SKIPPED: strace cannot trace here (in a container, add --cap-add SYS_PTRACE)"
fi

echo
echo "== exercise C: your fix for the leak"
if [ -n "$SOLUTION_DIR" ] && [ -f "$SOLUTION_DIR/evserver-fdleak.patch" ]; then
  cp "$HERE/evserver.c" evserver-fixed.c
  chmod u+w evserver-fixed.c
  if patch -s evserver-fixed.c < "$SOLUTION_DIR/evserver-fdleak.patch" &&
    $CC -pthread -I"$HERE" -o evserver-fixed evserver-fixed.c "$HERE/evloop.c" \
      "$HERE/fdpass.c" "$HERE/sealbuf.c"; then
    ./evserver-fixed demo
  else
    echo "FAILED: $SOLUTION_DIR/evserver-fdleak.patch did not apply or build"
  fi
else
  echo "SKIPPED: no evserver-fdleak.patch in SOLUTION_DIR (see exercise C in the README)"
fi

echo
echo "== exercise D: readiness vs completion (readiness)"
./readiness

echo
echo "build outputs are in $OUT"
