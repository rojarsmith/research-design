#!/bin/sh
# run.sh - Lab 09: build the harness clients and check a display server
# against them, with no one at the keyboard.
#
# usage: sh run.sh [server-program]
#
# Where the server comes from, first match wins:
#   1. the argument, or $SERVER: a program that is already built;
#   2. $SOLUTION_DIR (default: ../solution, next to this folder): your
#      source. It is copied to $OUT/server-src and built there. With a
#      Makefile, "make" must leave a program called server, and gets
#      PROTO_DIR=<this folder> so that it can include lab09-protocol.h.
#      Without one, every *.c in it is compiled into one program with
#      gcc -O2 -Wall -Wextra -I<this folder>.
# Without a server, the clients are still built, and the check is skipped
# with a message (exit status 0).
#
# Builds, logs and frames go to $OUT (default: a new directory under /tmp).
# Arguments for check.py go in $CHECK_ARGS, for example
#   CHECK_ARGS="--only damage" sh run.sh
# Needs gcc and python3, nothing else.
set -u
HERE=$(cd "$(dirname "$0")" && pwd)
OUT=${OUT:-$(mktemp -d /tmp/lab09.XXXXXX)}
SOLUTION_DIR=${SOLUTION_DIR:-$HERE/../solution}
SERVER=${1:-${SERVER:-}}
CHECK_ARGS=${CHECK_ARGS:-}
mkdir -p "$OUT/bin" || exit 1

echo "== environment"
uname -r
nproc
gcc --version | head -n 1
python3 --version

echo
echo "== building the harness clients in $OUT/bin"
for c in good-anim good-alpha bad-crash bad-noread bad-damage bad-unsealed; do
  gcc -O2 -Wall -Wextra -o "$OUT/bin/$c" "$HERE/$c.c" "$HERE/clientlib.c" || exit 1
done
echo "built: good-anim good-alpha bad-crash bad-noread bad-damage bad-unsealed"

if [ -z "$SERVER" ]; then
  if [ -d "$SOLUTION_DIR" ] && { [ -f "$SOLUTION_DIR/Makefile" ] || ls "$SOLUTION_DIR"/*.c > /dev/null 2>&1; }; then
    echo
    echo "== building the server from $SOLUTION_DIR"
    rm -rf "$OUT/server-src"
    cp -r "$SOLUTION_DIR" "$OUT/server-src" || exit 1
    if [ -f "$OUT/server-src/Makefile" ]; then
      make -C "$OUT/server-src" PROTO_DIR="$HERE" || exit 1
    else
      gcc -O2 -Wall -Wextra -I"$HERE" -o "$OUT/server-src/server" "$OUT/server-src"/*.c || exit 1
    fi
    SERVER=$OUT/server-src/server
  else
    echo
    echo "--- skipped the check: no server. Pass the program as an argument or in SERVER,"
    echo "--- or put your source in $SOLUTION_DIR (or point SOLUTION_DIR at it)."
    exit 0
  fi
fi
if [ ! -x "$SERVER" ]; then
  echo "--- $SERVER is not an executable program"
  exit 1
fi

echo
echo "== check.py, server $SERVER"
# shellcheck disable=SC2086
python3 "$HERE/check.py" --server "$SERVER" --bin "$OUT/bin" --work "$OUT/work" $CHECK_ARGS
status=$?
echo
echo "logs and frames are in $OUT/work"
exit $status
