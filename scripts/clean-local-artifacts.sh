#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
MODE="${1:---dry-run}"

usage() {
    cat <<'EOF'
Usage:
  scripts/clean-local-artifacts.sh [--dry-run]
  scripts/clean-local-artifacts.sh --force

Removes generated build/test artifacts that are safe to recreate. The default is
a dry run. IDE settings and other ignored personal files are intentionally left
alone.
EOF
}

case "$MODE" in
    --dry-run) FORCE=0 ;;
    --force) FORCE=1 ;;
    -h|--help)
        usage
        exit 0
        ;;
    *)
        usage >&2
        exit 2
        ;;
esac

cd "$ROOT_DIR"

shopt -s nullglob
candidates=(
    build
    build-*
    cmake-build-*
    out
    build-api-docs
    CMakeFiles
    CMakeCache.txt
    __pycache__
    scripts/__pycache__
    test_package/CMakeUserPresets.json
    formula.ppm
    texgui_shot.png
    shot_*.png
)
shopt -u nullglob

paths=()
add_path() {
    local candidate="$1"
    local existing
    [[ -e "$candidate" || -L "$candidate" ]] || return 0
    for existing in "${paths[@]}"; do
        [[ "$existing" == "$candidate" ]] && return 0
    done
    paths+=("$candidate")
}

for candidate in "${candidates[@]}"; do
    add_path "$candidate"
done

if (( ${#paths[@]} == 0 )); then
    printf 'no generated local artifacts found\n'
    exit 0
fi

printf 'generated local artifacts:\n'
du -sh "${paths[@]}" 2>/dev/null || true

if (( FORCE == 0 )); then
    printf '\ndry run only. Re-run with --force to remove these artifacts.\n'
    exit 0
fi

rm -rf -- "${paths[@]}"
printf '\nremoved generated local artifacts\n'
