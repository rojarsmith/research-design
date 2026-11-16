#!/bin/sh
# run.sh - Lab 05, Linux part: build the three programs and run every
# exercise once, with no input. Needs gcc, and strace for the futex count.
#
# usage: sh run.sh [seconds-per-cell]      (default 0.25)
# Build outputs go to $OUT (default: a new directory under /tmp).
set -u
HERE=$(cd "$(dirname "$0")" && pwd)
OUT=${OUT:-$(mktemp -d /tmp/lab05-linux.XXXXXX)}
SECS=${1:-0.25}

echo "== environment"
uname -r
nproc
gcc --version | head -n 1

cd "$OUT" || exit 1
gcc -O2 -Wall -pthread -o futex_mutex "$HERE/futex_mutex.c" || exit 1
gcc -O2 -Wall -pthread -o lostwake "$HERE/lostwake.c" || exit 1
gcc -O1 -g -Wall -pthread -o lockorder "$HERE/lockorder.c" || exit 1

echo
echo "== exercise D: futex mutex vs pthread_mutex"
./futex_mutex "$SECS"

if command -v strace > /dev/null; then
  for lk in pthread futex; do
    echo
    echo "== exercise D: futex system calls made by the $lk lock, 8 threads, 0.5 s, under strace"
    strace -f -c -e trace=futex -o "strace-$lk.txt" ./futex_mutex 0.5 "$lk" 8
    cat "strace-$lk.txt"
  done
fi

echo
echo "== exercise E: lost wakeups"
./lostwake 20000 5
./lostwake 20000 0

echo
echo "== exercise F: lock order"
./lockorder
./lockorder together
if gcc -O1 -g -Wall -pthread -fsanitize=thread -o lockorder-tsan "$HERE/lockorder.c" 2> tsan-build.log; then
  echo "-- under ThreadSanitizer"
  ./lockorder-tsan > tsan.log 2>&1
  if grep -q "unexpected memory mapping" tsan.log; then
    # GCC 13's ThreadSanitizer cannot handle vm.mmap_rnd_bits above 28.
    echo "   ($(cat tsan.log | head -n 1)); vm.mmap_rnd_bits is $(cat /proc/sys/vm/mmap_rnd_bits)"
    echo "   retrying with address-space randomization off (setarch -R)"
    setarch "$(uname -m)" -R ./lockorder-tsan > tsan.log 2>&1
  fi
  head -n 40 tsan.log
else
  echo "-- ThreadSanitizer build failed:"
  cat tsan-build.log
fi
echo
echo "build outputs are in $OUT"
