# Support

Thanks for trying latex-renderer. This project is maintained as an open-source
library and portfolio project, so support is best-effort.

## Questions

For usage questions, start with:

- [README.md](README.md)
- [docs/integration-examples.md](docs/integration-examples.md)
- [docs/api.md](docs/api.md)
- [docs/known-limitations.md](docs/known-limitations.md)
- [docs/why-not-just-tex.md](docs/why-not-just-tex.md)

If those do not answer the question, open a GitHub Discussion if discussions are
enabled, or open an issue with the "question" context in the description.

## Bug Reports

Open an issue with:

- your platform, compiler, CMake version, and backend
- the LaTeX input or minimal code that reproduces the issue
- whether you use FetchContent, add_subdirectory, installed CMake, vcpkg, or
  Conan
- the full configure/build/runtime error, preferably as text

For package-manager issues, also include the output of:

```sh
scripts/check-registry-readiness.py
```

## Security

Do not open public issues for security vulnerabilities. Follow
[SECURITY.md](SECURITY.md) instead.

## Scope

This project does not provide paid support, full TeX document processing, or
application-specific GUI integration services. Pull requests that improve the
documented library surface, tests, packaging, or examples are welcome.
