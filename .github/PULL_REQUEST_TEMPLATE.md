## Summary

<!-- What does this change, and why? Link the issue if one exists. -->

## Testing

<!-- How was this verified? -->

- [ ] `ctest` passes with the default (raylib) configuration
- [ ] `ctest` passes with `-DTEXGUI_BACKEND=headless`
- [ ] Public API docs updated for public header changes
- [ ] Consumer/package examples updated for dependency-story changes
- [ ] Touched files are formatted: `git ls-files '*.cpp' '*.hpp' | xargs clang-format -i`
- [ ] Tests added or updated for behaviour changes

## Risk

<!-- What could regress? Rendering output, package exports, WebAssembly, resource paths, etc. -->
