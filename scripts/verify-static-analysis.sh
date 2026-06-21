#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-/tmp/texrender-tidy}"
GENERATOR="${GENERATOR:-Ninja}"
CXX_COMPILER="${CXX:-}"
CLANG_TIDY_BIN="${CLANG_TIDY:-}"
TIDY_FILES="${BUILD_DIR}/tidy-files.txt"

run() {
    printf '\n==> %s\n' "$*"
    "$@"
}

has() {
    command -v "$1" >/dev/null 2>&1
}

if ! has cmake; then
    printf 'cmake is required\n' >&2
    exit 1
fi

if [[ "$GENERATOR" == "Ninja" ]] && ! has ninja; then
    printf 'ninja is required for GENERATOR=Ninja\n' >&2
    exit 1
fi

if [[ -z "$CLANG_TIDY_BIN" ]]; then
    for candidate in \
        clang-tidy-22 \
        clang-tidy \
        /opt/homebrew/opt/llvm/bin/clang-tidy \
        /usr/local/opt/llvm/bin/clang-tidy; do
        if has "$candidate"; then
            CLANG_TIDY_BIN="$(command -v "$candidate")"
            break
        fi
    done
fi

if [[ -z "$CLANG_TIDY_BIN" ]]; then
    printf 'clang-tidy is required\n' >&2
    printf 'Set CLANG_TIDY to a clang-tidy executable, for example CLANG_TIDY=clang-tidy-22.\n' >&2
    exit 1
fi

if [[ -z "$CXX_COMPILER" ]]; then
    tidy_dir="$(cd "$(dirname "$CLANG_TIDY_BIN")" && pwd)"
    if [[ -x "${tidy_dir}/clang++" ]]; then
        CXX_COMPILER="${tidy_dir}/clang++"
    else
        CXX_COMPILER="c++"
    fi
fi

if ! has "$CXX_COMPILER"; then
    printf "C++ compiler '%s' is not available\n" "$CXX_COMPILER" >&2
    printf 'Set CXX to the compiler that should generate compile_commands.json.\n' >&2
    exit 1
fi

cd "$ROOT_DIR"

run cmake -S "$ROOT_DIR" -B "$BUILD_DIR" -G "$GENERATOR" \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_CXX_COMPILER="$CXX_COMPILER" \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    -DTEXRENDER_BACKEND=headless \
    -DTEXRENDER_BUILD_EXAMPLES=ON \
    -DTEXRENDER_BUILD_TESTS=ON \
    -DTEXRENDER_BUILD_BENCHMARKS=ON

run python3 - "$ROOT_DIR" "$BUILD_DIR/compile_commands.json" "$TIDY_FILES" <<'PY'
import json
import sys
from pathlib import Path

root = Path(sys.argv[1]).resolve()
compile_commands = Path(sys.argv[2])
output = Path(sys.argv[3])
prefixes = ("source/", "tests/", "benchmarks/", "examples/")
files = set()

for entry in json.loads(compile_commands.read_text()):
    path = Path(entry["file"]).resolve()
    try:
        rel = path.relative_to(root).as_posix()
    except ValueError:
        continue
    if rel.startswith(prefixes):
        files.add(str(path))

if not files:
    raise SystemExit("no first-party translation units found")

output.write_text("".join(f"{path}\n" for path in sorted(files)))
PY

run xargs "$CLANG_TIDY_BIN" -p "$BUILD_DIR" --warnings-as-errors='*' < "$TIDY_FILES"

printf '\nstatic analysis completed successfully\n'
