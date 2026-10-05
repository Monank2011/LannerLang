#!/usr/bin/env bash
# Verifies that the self-hosted native LLVM readFile builtin fails safely when
# the requested file cannot be opened. The exact trap exit code is platform- and
# runtime-dependent, so the invariant is simply a non-zero process status.
set -u
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
STABLEC="${STABLEC:-$ROOT/build/stablec}"
CLANG="${STABLE_CLANG:-clang}"
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT

"$STABLEC" "$ROOT/selfhost/compiler.st" --backend=llvm -o "$WORK/selfhost" >/dev/null || { echo "FAIL: could not build selfhost/compiler.st"; exit 1; }
STABLE_SELFHOST_INPUT="$ROOT/examples/native_selfhost/read_file_missing.st" "$WORK/selfhost" > "$WORK/read_file_missing.ll" 2>&1 || { echo "FAIL: native selfhost emitter failed"; exit 1; }
"$CLANG" "$WORK/read_file_missing.ll" -o "$WORK/read_file_missing" >/dev/null 2>&1 || { echo "FAIL: emitted LLVM did not compile"; exit 1; }
set +e
"$WORK/read_file_missing" >/dev/null 2>&1
rc=$?
set -e
if [ "$rc" -eq 0 ]; then
  echo "FAIL: missing readFile unexpectedly returned success"
  exit 1
fi
echo "PASS read_file_missing (non-zero trap status: $rc)"
