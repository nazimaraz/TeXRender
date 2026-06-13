# First Public Push Dry Run

Use this immediately before publishing the repository. It is intentionally
shorter than the full release checklist and focuses on the first public GitHub
experience.

## Local Gate

Run from a clean worktree:

```sh
scripts/check-doc-links.py
scripts/check-registry-readiness.py
scripts/check-public-api.py
scripts/lint-workflows.sh
TEXGUI_FRESH_CLONE_FULL=1 scripts/verify-fresh-clone.sh
scripts/prepare-public-release.sh
```

If `scripts/lint-workflows.sh` reports that `actionlint` is missing, install it
locally and rerun the script before pushing.

## First Push

```sh
git remote -v
git push -u origin main
```

After the push:

- Confirm README badges resolve on GitHub.
- Confirm images, gallery links and docs render in the GitHub UI.
- Enable GitHub Pages with **GitHub Actions** as the source.
- Enable private vulnerability reporting.
- Confirm issue forms show one bug option and one feature option.
- Watch CI, Package managers, Registry readiness, Static analysis, CodeQL,
  Coverage, Workflow lint, Benchmarks, Web demo and Release smoke.
- Manually start Thread sanitizer and Nightly fuzz once if you do not want to
  wait for their schedules.

## First Tag

Only tag after the first public CI run is green:

```sh
scripts/prepare-public-release.sh v0.1.0
git push origin v0.1.0
```

Then verify:

- The `Release` workflow uploads install archives, CPack archives and
  `SHA256SUMS`.
- The release body starts from
  [`docs/release-body-v0.1.0.md`](release-body-v0.1.0.md).
- The live editor URL and API docs URL work.
- [`docs/release-assets.md`](release-assets.md) explains which archive a user
  should download.

## After The Tag

- Open the first registry submission PRs only after release assets are stable.
- Check Dependabot's first pull request for labels, title style and workflow
  behavior.
- Revisit branch protection once the first public workflow names are known.
