#!/bin/sh
# run-module.sh - Stage 04, exercise D: build, load, exercise and unload
# the tickdev module, without typing anything.
#
# usage: sh run-module.sh [work dir]          (default: /tmp/lab04-module)
#
# Run it in a Linux VM, as a user who may use sudo, with the headers for
# the running kernel installed. Never on WSL 2 (its kernel ships no module
# headers) and never on a machine you care about: a bug in a module can
# take the whole kernel down.
#
# NOT run on the machine where this lab was written; see the lab README.
set -eu
LAB=$(cd "$(dirname "$0")" && pwd)
WORK=${1:-/tmp/lab04-module}
rm -rf "$WORK"
mkdir -p "$WORK"
cp "$LAB/tickdev.c" "$LAB/tickdev.h" "$LAB/tickread.c" "$LAB/Makefile" "$WORK/"
cd "$WORK"

uname -r
make
modinfo ./tickdev.ko | grep -E '^(name|description|parm|vermagic)'

sudo insmod ./tickdev.ko period_ms=100
trap 'sudo rmmod tickdev 2>/dev/null || true' EXIT
sudo dmesg | grep tickdev | tail -1

echo
echo "=== the device, as the driver model and udev present it ==="
ls -l /dev/tickdev
cat /sys/class/misc/tickdev/dev
ls -l /sys/class/misc/tickdev
udevadm info --query=all --name=/dev/tickdev || true

echo
echo "=== 1. one reader, 10 ticks ==="
./tickread -n 10

echo
echo "=== 2. two readers at once: both see every tick ==="
./tickread -n 5 > r1.txt &
./tickread -n 5 > r2.txt &
wait
paste r1.txt r2.txt | cut -c1-120

echo
echo "=== 3. a slow reader (sleeps 250 ms after each read) ==="
./tickread -n 5 -s 250

echo
echo "=== 4. non-blocking reads ==="
./tickread -n 3 -N

echo
echo "=== 5. a reader blocked in read(): its state and wait channel ==="
./tickread -n 30 > /dev/null &
r=$!
sleep 1
ps -o pid,stat,wchan:32,cmd -p "$r"
cat "/proc/$r/stack" 2>/dev/null || sudo cat "/proc/$r/stack" || true
kill "$r"
wait "$r" || echo "reader ended with status $? after the signal"

echo
echo "=== 6. unload ==="
grep tickdev /proc/modules
sudo rmmod tickdev
trap - EXIT
sudo dmesg | grep tickdev | tail -2
