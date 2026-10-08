#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${LANNER_LEGACY_BUILD_DIR:-${LANNER_BUILD_DIR:-$ROOT_DIR/build-legacy}}"
CXX="${LANNER_BOOTSTRAP_CXX:-$(command -v clang++ || true)}"
if [[ -z "$CXX" ]]; then
    echo "error: clang++ is required for the stage-2 bootstrap check" >&2
    exit 1
fi

STAGE1="$BUILD_DIR/stage1-selfhost"
STAGE2_CPP="$BUILD_DIR/stage2-selfhost.cpp"
STAGE2="$BUILD_DIR/stage2-selfhost"
STAGE3_CPP="$BUILD_DIR/stage3-selfhost.cpp"
STAGE3="$BUILD_DIR/stage3-selfhost"
STAGE2_ASAN="$BUILD_DIR/stage2-selfhost-asan"
PROBE2="$BUILD_DIR/stage2-probe.cpp"
PROBE3="$BUILD_DIR/stage3-probe.cpp"
STRESS2="$BUILD_DIR/stage2-stress.cpp"
STRESS3="$BUILD_DIR/stage3-stress.cpp"
STRESS2_BIN="$BUILD_DIR/stage2-stress"
STRESS3_BIN="$BUILD_DIR/stage3-stress"
STAGE0_PROBE="$BUILD_DIR/stage0-probe"
STAGE0_STRESS="$BUILD_DIR/stage0-stress"
INVALID="examples/selfhost_semantic_invalid.lan"

if [[ ! -x "$BUILD_DIR/lanner" ]]; then
    cmake -S "$ROOT_DIR" -B "$BUILD_DIR" -DLANNER_BUILD_TESTS=ON -DLANNER_ENABLE_LEGACY_HIR=ON
    cmake --build "$BUILD_DIR" -j"${CMAKE_BUILD_PARALLEL_LEVEL:-2}"
fi

printf '[1/10] stage 0 -> stage 1 compiler\n'
"$BUILD_DIR/lanner" "$ROOT_DIR/selfhost/compiler.lan" --backend=hir -o "$STAGE1"

printf '[2/10] stage 1 -> stage 2 compiler source\n'
(
    cd "$ROOT_DIR"
    LANNER_SELFHOST_REFERENCE=1 LANNER_SELFHOST_INPUT=selfhost/compiler.lan "$STAGE1" > "$STAGE2_CPP"
)
"$CXX" -std=c++17 -O2 -Wall -Wextra -Wpedantic -Werror "$STAGE2_CPP" -o "$STAGE2"

printf '[3/10] stage 2 -> stage 3 compiler source\n'
(
    cd "$ROOT_DIR"
    LANNER_SELFHOST_REFERENCE=1 LANNER_SELFHOST_INPUT=selfhost/compiler.lan "$STAGE2" > "$STAGE3_CPP"
)
"$CXX" -std=c++17 -O2 -Wall -Wextra -Wpedantic -Werror "$STAGE3_CPP" -o "$STAGE3"
cmp -s "$STAGE2_CPP" "$STAGE3_CPP"

printf '[4/10] sanitized stage-2 compiler execution\n'
"$CXX" -std=c++17 -O1 -fsanitize=address,undefined -fno-omit-frame-pointer "$STAGE2_CPP" -o "$STAGE2_ASAN"
(
    cd "$ROOT_DIR"
    LANNER_SELFHOST_INPUT=examples/selfhost_input.lan "$STAGE2_ASAN" > /dev/null
)

printf '[5/10] stage-0 reference behavior\n'
"$BUILD_DIR/lanner" "$ROOT_DIR/examples/selfhost_input.lan" --backend=hir -o "$STAGE0_PROBE"
"$BUILD_DIR/lanner" "$ROOT_DIR/examples/selfhost_bootstrap.lan" --backend=hir -o "$STAGE0_STRESS"
set +e
"$STAGE0_PROBE" >/dev/null
STAGE0_PROBE_RC=$?
"$STAGE0_STRESS" >/dev/null
STAGE0_STRESS_RC=$?
set -e
if [[ "$STAGE0_PROBE_RC" -ne 0 || "$STAGE0_STRESS_RC" -ne 2 ]]; then
    echo "unexpected stage-0 reference results: probe=$STAGE0_PROBE_RC stress=$STAGE0_STRESS_RC" >&2
    exit 1
