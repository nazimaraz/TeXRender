#!/usr/bin/env python3
"""Check local Markdown links and images.

External URLs are intentionally skipped; this catches repo-local path drift
before docs are published.
"""

from __future__ import annotations

import re
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
LINK_RE = re.compile(r"!?\[[^\]]*\]\(([^)]+)\)")
SKIP_SCHEMES = ("http://", "https://", "mailto:", "tel:")


def markdown_files() -> list[Path]:
    paths: list[Path] = []
    for path in ROOT.rglob("*.md"):
        rel = path.relative_to(ROOT).as_posix()
        dirs = Path(rel).parts[:-1]
        if any(
            part in {".cache", ".git", "_deps", "build", "build-api-docs", "out"}
            or part.startswith("build-")
            or part.startswith("cmake-build")
            for part in dirs
        ):
            continue
        paths.append(path)
    return sorted(paths)


def local_target(raw: str) -> str | None:
    target = raw.strip()
    if not target or target.startswith("#") or target.startswith(SKIP_SCHEMES):
        return None
    if " " in target and not target.startswith("<"):
        target = target.split()[0]
    if target.startswith("<") and target.endswith(">"):
        target = target[1:-1]
    if target.startswith(SKIP_SCHEMES) or target.startswith("#"):
        return None
    return target


def main() -> int:
    failures: list[str] = []
    for path in markdown_files():
        text = path.read_text(encoding="utf-8")
        for match in LINK_RE.finditer(text):
            target = local_target(match.group(1))
            if target is None:
                continue

            clean_target = target.split("#", 1)[0]
            if not clean_target:
                continue

            candidate = (path.parent / clean_target).resolve()
            try:
                candidate.relative_to(ROOT)
            except ValueError:
                failures.append(f"{path.relative_to(ROOT)}: link escapes repo: {target}")
                continue

            if not candidate.exists():
                failures.append(f"{path.relative_to(ROOT)}: missing link target: {target}")

    if failures:
        for failure in failures:
            print(failure, file=sys.stderr)
        return 1

    print(f"checked {len(markdown_files())} Markdown files")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
