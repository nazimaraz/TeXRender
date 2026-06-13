#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
VCPKG_ROOT="${VCPKG_ROOT:-}"
TRIPLET="${VCPKG_DEFAULT_TRIPLET:-}"

if [[ -z "$VCPKG_ROOT" || ! -x "$VCPKG_ROOT/vcpkg" ]]; then
    printf 'VCPKG_ROOT must point at a bootstrapped vcpkg checkout\n' >&2
    exit 1
fi

if [[ -z "$TRIPLET" ]]; then
    case "$(uname -s)" in
        Darwin)
            case "$(uname -m)" in
                arm64|aarch64) TRIPLET="arm64-osx" ;;
                *) TRIPLET="x64-osx" ;;
            esac
            ;;
        Linux) TRIPLET="x64-linux" ;;
        MINGW*|MSYS*|CYGWIN*) TRIPLET="x64-windows" ;;
        *) TRIPLET="x64-linux" ;;
    esac
fi

export TEXGUI_VCPKG_SOURCE_DIR="$ROOT_DIR"

"$VCPKG_ROOT/vcpkg" install latex-renderer \
    --overlay-ports="$ROOT_DIR/packaging/vcpkg-overlay/ports" \
    --triplet="$TRIPLET"
