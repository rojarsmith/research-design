#!/bin/sh
# run.sh - Lab 07, Linux part: build and run ofd and pagecache with no
# input. Needs gcc.
#
# usage: sh run.sh [directory] [pages] [seconds]
#   directory  where the test files go; use a disk file system such as
#              your home directory in WSL 2 (ext4), not /tmp on tmpfs
#              (default: the current directory)
#   pages      size of the mapped file in pages (default 4096, 16 MiB)
#   seconds    how long part 4 of pagecache waits for the flusher (60)
# Build outputs go to $OUT (default: a new directory under /tmp).
set -u
HERE=$(cd "$(dirname "$0")" && pwd)
OUT=${OUT:-$(mktemp -d /tmp/lab07-linux.XXXXXX)}
DIR=$(cd "${1:-.}" && pwd)
PAGES=${2:-4096}
SECS=${3:-60}

echo "== environment"
uname -r
nproc
gcc --version | head -n 1
df -T "$DIR" | tail -n 1

cd "$OUT" || exit 1
gcc -O2 -Wall -o ofd "$HERE/ofd.c" || exit 1
gcc -O2 -Wall -o pagecache "$HERE/pagecache.c" || exit 1

echo
echo "== exercise G: ./ofd $DIR"
./ofd "$DIR"
echo
echo "== exercise H: ./pagecache $DIR $PAGES $SECS"
./pagecache "$DIR" "$PAGES" "$SECS"
echo
echo "build outputs are in $OUT"
