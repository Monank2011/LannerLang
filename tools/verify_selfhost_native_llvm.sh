#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${LANNER_BUILD_DIR:-$ROOT_DIR/build}"
CLANG="${LANNER_BOOTSTRAP_CLANG:-$(command -v clang || true)}"
if [[ -z "$CLANG" ]]; then
    echo "error: clang is required for the native LLVM bootstrap check" >&2
    exit 1
fi

if [[ ! -x "$BUILD_DIR/lanner" ]]; then
    cmake -S "$ROOT_DIR" -B "$BUILD_DIR" -DLANNER_BUILD_TESTS=ON -DLANNER_ENABLE_LEGACY_HIR=OFF
    cmake --build "$BUILD_DIR" -j"${CMAKE_BUILD_PARALLEL_LEVEL:-2}"
fi

STAGE1="$BUILD_DIR/stage1-native-llvm"
STAGE2="$BUILD_DIR/stage2-native-llvm"
STAGE3="$BUILD_DIR/stage3-native-llvm"
STAGE1_LL="$BUILD_DIR/stage1-native-selfhost.ll"
STAGE2_LL="$BUILD_DIR/stage2-native-selfhost.ll"
STAGE3_LL="$BUILD_DIR/stage3-native-selfhost.ll"
STAGE1_OBJ="$BUILD_DIR/stage1-native-selfhost.o"
STAGE2_OBJ="$BUILD_DIR/stage2-native-selfhost.o"
STAGE3_OBJ="$BUILD_DIR/stage3-native-selfhost.o"
INPUT="examples/selfhost_native_llvm_replacement.lan"
NULL_ENV_INPUT="examples/selfhost_native_llvm_getenv_null.lan"
NULL_ENV_NAME="LANNER_NATIVE_LLVM_TEST_MUST_BE_UNSET_7F9C2A"

printf '[1/8] stage 0 -> native stage 1 compiler\n'
"$BUILD_DIR/lanner" "$ROOT_DIR/selfhost/compiler.lan" --backend=llvm -o "$STAGE1"

printf '[2/8] native stage 1 -> stage 2 LLVM\n'
(
    cd "$ROOT_DIR"
    LANNER_SELFHOST_INPUT=selfhost/compiler.lan "$STAGE1" > "$STAGE2_LL"
)
"$CLANG" -c "$STAGE2_LL" -o "$STAGE2_OBJ"
"$CLANG" "$STAGE2_OBJ" -o "$STAGE2" >/dev/null 2>&1

printf '[3/8] native stage 2 -> stage 3 LLVM\n'
(
    cd "$ROOT_DIR"
    LANNER_SELFHOST_INPUT=selfhost/compiler.lan "$STAGE2" > "$STAGE3_LL"
)
"$CLANG" -c "$STAGE3_LL" -o "$STAGE3_OBJ"
"$CLANG" "$STAGE3_OBJ" -o "$STAGE3" >/dev/null 2>&1

printf '[4/8] native fixed point\n'
if ! cmp -s "$STAGE2_LL" "$STAGE3_LL"; then
    echo 'native bootstrap mismatch: stage-2 and stage-3 LLVM differ' >&2
    sha256sum "$STAGE2_LL" "$STAGE3_LL" >&2
    exit 1
fi

printf '[5/8] native stage-2/stage-3 behavior\n'
(
    cd "$ROOT_DIR"
    LANNER_SELFHOST_INPUT="$INPUT" "$STAGE2" > "$BUILD_DIR/native-stage2-probe.ll"
    LANNER_SELFHOST_INPUT="$INPUT" "$STAGE3" > "$BUILD_DIR/native-stage3-probe.ll"
)
if ! cmp -s "$BUILD_DIR/native-stage2-probe.ll" "$BUILD_DIR/native-stage3-probe.ll"; then
    echo 'native probe mismatch between stage 2 and stage 3' >&2
    exit 1
fi
"$CLANG" -c "$BUILD_DIR/native-stage2-probe.ll" -o "$BUILD_DIR/native-stage2-probe.o"
"$CLANG" "$BUILD_DIR/native-stage2-probe.o" -o "$BUILD_DIR/native-stage2-probe" >/dev/null 2>&1
set +e
"$BUILD_DIR/native-stage2-probe" >/dev/null
PROBE_RC=$?
set -e
if [[ "$PROBE_RC" -ne 0 ]]; then
    echo "native replacement probe failed with exit status $PROBE_RC" >&2
    exit 1
fi

printf '[6/8] native stage-1 behavior\n'
(
    cd "$ROOT_DIR"
    LANNER_SELFHOST_INPUT="$INPUT" "$STAGE1" > "$BUILD_DIR/native-stage1-probe.ll"
)
"$CLANG" "$BUILD_DIR/native-stage1-probe.ll" -o "$BUILD_DIR/native-stage1-probe" >/dev/null 2>&1
set +e
"$BUILD_DIR/native-stage1-probe" >/dev/null
STAGE1_RC=$?
set -e
if [[ "$STAGE1_RC" -ne 0 ]]; then
    echo "native stage-1 probe failed with exit status $STAGE1_RC" >&2
    exit 1
fi

printf '[7/8] native getenv null-safety\n'
NULL_ENV_LLVM="$BUILD_DIR/selfhost-native-llvm-getenv-null.ll"
NULL_ENV_OBJ="$BUILD_DIR/selfhost-native-llvm-getenv-null.o"
NULL_ENV_BIN="$BUILD_DIR/selfhost-native-llvm-getenv-null"
(
    cd "$ROOT_DIR"
    LANNER_SELFHOST_INPUT="$NULL_ENV_INPUT" "$STAGE1" > "$NULL_ENV_LLVM"
)
"$CLANG" -c "$NULL_ENV_LLVM" -o "$NULL_ENV_OBJ"
"$CLANG" "$NULL_ENV_OBJ" -o "$NULL_ENV_BIN" >/dev/null 2>&1
set +e
env -u "$NULL_ENV_NAME" "$NULL_ENV_BIN" >/dev/null
NULL_ENV_RC=$?
set -e
if [[ "$NULL_ENV_RC" -ne 0 ]]; then
    echo "native getenv null-safety probe failed with exit status $NULL_ENV_RC" >&2
    exit 1
fi

printf '[8/8] native bootstrap hashes\n'
sha256sum "$STAGE2_LL" "$STAGE3_LL" "$BUILD_DIR/native-stage2-probe.ll" "$BUILD_DIR/native-stage3-probe.ll"
printf 'self-host native LLVM stage-1 -> stage-2 -> stage-3 verification: PASS\n'
