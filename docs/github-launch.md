# GitHub Launch Checklist

Use this when the local repository is ready to become the public project.

## One-Time Repository Setup

1. Create the GitHub repository, then add the remote:

   ```sh
   git remote add origin git@github.com:nazimaraz/latex-renderer.git
   git remote -v
   ```

2. Push `main` and tags only after local verification is green:

   ```sh
   scripts/verify-release.sh
   scripts/prepare-public-release.sh
   git push -u origin main
   ```

3. In GitHub repository settings:

   - Enable GitHub Pages with source set to **GitHub Actions**.
   - Enable Dependabot alerts and private vulnerability reporting.
   - Enable Discussions only if you want public Q&A before the first release.
   - Add topics: `cpp`, `latex`, `renderer`, `raylib`, `sdl2`, `svg`,
     `freetype`, `webassembly`, `cmake`, `graphics`.
   - Add description:
     `C++ LaTeX math renderer for raylib, SDL2, headless RGBA output, SVG, and WebAssembly.`

4. After the first push, wait for these workflows:

   - CI
   - Package managers
   - Registry readiness
   - Static analysis
   - Thread sanitizer, after its first scheduled/manual run
   - CodeQL
   - Coverage
   - Workflow lint
   - Benchmarks
   - Web demo
   - Release smoke
   - Nightly fuzz, after its first scheduled/manual run

5. Fix first-run CI failures before tagging. Public CI is the real portability
   check for package managers, SDL2, Windows shared-library behavior, and Pages.
   Confirm CODEOWNERS, support, code-of-conduct, citation and release-asset docs
   render correctly in the GitHub UI.

## Branch Protection

After the first green run, protect `main`:

- Require pull requests before merging.
- Require status checks that match the workflows above.
- Require branches to be up to date before merging.
- Allow squash merging to keep release history clean.

For a personal showcase repository, branch protection can be enabled after the
first tag if you want the initial import to stay simple.

## Cutting `v0.1.0`

Only tag after public CI is green:

```sh
scripts/prepare-public-release.sh v0.1.0
git push origin v0.1.0
```

The `Release` workflow builds install archives, CPack archives and checksum
files. The `Release smoke` workflow validates source archives, CPack archives,
external consumers and paths with spaces. Locally, `scripts/verify-cpack.sh`
validates CPack binary/source archives before those artifacts are attached to a
public release. Use [`docs/release-assets.md`](release-assets.md) to explain the
download choices in the release body.

## First Failure Triage

- Package-manager failures usually mean a dependency name, feature, or exported
  target differs between local CMake and the registry environment.
- Windows failures usually involve DLL copying, export macros, path quoting, or
  multi-config generator assumptions.
- WebAssembly failures usually involve Emscripten SDK version drift or resource
  packaging.
- CodeQL failures are usually real code-quality findings or a missing build
  dependency in the workflow.
