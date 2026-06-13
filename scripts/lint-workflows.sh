#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

if ! command -v actionlint >/dev/null 2>&1; then
    cat >&2 <<'EOF'
actionlint is not installed.

Install it with one of:
  brew install actionlint
  go install github.com/rhysd/actionlint/cmd/actionlint@latest

Then rerun:
  scripts/lint-workflows.sh
EOF
    exit 127
fi

actionlint "$@"
