# Local Release Rehearsal

Use this guide before the first public push or any release tag. It focuses on
checks that simulate a new developer or release consumer instead of relying on
the current working directory.

## Fresh Clone

Run:

```sh
scripts/verify-fresh-clone.sh
```

The script clones the current repository into `/tmp/texgui-fresh-clone`, checks
out the current commit, verifies docs/package/API metadata, builds the headless
backend, runs tests, smoke-renders a PNG, installs the package, builds an
installed package consumer, compiles the installed public headers, and builds
the FetchContent consumer example.

For a longer rehearsal that also validates CPack and source release archives:

```sh
TEXGUI_FRESH_CLONE_FULL=1 scripts/verify-fresh-clone.sh
```

By default, the script requires a clean worktree so it cannot accidentally prove
an old commit while local edits are still unstaged. To intentionally test
committed `HEAD` while local edits exist:

```sh
TEXGUI_FRESH_CLONE_ALLOW_DIRTY=1 scripts/verify-fresh-clone.sh
```

## Local Artifact Cleanup

To inspect generated local build artifacts:

```sh
scripts/clean-local-artifacts.sh
```

This is a dry run. It reports generated build/test directories and files that can
be recreated from source. It intentionally leaves IDE settings and personal
ignored files alone.

To remove those generated artifacts:

```sh
scripts/clean-local-artifacts.sh --force
```

Run this before a final fresh-clone rehearsal if you want confidence that the
repository no longer depends on stale local build products.

## Release Body

The first release body is drafted in
[docs/release-body-v0.1.0.md](release-body-v0.1.0.md). Use it as the starting
text for the GitHub release after public CI is green and the tag exists.

## Recommended Final Local Pass

```sh
scripts/check-doc-links.py
scripts/check-registry-readiness.py
scripts/check-public-api.py
scripts/verify-fresh-clone.sh
TEXGUI_FRESH_CLONE_FULL=1 scripts/verify-fresh-clone.sh
scripts/verify-installed-headers.sh
scripts/verify-backend-packages.sh
scripts/verify-pkg-config.sh
scripts/verify-release-archive.sh
scripts/verify-cpack.sh
```

Then push `main`, wait for public CI, and tag only after the first public run is
green.
