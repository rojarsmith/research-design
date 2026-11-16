#!/bin/sh
# run-linux.sh - Stage 03: build and run the Linux programs of this lab.
#
# usage: sh run-linux.sh [work dir]      (default: /tmp/lab03-linux)
#
# Builds into the work dir and runs there, so the lab folder stays clean
# and may be read-only. Needs gcc; no root.
set -eu
LAB=$(cd "$(dirname "$0")" && pwd)
WORK=${1:-/tmp/lab03-linux}
mkdir -p "$WORK"
cd "$WORK"
for p in mapshare sigbus forkcow; do
  gcc -O2 -Wall -o "$p" "$LAB/$p.c"
done
uname -r
echo
echo "=== mapshare ==="
./mapshare
echo
echo "=== sigbus ==="
./sigbus
echo
echo "=== forkcow ==="
./forkcow
