#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
STABLEC=${STABLEC:-"$ROOT/build/stablec"}
: "${STABLEC:?stablec path missing}"

if [ "$#" -eq 0 ]; then
  set -- "${STABLE_CLANG:-${LLVM_CC:-clang}}"
fi

for clang in "$@"; do
  echo "== LLVM compatibility check: $clang =="
  version=$($clang --version | head -1 || true)
  echo "$version"
  major=$(printf '%s\\n' "$version" | sed -n 's/.*clang version \\([0-9][0-9]*\\).*/\\1/p')
  if [ -z "$major" ]; then
    major=$(printf '%s\\n' "$version" | sed -n 's/.*LLVM version \\([0-9][0-9]*\\).*/\\1/p')
  fi
  if [ -n "$major" ] && [ "$major" -lt 15 ]; then
    echo "SKIP: LLVM $major predates opaque pointers used by Stable" >&2
    continue
  fi

  tmp=$(mktemp "${TMPDIR:-/tmp}/stable-llvm-compat.XXXXXX")
  trap 'rm -f "$tmp" "$tmp.exe"' EXIT INT TERM
  STABLE_CLANG="$clang" "$STABLEC" "$ROOT/examples/f32_numeric_literals.st" --backend=llvm --emit-llvm -o "$tmp"
  "$clang" -x ir "$tmp" -O2 -Wno-override-module -o "$tmp.exe"
  set +e
  "$tmp.exe" >/dev/null
  rc=$?
  set -e
  [ "$rc" -eq 42 ] || { echo "FAIL: f32 literal regression under $clang: expected 42, got $rc" >&2; exit 1; }

  STABLE_CLANG="$clang" "$STABLEC" "$ROOT/examples/llvm_portable_fp.st" --backend=llvm --emit-llvm -o "$tmp"
  "$clang" -x ir "$tmp" -O2 -Wno-override-module -o "$tmp.exe"
  set +e
  "$tmp.exe" >/dev/null
  rc=$?
  set -e
  # c=0.875 -> 0 and e=1.25 -> 1, so 44 is the expected result.
  [ "$rc" -eq 44 ] || { echo "FAIL: expected exit 44, got $rc" >&2; exit 1; }
  rm -f "$tmp" "$tmp.exe"
  trap - EXIT INT TERM
  echo "PASS"
done
