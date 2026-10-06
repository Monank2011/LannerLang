#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${LANNER_BUILD_DIR:-$ROOT_DIR/build}"

printf '[1/5] building stage-0 compiler\n'
cmake -S "$ROOT_DIR" -B "$BUILD_DIR" -DLANNER_BUILD_TESTS=ON
cmake --build "$BUILD_DIR" -j"${CMAKE_BUILD_PARALLEL_LEVEL:-2}"

printf '[2/5] running CTest\n'
ctest --test-dir "$BUILD_DIR" --output-on-failure

printf '[3/5] checking Lanner-written compiler frontend\n'
"$BUILD_DIR/lanner" "$ROOT_DIR/selfhost/compiler.st" --backend=hir --check

printf '[4/5] stage-0 -> stage-1 bootstrap and self-parse\n'
"$ROOT_DIR/tools/bootstrap_selfhost.sh" selfhost/compiler.st

printf 'self-hosting frontend verification: PASS\n'
printf '[5/5] full stage-1 -> stage-2 -> stage-3 bootstrap\n'
"$ROOT_DIR/tools/verify_selfhost_bootstrap.sh"
