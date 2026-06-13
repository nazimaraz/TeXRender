#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_ROOT="${BUILD_ROOT:-/tmp/texgui-fresh-clone}"
GENERATOR="${GENERATOR:-Ninja}"
SOURCE_REPO="${TEXGUI_FRESH_CLONE_SOURCE:-$ROOT_DIR}"
SOURCE_REF="${TEXGUI_FRESH_CLONE_REF:-$(git -C "$ROOT_DIR" rev-parse HEAD)}"
ALLOW_DIRTY="${TEXGUI_FRESH_CLONE_ALLOW_DIRTY:-0}"
FULL_SMOKE="${TEXGUI_FRESH_CLONE_FULL:-0}"
CLONE_DIR="${BUILD_ROOT}/clone"
BUILD_DIR="${BUILD_ROOT}/build"
INSTALL_PREFIX="${BUILD_ROOT}/prefix"
PACKAGE_CONSUMER_BUILD="${BUILD_ROOT}/package-consumer"
FETCH_CONSUMER_BUILD="${BUILD_ROOT}/fetch-consumer"

run() {
    printf '\n==> %s\n' "$*"
    "$@"
}

has() {
    command -v "$1" >/dev/null 2>&1
}

if ! has git; then
    printf 'git is required\n' >&2
    exit 1
fi

if ! has cmake; then
    printf 'cmake is required\n' >&2
    exit 1
fi

if ! has python3; then
    printf 'python3 is required\n' >&2
    exit 1
fi

if [[ "$GENERATOR" == "Ninja" ]] && ! has ninja; then
    printf 'ninja is required for GENERATOR=Ninja\n' >&2
    exit 1
fi

if [[ "$ALLOW_DIRTY" != "1" && -n "$(git -C "$ROOT_DIR" status --porcelain)" ]]; then
    printf 'fresh-clone verification requires a clean worktree by default\n' >&2
    printf 'commit/stash changes, or set TEXGUI_FRESH_CLONE_ALLOW_DIRTY=1 to test committed HEAD anyway\n' >&2
    git -C "$ROOT_DIR" status --short >&2
    exit 1
fi

rm -rf "$BUILD_ROOT"
mkdir -p "$BUILD_ROOT"

run git clone --no-local "$SOURCE_REPO" "$CLONE_DIR"
run git -C "$CLONE_DIR" checkout --detach "$SOURCE_REF"
run git -C "$CLONE_DIR" status --short
run python3 "$CLONE_DIR/scripts/check-doc-links.py"
run python3 "$CLONE_DIR/scripts/check-registry-readiness.py"
run python3 "$CLONE_DIR/scripts/check-public-api.py"

run cmake -S "$CLONE_DIR" -B "$BUILD_DIR" -G "$GENERATOR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DTEXGUI_BACKEND=headless \
    -DTEXGUI_WARNINGS_AS_ERRORS=ON
run cmake --build "$BUILD_DIR" -j
run ctest --test-dir "$BUILD_DIR" --output-on-failure
run "$BUILD_DIR/texgui_offscreen" 'e^{i\pi} + 1 = 0' "${BUILD_ROOT}/formula.png"

run cmake --install "$BUILD_DIR" --prefix "$INSTALL_PREFIX"
run cmake -S "$CLONE_DIR/tests/package_consumer" -B "$PACKAGE_CONSUMER_BUILD" -G "$GENERATOR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_PREFIX_PATH="$INSTALL_PREFIX"
run cmake --build "$PACKAGE_CONSUMER_BUILD" -j
run ctest --test-dir "$PACKAGE_CONSUMER_BUILD" --output-on-failure

run cmake -S "$CLONE_DIR/examples/consumer-fetchcontent" -B "$FETCH_CONSUMER_BUILD" -G "$GENERATOR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DTEXGUI_FETCHCONTENT_SOURCE_DIR="$CLONE_DIR"
run cmake --build "$FETCH_CONSUMER_BUILD" -j
run cmake -E chdir "$FETCH_CONSUMER_BUILD" "$FETCH_CONSUMER_BUILD/texgui_fetchcontent_example"

if [[ "$FULL_SMOKE" == "1" ]]; then
    run env BUILD_ROOT="${BUILD_ROOT}/cpack" "$CLONE_DIR/scripts/verify-cpack.sh"
    run env BUILD_ROOT="${BUILD_ROOT}/release-archive" "$CLONE_DIR/scripts/verify-release-archive.sh"
fi

printf '\nfresh-clone verification completed successfully\n'
printf 'source repo: %s\n' "$SOURCE_REPO"
printf 'source ref: %s\n' "$SOURCE_REF"
printf 'clone: %s\n' "$CLONE_DIR"
