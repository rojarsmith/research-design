#!/bin/sh
# run.sh - Lab 06, Linux part: the 60 Hz timerfd wakeup, idle and under
# load, with the default policy and with SCHED_FIFO. No input needed.
#
# usage: sh run.sh [seconds-per-run] [load-threads]
#   defaults: 5 s per run, one load thread per online CPU
# SCHED_FIFO needs CAP_SYS_NICE or RLIMIT_RTPRIO; in a container, for
# example podman run --cap-add SYS_NICE. If it is refused, that run says so.
set -u
HERE=$(cd "$(dirname "$0")" && pwd)
OUT=${OUT:-$(mktemp -d /tmp/lab06-linux.XXXXXX)}
SECS=${1:-5}
LOAD=${2:-$(nproc)}

echo "== environment"
uname -r
nproc
gcc --version | head -n 1
echo "RLIMIT_RTPRIO (ulimit -r): $(ulimit -r)"
grep -E '^Cap(Eff|Bnd)' /proc/self/status
for f in /proc/sys/kernel/sched_rt_runtime_us /proc/sys/kernel/sched_rt_period_us; do
  [ -r "$f" ] && echo "$f: $(cat "$f")"
done

cd "$OUT" || exit 1
gcc -O2 -Wall -pthread -o latency "$HERE/latency.c" || exit 1

echo
echo "== 1. SCHED_OTHER, idle"
./latency other 0 "$SECS"
echo
echo "== 2. SCHED_OTHER, $LOAD load threads"
./latency other "$LOAD" "$SECS"
echo
echo "== 3. SCHED_FIFO 50, $LOAD load threads"
./latency fifo "$LOAD" "$SECS" 50
echo "(exit status $?)"
echo
echo "build outputs are in $OUT"
