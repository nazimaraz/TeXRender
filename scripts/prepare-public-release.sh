#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TAG="${1:-}"
REMOTE="${REMOTE:-origin}"
RUN_ARCHIVE_SMOKE="${RUN_ARCHIVE_SMOKE:-1}"
RUN_FRESH_CLONE_SMOKE="${RUN_FRESH_CLONE_SMOKE:-1}"

run() {
    printf '\n==> %s\n' "$*"
    "$@"
}

cd "$ROOT_DIR"

if [[ -n "$(git status --porcelain)" ]]; then
    printf 'working tree must be clean before public release preparation\n' >&2
    git status --short
    exit 1
fi

if ! git remote get-url "$REMOTE" >/dev/null 2>&1; then
    printf "remote '%s' is not configured. Add it first, for example:\n" "$REMOTE" >&2
    printf '  git remote add origin git@github.com:nazimaraz/latex-renderer.git\n' >&2
    exit 1
fi

run git remote -v
run cmake --list-presets
run git diff --check

if command -v clang-format >/dev/null 2>&1; then
    printf '\n==> clang-format --dry-run\n'
    git ls-files '*.cpp' '*.hpp' | xargs clang-format --dry-run -Werror
else
    printf '\n==> skipping clang-format: not installed\n'
fi

if [[ "$RUN_FRESH_CLONE_SMOKE" != "0" ]]; then
    run "$ROOT_DIR/scripts/verify-fresh-clone.sh"
fi

if [[ "$RUN_ARCHIVE_SMOKE" != "0" ]]; then
    run "$ROOT_DIR/scripts/verify-release-archive.sh"
fi

printf '\npublic release preparation checks passed\n'
printf 'current commit: %s\n' "$(git rev-parse --short HEAD)"

if [[ -n "$TAG" ]]; then
    if git rev-parse -q --verify "refs/tags/$TAG" >/dev/null; then
        printf "tag '%s' already exists\n" "$TAG" >&2
        exit 1
    fi

    run git tag -a "$TAG" -m "Release $TAG"
    printf "\ncreated local tag %s\n" "$TAG"
    printf "push when public CI is green: git push %s %s\n" "$REMOTE" "$TAG"
else
    printf "\nno tag created. Pass a tag name, for example: %s v0.1.0\n" "$0"
fi
