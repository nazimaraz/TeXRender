# Upstream MicroTeX Patch Note

TeXRender carries one configure-time patch against the pinned MicroTeX checkout.
The patch is applied in [`cmake/Dependencies.cmake`](../cmake/Dependencies.cmake)
before MicroTeX is compiled.

## Issue

MicroTeX's `IndexedArray` binary search initializes the high bound to `_rows`
instead of the last valid row. When a lookup misses, the search can inspect one
row past a static font table. AddressSanitizer reported this while rendering
accent formulas such as:

```tex
\hat{x}+\vec{v}
```

## Local Patch

The local patch changes the upper bound from:

```cpp
int     l = 0, h = _rows;
```

to:

```cpp
int     l = 0, h = static_cast<int>(_rows) - 1;
```

The CMake patch is intentionally strict: if the expected source pattern changes
and the already-patched form is not present, configuration fails instead of
silently compiling an unverified MicroTeX table lookup.

## Upstream Issue/PR Draft

Title:

```text
Fix IndexedArray binary-search upper bound
```

Body:

````markdown
`IndexedArray` initializes the binary-search high bound to `_rows`, but the last
valid row is `_rows - 1`. On misses this can read one row past static font tables.

I hit this under AddressSanitizer while rendering accent formulas such as
`\hat{x}+\vec{v}` through MicroTeX.

Suggested fix:

```diff
- int     l = 0, h = _rows;
+ int     l = 0, h = static_cast<int>(_rows) - 1;
```

This keeps the existing inclusive-bound search logic while avoiding the
one-past-end table read.
````

## Maintainer Follow-Up

- Open an upstream MicroTeX issue or PR with the draft above.
- Keep the local patch until the pinned MicroTeX revision includes the fix.
- When bumping MicroTeX, verify ASan visual regression before removing the local
  patch.
