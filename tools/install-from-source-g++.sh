#!/usr/bin/env bash
set -euo pipefail

REPO_URL="${LANNER_REPO_URL:-https://github.com/Monank2011/LannerLang.git}"
PREFIX="${LANNER_PREFIX:-${HOME}/.local/lanner/source}"
WORK_DIR="$(mktemp -d)"
cleanup() { rm -rf "$WORK_DIR"; }
trap cleanup EXIT

command -v git >/dev/null || { echo 'error: git is required' >&2; exit 1; }
command -v g++ >/dev/null || { echo 'error: g++ is required' >&2; exit 1; }
command -v cmake >/dev/null || { echo 'error: cmake is required' >&2; exit 1; }

git clone --depth 1 "$REPO_URL" "$WORK_DIR/LANNER"
cmake -S "$WORK_DIR/LANNER" -B "$WORK_DIR/build" -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=g++ -DLANNER_BUILD_TESTS=OFF
cmake --build "$WORK_DIR/build" --parallel
mkdir -p "$PREFIX/bin"
cp "$WORK_DIR/build/lanner" "$PREFIX/bin/lanner"
chmod +x "$PREFIX/bin/lanner"
printf 'Lanner built from the latest source by Monank Gohil.\nCompiler: %s\nAdd %s to PATH.\n' "$PREFIX/bin/lanner" "$PREFIX/bin"