fi

printf '[6/10] generated probe artifact comparison\n'
(
    cd "$ROOT_DIR"
    LANNER_SELFHOST_REFERENCE=1 LANNER_SELFHOST_INPUT=examples/selfhost_input.lan "$STAGE2" > "$PROBE2"
    LANNER_SELFHOST_REFERENCE=1 LANNER_SELFHOST_INPUT=examples/selfhost_input.lan "$STAGE3" > "$PROBE3"
)
cmp -s "$PROBE2" "$PROBE3"
"$CXX" -std=c++17 -O2 "$PROBE2" -o "$BUILD_DIR/stage2-probe"

printf '[7/10] nested ownership/Result bootstrap probe\n'
(
    cd "$ROOT_DIR"
    LANNER_SELFHOST_REFERENCE=1 LANNER_SELFHOST_INPUT=examples/selfhost_bootstrap.lan "$STAGE2" > "$STRESS2"
    LANNER_SELFHOST_REFERENCE=1 LANNER_SELFHOST_INPUT=examples/selfhost_bootstrap.lan "$STAGE3" > "$STRESS3"
)
cmp -s "$STRESS2" "$STRESS3"
"$CXX" -std=c++17 -O2 "$STRESS2" -o "$STRESS2_BIN"
"$CXX" -std=c++17 -O2 "$STRESS3" -o "$STRESS3_BIN"

printf '[8/10] stage-1/stage-2/stage-3 behavioral equivalence\n'
set +e
"$BUILD_DIR/stage2-probe" >/dev/null
RC_PROBE2=$?
"$STRESS2_BIN" >/dev/null
RC2=$?
"$STRESS3_BIN" >/dev/null
RC3=$?
set -e
if [[ "$RC_PROBE2" -ne "$STAGE0_PROBE_RC" || "$RC2" -ne "$STAGE0_STRESS_RC" || "$RC3" -ne "$STAGE0_STRESS_RC" ]]; then
    echo "behavior mismatch: stage0_probe=$STAGE0_PROBE_RC stage2_probe=$RC_PROBE2 stage0_stress=$STAGE0_STRESS_RC stage2_stress=$RC2 stage3_stress=$RC3" >&2
    exit 1
fi

printf '[9/10] semantic rejection equivalence\n'
set +e
"$BUILD_DIR/lanner" "$ROOT_DIR/$INVALID" --backend=hir --check >/dev/null 2>/dev/null
REF_INVALID_RC=$?
(
    cd "$ROOT_DIR"
    LANNER_SELFHOST_REFERENCE=1 LANNER_SELFHOST_INPUT="$INVALID" "$STAGE2" >/dev/null 2>/dev/null
)
STAGE2_INVALID_RC=$?
(
    cd "$ROOT_DIR"
    LANNER_SELFHOST_REFERENCE=1 LANNER_SELFHOST_INPUT="$INVALID" "$STAGE3" >/dev/null 2>/dev/null
)
STAGE3_INVALID_RC=$?
set -e
if [[ "$REF_INVALID_RC" -ne 1 || "$STAGE2_INVALID_RC" -ne 1 || "$STAGE3_INVALID_RC" -ne 1 ]]; then
    echo "semantic rejection mismatch: stage0=$REF_INVALID_RC stage2=$STAGE2_INVALID_RC stage3=$STAGE3_INVALID_RC" >&2
    exit 1
fi

printf '[10/10] bootstrap hashes\n'
sha256sum "$STAGE2_CPP" "$STAGE3_CPP" "$PROBE2" "$PROBE3" "$STRESS2" "$STRESS3"
printf 'self-hosting stage-1 -> stage-2 -> stage-3 verification: PASS\n'
