#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
FORMAT="tar"
PREFIX=""
SOURCE_REF="HEAD"
OUTPUT=""

usage() {
    printf 'usage: %s --output <path> [--format tar|tar.gz] [--prefix <prefix/>] [--ref <git-ref>]\n' "$0" >&2
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --format)
            FORMAT="${2:-}"
            shift 2
            ;;
        --output)
            OUTPUT="${2:-}"
            shift 2
            ;;
        --prefix)
            PREFIX="${2:-}"
            shift 2
            ;;
        --ref)
            SOURCE_REF="${2:-}"
            shift 2
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            usage
            exit 1
            ;;
    esac
done

if [[ -z "$OUTPUT" ]]; then
    usage
    exit 1
fi

if [[ "$FORMAT" != "tar" && "$FORMAT" != "tar.gz" ]]; then
    printf 'unsupported archive format: %s\n' "$FORMAT" >&2
    exit 1
fi

TMP_DIR="$(mktemp -d "${TMPDIR:-/tmp}/texrender-source-archive.XXXXXX")"
trap 'rm -rf "$TMP_DIR"' EXIT

mkdir -p "$(dirname "$OUTPUT")"
SUPER_TAR="${TMP_DIR}/source.tar"
MICROTEX_TAR="${TMP_DIR}/microtex.tar"

git -C "$ROOT_DIR" archive --format=tar --prefix="$PREFIX" "$SOURCE_REF" -o "$SUPER_TAR"

microtex_tree_entry="$(git -C "$ROOT_DIR" ls-tree "$SOURCE_REF" third_party/microtex || true)"
if [[ "$microtex_tree_entry" == 160000* ]]; then
    microtex_sha="$(printf '%s\n' "$microtex_tree_entry" | awk '{print $3}')"
    if [[ ! -e "$ROOT_DIR/third_party/microtex/.git" ]]; then
        git -C "$ROOT_DIR" submodule update --init --recursive third_party/microtex
    fi
    if ! git -C "$ROOT_DIR/third_party/microtex" cat-file -e "${microtex_sha}^{commit}" 2>/dev/null; then
        git -C "$ROOT_DIR/third_party/microtex" fetch origin "$microtex_sha"
    fi
    git -C "$ROOT_DIR/third_party/microtex" archive \
        --format=tar \
        --prefix="${PREFIX}third_party/microtex/" \
        "$microtex_sha" \
        -o "$MICROTEX_TAR"
    tar -Af "$SUPER_TAR" "$MICROTEX_TAR"
fi

if [[ "$FORMAT" == "tar.gz" ]]; then
    gzip -n -c "$SUPER_TAR" > "$OUTPUT"
else
    mv "$SUPER_TAR" "$OUTPUT"
fi
