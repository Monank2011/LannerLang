#!/usr/bin/env bash
set -euo pipefail

LANNER_VERSION="3.0.0"
LLVM_VERSION="23.1.2"
LLVM_URL="https://github.com/llvm/llvm-project/releases/download/llvmorg-${LLVM_VERSION}/LLVM-${LLVM_VERSION}-Linux-X64.tar.xz"
LLVM_SHA256="b5ed9675149cc837c282e9b6962c276c9fa62863d5b2f91537b60848552995b7"

PREFIX="${LANNER_PREFIX:-${HOME}/.local/lanner/${LANNER_VERSION}}"
if [[ "${1:-}" == "--system" ]]; then
  PREFIX="/usr/local/lib/lanner/${LANNER_VERSION}"
fi

if [[ ! -w "$(dirname "$PREFIX")" && ! -w "$PREFIX" ]]; then
  if [[ "$PREFIX" == /usr/local/* ]]; then
    echo "error: $PREFIX is not writable; rerun with sudo or omit --system" >&2
    exit 1
  fi
  mkdir -p "$PREFIX"
fi

TMP_DIR="$(mktemp -d)"
cleanup() { rm -rf "$TMP_DIR"; }
trap cleanup EXIT
ARCHIVE="$TMP_DIR/llvm.tar.xz"

if command -v curl >/dev/null 2>&1; then
  curl --fail --location --retry 3 --output "$ARCHIVE" "$LLVM_URL"
elif command -v wget >/dev/null 2>&1; then
  wget -O "$ARCHIVE" "$LLVM_URL"
else
  echo "error: curl or wget is required" >&2
  exit 1
fi

echo "${LLVM_SHA256}  ${ARCHIVE}" | sha256sum --check --status

mkdir -p "$PREFIX/llvm/$LLVM_VERSION" "$PREFIX/bin"
rm -rf "$TMP_DIR/llvm-extracted"
mkdir "$TMP_DIR/llvm-extracted"
tar -xJf "$ARCHIVE" -C "$TMP_DIR/llvm-extracted"
LLVM_ROOT="$(find "$TMP_DIR/llvm-extracted" -mindepth 1 -maxdepth 1 -type d -print -quit)"
if [[ -z "$LLVM_ROOT" || ! -x "$LLVM_ROOT/bin/clang" ]]; then
  echo "error: downloaded LLVM archive has an unexpected layout" >&2
  exit 1
fi
rm -rf "$PREFIX/llvm/$LLVM_VERSION"
mv "$LLVM_ROOT" "$PREFIX/llvm/$LLVM_VERSION"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
COMPILER="${SCRIPT_DIR}/../bin/lanner"
if [[ ! -x "$COMPILER" ]]; then
  echo "error: lanner binary not found next to this installer" >&2
  exit 1
fi
cp "$COMPILER" "$PREFIX/bin/lanner-bin"
cat > "$PREFIX/bin/lanner" <<EOF
#!/usr/bin/env bash
set -e
export LANNER_CLANG="${PREFIX}/llvm/${LLVM_VERSION}/bin/clang"
exec "${PREFIX}/bin/lanner-bin" "\$@"
EOF
chmod +x "$PREFIX/bin/lanner"

cat <<EOF
Lanner ${LANNER_VERSION} installed.
LLVM/Clang ${LLVM_VERSION} installed side by side at:
  ${PREFIX}/llvm/${LLVM_VERSION}
Compiler wrapper:
  ${PREFIX}/bin/lanner
Add ${PREFIX}/bin to PATH to use lanner from new shells.
EOF
