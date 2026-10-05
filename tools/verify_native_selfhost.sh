#!/usr/bin/env bash
# Builds selfhost/compiler.st with the production native LLVM compiler, then runs every
# program in examples/native_selfhost through the SELF-HOSTED native LLVM path,
# compiles the emitted IR with clang, runs it, and checks the exit code.
#
# Override STABLEC=/path/to/stablec to use a different stage-0 compiler.
set -u
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
STABLEC="${STABLEC:-$ROOT/build/stablec}"
CLANG="${STABLE_CLANG:-clang}"
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT

"$STABLEC" "$ROOT/selfhost/compiler.st" --backend=llvm -o "$WORK/selfhost" >/dev/null || { echo "FAIL: could not build selfhost/compiler.st"; exit 1; }

pass=0; fail=0
while read -r name expect; do
  case "$name" in ''|'#'*) continue;; esac
  STABLE_SELFHOST_INPUT="$ROOT/examples/native_selfhost/$name.st" "$WORK/selfhost" > "$WORK/$name.ll" 2>&1
  if "$CLANG" "$WORK/$name.ll" -o "$WORK/$name" 2>/dev/null; then
    "$WORK/$name" >/dev/null 2>&1; got=$?
  else
    got=NO_BINARY
  fi
  if [ "$got" = "$expect" ]; then pass=$((pass+1)); echo "PASS $name"; else fail=$((fail+1)); echo "FAIL $name (got $got, want $expect)"; fi
done < "$ROOT/examples/native_selfhost/EXPECTED.txt"
echo "native selfhost: $pass passed, $fail failed"
[ "$fail" = 0 ]
