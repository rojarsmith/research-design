#!/bin/sh
# run-linux.sh - Stage 04: the parts of the Linux exercises that need no
# kernel module and no root. Builds in the work dir, so the lab folder
# may be read-only.
#
# usage: sh run-linux.sh [work dir]      (default: /tmp/lab04-linux)
#
#   1. compile tickread (the module's reader) to check that it builds
#   2. timerfd_ticks: 10 ticks at 100 ms, then a slow reader
set -eu
LAB=$(cd "$(dirname "$0")" && pwd)
WORK=${1:-/tmp/lab04-linux}
mkdir -p "$WORK"
cd "$WORK"
uname -r
gcc -O2 -Wall -o tickread "$LAB/tickread.c" && echo "tickread.c compiles"
gcc -O2 -Wall -pthread -o timerfd_ticks "$LAB/timerfd_ticks.c"
echo
echo "=== timerfd_ticks -n 10 ==="
./timerfd_ticks -n 10
echo
echo "=== timerfd_ticks -n 5 -s 250 (a slow reader) ==="
./timerfd_ticks -n 5 -s 250
