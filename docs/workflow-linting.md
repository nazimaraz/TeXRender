# Workflow Linting

GitHub Actions YAML can be syntactically valid while still referencing invalid
contexts, misspelled fields, or impossible expressions. texgui uses
[`actionlint`](https://github.com/rhysd/actionlint) to catch those issues.

## GitHub Workflow

The [`Workflow lint`](../.github/workflows/workflow-lint.yml) workflow runs
actionlint over every workflow file.

## Local Run

Install actionlint:

```sh
brew install actionlint
```

or:

```sh
go install github.com/rhysd/actionlint/cmd/actionlint@latest
```

Then run:

```sh
scripts/lint-workflows.sh
```

The release verifier also runs actionlint automatically when it is installed.
