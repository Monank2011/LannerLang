#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
if [[ "${LANNER_BOOTSTRAP_FRONTEND_ONLY:-0}" == "1" ]]; then
    BUILD_DIR="${LANNER_BUILD_DIR:-$ROOT_DIR/build}"
    INPUT="${1:-selfhost/compiler.lan}"
    SELFHOST_BIN="${LANNER_SELFHOST_BIN:-$BUILD_DIR/lanner-selfhost-front}"
    if [[ ! -x "$BUILD_DIR/lanner" ]]; then
        cmake -S "$ROOT_DIR" -B "$BUILD_DIR" -DLANNER_BUILD_TESTS=ON -DLANNER_ENABLE_LEGACY_HIR=OFF
        cmake --build "$BUILD_DIR" -j"${CMAKE_BUILD_PARALLEL_LEVEL:-2}"
    fi
    printf '[stage0] building Lanner-written frontend -> %s\n' "$SELFHOST_BIN"
    "$BUILD_DIR/lanner" "$ROOT_DIR/selfhost/compiler.lan" --backend=llvm -o "$SELFHOST_BIN"
    printf '[stage1] running Lanner-written native frontend on %s\n' "$INPUT"
    ( cd "$ROOT_DIR"; LANNER_SELFHOST_INPUT="$INPUT" "$SELFHOST_BIN" )
    exit 0
fi

# Native LLVM is the unconditional production self-hosting bootstrap. The older
# C++/HIR bridge remains available only as explicit reference infrastructure.
exec "$ROOT_DIR/tools/verify_selfhost_native_llvm.sh"
