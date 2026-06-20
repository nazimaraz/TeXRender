#!/usr/bin/env python3
"""Check third-party notice files and release-license documentation for drift."""

from __future__ import annotations

import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def require(condition: bool, message: str, failures: list[str]) -> None:
    if not condition:
        failures.append(message)


def require_contains(path: str, tokens: tuple[str, ...], failures: list[str]) -> None:
    text = read(path)
    for token in tokens:
        require(token in text, f"{path} is missing {token!r}", failures)


def main() -> int:
    failures: list[str] = []

    for path in (
        "LICENSE",
        "THIRD_PARTY.md",
        "docs/dependency-license-audit.md",
        "docs/upstream-microtex-patch.md",
    ):
        require((ROOT / path).exists(), f"missing required notice/audit file: {path}", failures)

    if failures:
        for failure in failures:
            print(failure, file=sys.stderr)
        return 1

    require_contains(
        "THIRD_PARTY.md",
        (
            "MicroTeX",
            "MicroTeX `res/`",
            "FreeType",
            "tinyxml2",
            "raylib",
            "SDL2",
            "Knuth License",
            "SIL Open Font License",
            "dsrom",
        ),
        failures,
    )
    require_contains(
        "docs/dependency-license-audit.md",
        (
            "THIRD_PARTY.md",
            "MicroTeX core",
            "MicroTeX `res/` tree",
            "FreeType",
            "tinyxml2",
            "raylib",
            "SDL2",
            "check-third-party-notices.py",
            "upstream-microtex-patch.md",
        ),
        failures,
    )
    require_contains(
        "docs/upstream-microtex-patch.md",
        (
            "IndexedArray",
            "ASan",
            "static_cast<int>(_rows) - 1",
            "cmake/Dependencies.cmake",
        ),
        failures,
    )
    require_contains(
        "CMakeLists.txt",
        (
            'install(DIRECTORY "${MICROTEX_RES_DIR}/" DESTINATION "${TEXRENDER_INSTALL_RESDIR}")',
            'DESTINATION "${CMAKE_INSTALL_DOCDIR}"',
            "CPACK_RESOURCE_FILE_LICENSE",
        ),
        failures,
    )
    require_contains(
        "scripts/verify-installed-headers.sh",
        (
            "share/doc/texrender",
            "THIRD_PARTY.md",
            "dependency-license-audit.md",
            "upstream-microtex-patch.md",
        ),
        failures,
    )
    require_contains(
        "conanfile.py",
        (
            '"THIRD_PARTY.md"',
            'copy(self, "THIRD_PARTY.md"',
        ),
        failures,
    )
    require_contains(
        "packaging/vcpkg-overlay/ports/texrender/portfile.cmake",
        (
            "vcpkg_install_copyright",
            "THIRD_PARTY.md",
        ),
        failures,
    )
    require_contains(
        "Doxyfile",
        (
            "THIRD_PARTY.md",
            "docs/dependency-license-audit.md",
            "docs/upstream-microtex-patch.md",
        ),
        failures,
    )
    require_contains(
        "README.md",
        (
            "THIRD_PARTY.md",
            "dependency-license-audit.md",
            "upstream-microtex-patch.md",
        ),
        failures,
    )

    if failures:
        for failure in failures:
            print(failure, file=sys.stderr)
        return 1

    print("third-party notice checks passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
