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

git -C "$ROOT_DIR" archive --format=tar --prefix="$PREFIX" "$SOURCE_REF" -o "$SUPER_TAR"

append_submodule_archives() {
    local repo_dir="$1"
    local tree_ref="$2"
    local relative_prefix="$3"
    local mode _type sha path submodule_path submodule_dir submodule_tar

    while read -r mode _type sha path; do
        if [[ "$mode" != "160000" ]]; then
            continue
        fi

        submodule_path="${relative_prefix}${path}"
        submodule_dir="${repo_dir}/${path}"

        if [[ ! -e "$submodule_dir/.git" ]]; then
            git -C "$repo_dir" submodule update --init --recursive "$path"
        fi
        if ! git -C "$submodule_dir" cat-file -e "${sha}^{commit}" 2>/dev/null; then
            git -C "$submodule_dir" fetch origin "$sha"
        fi

        submodule_tar="${TMP_DIR}/submodule-${submodule_path//\//_}.tar"
        git -C "$submodule_dir" archive \
            --format=tar \
            --prefix="${PREFIX}${submodule_path}/" \
            "$sha" \
            -o "$submodule_tar"
        tar -Af "$SUPER_TAR" "$submodule_tar"

        append_submodule_archives "$submodule_dir" "$sha" "${submodule_path}/"
    done < <(git -C "$repo_dir" ls-tree -r "$tree_ref")
}

append_submodule_archives "$ROOT_DIR" "$SOURCE_REF" ""

if [[ "$FORMAT" == "tar.gz" ]]; then
    gzip -n -c "$SUPER_TAR" > "$OUTPUT"
else
    mv "$SUPER_TAR" "$OUTPUT"
fi
