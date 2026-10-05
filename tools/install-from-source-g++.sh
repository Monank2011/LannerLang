#!/usr/bin/env bash
set -euo pipefail

REPO_URL="${STABLE_REPO_URL:-https://github.com/Monank2011/STABLE.git}"
PREFIX="${STABLE_PREFIX:-${HOME}/.local/stable/source}"
WORK_DIR="$(mktemp -d)"
cleanup() { rm -rf "$WORK_DIR"; }
trap cleanup EXIT

command -v git >/dev/null || { echo 'error: git is required' >&2; exit 1; }
command -v g++ >/dev/null || { echo 'error: g++ is required' >&2; exit 1; }
command -v cmake >/dev/null || { echo 'error: cmake is required' >&2; exit 1; }

git clone --depth 1 "$REPO_URL" "$WORK_DIR/STABLE"
cmake -S "$WORK_DIR/STABLE" -B "$WORK_DIR/build" -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=g++ -DSTABLE_BUILD_TESTS=OFF
cmake --build "$WORK_DIR/build" --parallel
mkdir -p "$PREFIX/bin"
cp "$WORK_DIR/build/stablec" "$PREFIX/bin/stablec"
chmod +x "$PREFIX/bin/stablec"
printf 'Stable built from the latest source by Monank Gohil.\nCompiler: %s\nAdd %s to PATH.\n' "$PREFIX/bin/stablec" "$PREFIX/bin"
