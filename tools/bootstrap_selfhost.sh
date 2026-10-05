#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
if [[ "${STABLE_BOOTSTRAP_FRONTEND_ONLY:-0}" == "1" ]]; then
    BUILD_DIR="${STABLE_BUILD_DIR:-$ROOT_DIR/build}"
    INPUT="${1:-selfhost/compiler.st}"
    SELFHOST_BIN="${STABLE_SELFHOST_BIN:-$BUILD_DIR/stable-selfhost-front}"
    if [[ ! -x "$BUILD_DIR/stablec" ]]; then
        cmake -S "$ROOT_DIR" -B "$BUILD_DIR" -DSTABLE_BUILD_TESTS=ON -DSTABLE_ENABLE_LEGACY_HIR=OFF
        cmake --build "$BUILD_DIR" -j"${CMAKE_BUILD_PARALLEL_LEVEL:-2}"
    fi
    printf '[stage0] building Stable-written frontend -> %s\n' "$SELFHOST_BIN"
    "$BUILD_DIR/stablec" "$ROOT_DIR/selfhost/compiler.st" --backend=llvm -o "$SELFHOST_BIN"
    printf '[stage1] running Stable-written native frontend on %s\n' "$INPUT"
    ( cd "$ROOT_DIR"; STABLE_SELFHOST_INPUT="$INPUT" "$SELFHOST_BIN" )
    exit 0
fi

# Native LLVM is the unconditional production self-hosting bootstrap. The older
# C++/HIR bridge remains available only as explicit reference infrastructure.
exec "$ROOT_DIR/tools/verify_selfhost_native_llvm.sh"
