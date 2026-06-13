# Release Assets

This project can be consumed from source, from an installed CMake package, or
through package-manager metadata. The release assets are meant to make those
paths explicit.

## Which Asset Should I Use?

Use the GitHub source archive or the tagged repository when you want
`FetchContent` or `add_subdirectory`:

```cmake
FetchContent_Declare(
    texgui
    GIT_REPOSITORY https://github.com/nazimaraz/latex-renderer.git
    GIT_TAG        v0.1.0
)
```

Use the platform install archives when you want a prebuilt headless package with
headers, libraries, CMake config files, and MicroTeX resources already arranged:

- `texgui-headless-linux-x64.tar.gz`
- `texgui-headless-macos-arm64.tar.gz`
- `texgui-headless-windows-x64.zip`

Use the CPack archives when you want the package format generated directly by
CMake/CPack:

- `latex-renderer-<version>-<system>-<arch>.tar.gz` or `.zip`
- `latex-renderer-<version>-source.tar.gz` or `.zip`

Use vcpkg or Conan when your project already uses those package managers. The
metadata in this repository is validated locally and in CI before public
registry submissions.

## Verifying Downloads

Release workflows attach checksum files and a combined `SHA256SUMS` file. After
downloading an archive, verify it before unpacking:

```sh
shasum -a 256 -c SHA256SUMS
```

On Linux, `sha256sum -c SHA256SUMS` is equivalent.

## Runtime Resources

Every binary/install archive must include the MicroTeX `res/` directory under
the package prefix. CMake consumers should still call:

```cmake
texgui_copy_resources(my_app)
```

That helper copies resources next to the executable and defines
`TEXGUI_RES_DIR` for the target.

## Maintainer Checklist

Before attaching or announcing release assets:

- run `scripts/verify-release.sh`
- run `TEXGUI_FRESH_CLONE_FULL=1 scripts/verify-fresh-clone.sh`
- run `scripts/verify-release-archive.sh`
- run `scripts/verify-cpack.sh`
- run `scripts/verify-pkg-config.sh`
- confirm the `Release smoke` workflow is green for the tag
- confirm `SHA256SUMS` includes every uploaded archive
