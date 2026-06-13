## Summary

<!-- What does this change, and why? Link the issue if one exists. -->

-

## Validation

<!-- Commands, presets, or workflows used to verify this change. -->

- [ ] `cmake --build build -j`
- [ ] `ctest --test-dir build --output-on-failure`
- [ ] Added or updated focused tests/docs
- [ ] Ran package/release smoke if CMake exports, package metadata, resources, or workflows changed

## Checklist

- [ ] I have read `CODE_OF_CONDUCT.md` and `CONTRIBUTING.md`
- [ ] Public API changes are documented in `docs/api.md` or Doxygen comments
- [ ] Dependency/package changes are reflected in `docs/package-publishing.md`
- [ ] Release-facing changes are reflected in `CHANGELOG.md`
- [ ] New rendering behavior has headless, SVG, or backend-specific coverage

## Risk

<!-- What could regress? Rendering output, package exports, WebAssembly, resources, etc. -->
