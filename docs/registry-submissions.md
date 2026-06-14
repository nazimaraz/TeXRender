# Registry Submissions

TeXRender already carries starter metadata for Conan and vcpkg. Treat public
registry submission as a follow-up to the first green public tag, not a
pre-push step.

## Pre-Submission Gate

Before opening registry PRs:

- Public CI is green on `main`.
- `v0.1.0` exists and the GitHub release workflow attached archives/checksums.
- `scripts/verify-release.sh` passes locally.
- `scripts/verify-release-archive.sh` passes from the tagged commit.
- `scripts/check-registry-readiness.py` passes from the tagged commit.
- `docs/package-publishing.md` matches the actual package options.
- The support matrix is current.

## vcpkg

The local overlay port lives at:

```text
packaging/vcpkg-overlay/ports/texrender/
```

Before submitting to the public registry:

1. Replace the local `TEXRENDER_VCPKG_SOURCE_DIR` development source path with a
   release archive or GitHub tag source.
2. Add the registry-required source hash.
3. Keep features aligned with `vcpkg.json`: default `raylib`, optional `sdl2`.
4. Validate the default, no-default-feature/headless, `raylib`, and `sdl2`
   combinations.
5. Confirm installed CMake config files land under `lib/cmake/texrender` or the
   layout expected by the registry review.

The local smoke command remains:

```sh
VCPKG_ROOT=/path/to/vcpkg scripts/verify-vcpkg-overlay.sh
TEXRENDER_VCPKG_PACKAGE_SPEC='texrender[sdl2]' \
  VCPKG_ROOT=/path/to/vcpkg scripts/verify-vcpkg-overlay.sh
```

## Conan Center

The local recipe is `conanfile.py`, with a `test_package/` consumer.

Before submitting:

1. Confirm the recipe exports all source, docs, examples, and CMake helper files
   needed for a clean package build.
2. Keep options aligned with CMake: `headless`, `raylib`, `svg`, `sdl2`.
3. Validate at least:

   ```sh
   conan create . --build=missing -o "&:backend=headless" -o "&:with_tests=False"
   conan create . --build=missing -o "&:backend=headless" -o "&:shared=True" -o "&:with_tests=False"
   conan create . --build=missing -o "&:backend=svg" -o "&:with_tests=False"
   conan create . --build=missing -o "&:backend=sdl2" -o "&:with_tests=False"
   ```

4. Add any registry-required patches or metadata in a separate branch so the
   project recipe stays readable.
5. Keep `test_package/` focused on dependency behavior: public headers,
   resource copying, headless rendering, and SVG output.

## Resume Signal

Registry PRs are useful even before merge because reviewers can see:

- package-manager conventions,
- source archive reproducibility,
- CMake install/export correctness,
- dependency feature design,
- downstream consumer tests.
