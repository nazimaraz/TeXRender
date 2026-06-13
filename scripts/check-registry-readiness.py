#!/usr/bin/env python3
"""Check package-registry metadata that should not drift before a release."""

from __future__ import annotations

import json
import re
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def load_json(path: str) -> dict:
    return json.loads(read(path))


def match(pattern: str, text: str, label: str) -> str:
    found = re.search(pattern, text, re.MULTILINE | re.DOTALL)
    if not found:
        raise ValueError(f"could not find {label}")
    return found.group(1)


def conan_string(field: str, text: str) -> str:
    return match(rf"^\s*{re.escape(field)}\s*=\s*[\"']([^\"']+)[\"']", text, f"conan field {field}")


def require(condition: bool, message: str, failures: list[str]) -> None:
    if not condition:
        failures.append(message)


def main() -> int:
    failures: list[str] = []

    cmake = read("CMakeLists.txt")
    conan = read("conanfile.py")
    portfile = read("packaging/vcpkg-overlay/ports/latex-renderer/portfile.cmake")
    manifest = load_json("vcpkg.json")
    overlay_manifest = load_json("packaging/vcpkg-overlay/ports/latex-renderer/vcpkg.json")

    try:
        cmake_version = match(r"project\s*\(\s*latex_renderer\s+VERSION\s+([0-9]+(?:\.[0-9]+){2})", cmake, "CMake project version")
    except ValueError as error:
        failures.append(str(error))
        cmake_version = ""

    conan_required_fields = {
        "name": "latex-renderer",
        "version": cmake_version,
        "license": "MIT",
        "url": "https://github.com/nazimaraz/latex-renderer",
        "package_type": "library",
    }
    for field, expected in conan_required_fields.items():
        try:
            actual = conan_string(field, conan)
        except ValueError as error:
            failures.append(str(error))
            continue
        require(actual == expected, f"conanfile.py {field!r} is {actual!r}, expected {expected!r}", failures)

    for field in ("description", "topics", "required_conan_version"):
        require(field in conan, f"conanfile.py is missing {field}", failures)

    for option in ('"shared"', '"fPIC"', '"backend"', '"with_examples"', '"with_tests"', '"with_benchmarks"'):
        require(option in conan, f"conanfile.py options/defaults are missing {option}", failures)

    for backend in ('"headless"', '"raylib"', '"svg"', '"sdl2"'):
        require(backend in conan, f"conanfile.py backend options are missing {backend}", failures)

    for exported in ('"CMakeLists.txt"', '"cmake/*"', '"include/*"', '"source/*"', '"tests/*"', '"docs/*"'):
        require(exported in conan, f"conanfile.py exports_sources is missing {exported}", failures)

    for path, data in (("vcpkg.json", manifest), ("packaging/vcpkg-overlay/ports/latex-renderer/vcpkg.json", overlay_manifest)):
        require(data.get("name") == "latex-renderer", f"{path} has wrong package name", failures)
        require(data.get("version-string") == cmake_version, f"{path} version-string does not match CMake project version", failures)
        require(data.get("license") == "MIT", f"{path} license must be MIT", failures)
        require(str(data.get("homepage", "")).startswith("https://github.com/"), f"{path} homepage should point at GitHub", failures)

        dependencies = data.get("dependencies", [])
        dependency_names = {dep if isinstance(dep, str) else dep.get("name") for dep in dependencies}
        require("freetype" in dependency_names, f"{path} is missing freetype dependency", failures)
        require("tinyxml2" in dependency_names, f"{path} is missing tinyxml2 dependency", failures)

        features = data.get("features", {})
        require("raylib" in features, f"{path} is missing raylib feature", failures)
        require("sdl2" in features, f"{path} is missing sdl2 feature", failures)

    require("raylib" in manifest.get("default-features", []), "vcpkg.json should keep raylib as the default feature", failures)

    for token in ("vcpkg_from_github", "SHA512", "FETCHCONTENT_SOURCE_DIR_MICROTEX", "TEXGUI_VCPKG_SOURCE_DIR"):
        require(token in portfile, f"vcpkg overlay portfile is missing {token}", failures)

    for path in (
        "cmake/texgui.pc.in",
        "scripts/verify-cpack.sh",
        "scripts/verify-external-consumers.sh",
        "scripts/verify-path-with-spaces.sh",
        "scripts/verify-pkg-config.sh",
        "scripts/verify-vcpkg-overlay.sh",
        "docs/package-publishing.md",
        "docs/registry-submissions.md",
    ):
        require((ROOT / path).exists(), f"missing release/registry support file: {path}", failures)

    require("CPACK_GENERATOR" in cmake and "CPACK_SOURCE_GENERATOR" in cmake, "CMakeLists.txt is missing CPack generator setup", failures)
    require("TEXGUI_INSTALL_PKGCONFIGDIR" in cmake and "texgui.pc" in cmake, "CMakeLists.txt is missing pkg-config install setup", failures)

    if failures:
        for failure in failures:
            print(failure, file=sys.stderr)
        return 1

    print("registry readiness checks passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
