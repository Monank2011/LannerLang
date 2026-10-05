#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${STABLE_BUILD_DIR:-$ROOT_DIR/build}"
CLANG="${STABLE_BOOTSTRAP_CLANG:-$(command -v clang || true)}"
if [[ -z "$CLANG" ]]; then
  echo "error: clang is required for the typed self-host frontend check" >&2
  exit 1
fi
if [[ ! -x "$BUILD_DIR/stablec" ]]; then
  cmake -S "$ROOT_DIR" -B "$BUILD_DIR" -DSTABLE_BUILD_TESTS=ON -DSTABLE_ENABLE_LEGACY_HIR=OFF
  cmake --build "$BUILD_DIR" -j"${CMAKE_BUILD_PARALLEL_LEVEL:-2}"
fi

WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT
STAGE1="$WORK/stage1"

printf '[1/4] stage-0 -> typed self-host compiler\n'
"$BUILD_DIR/stablec" "$ROOT_DIR/selfhost/compiler.st" --backend=llvm -o "$STAGE1"

printf '[2/4] typed aggregate registry path\n'
(
  cd "$ROOT_DIR"
  STABLE_SELFHOST_INPUT=examples/native_selfhost/fn_struct_return.st "$STAGE1" > "$WORK/aggregate.ll"
)
"$CLANG" "$WORK/aggregate.ll" -o "$WORK/aggregate" >/dev/null 2>&1
set +e
"$WORK/aggregate" >/dev/null
rc=$?
set -e
[[ "$rc" -eq 42 ]]

printf '[3/4] typed unknown nominal rejection\n'
cat > "$WORK/unknown_type.st" <<'EOF'
main() i32:
    x: DefinitelyNotAType = 1
    return x
EOF
set +e
(
  cd "$ROOT_DIR"
  STABLE_SELFHOST_INPUT="$WORK/unknown_type.st" "$STAGE1" > "$WORK/unknown.ll"
)
rc=$?
set -e
[[ "$rc" -ne 0 ]]

printf '[4/4] nested borrow rejection remains enforced\n'
set +e
(
  cd "$ROOT_DIR"
  STABLE_SELFHOST_INPUT=examples/selfhost_semantic_invalid.st "$STAGE1" > "$WORK/borrow.ll"
)
rc=$?
set -e
[[ "$rc" -ne 0 ]]

printf 'typed self-host frontend verification: PASS\n'
