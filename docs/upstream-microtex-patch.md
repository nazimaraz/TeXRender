# Upstream MicroTeX Patch Note

TeXRender carries configure-time patches against the pinned MicroTeX checkout.
They are applied in [`cmake/Dependencies.cmake`](../cmake/Dependencies.cmake)
before MicroTeX is compiled.

## IndexedArray Bounds

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

## Accent Script Null Base

MicroTeX can parse a malformed accent command with no base atom, then attach a
script to that accent. `AccentedAtom::createBox` handles a null base by using an
empty box, but `ScriptsAtom::createBox` has a special case for accented bases
that dereferences the accent's base directly. Fuzzing found this repro:

```tex
\frac{a+q{\r}'st{x1+0}}
```

## Local Patch

The local patch changes the scripted-accent path from:

```cpp
auto box = acc->_base->createBox(*(env.crampStyle()));
```

to:

```cpp
auto box = (
  acc->_base == nullptr
  ? sptrOf<StrutBox>(0.f, 0.f, 0.f, 0.f)
  : acc->_base->createBox(*(env.crampStyle()))
);
```

This mirrors the null-base handling already used by `AccentedAtom::createBox`
and prevents a layout-time null dereference for malformed input.

## Upstream Issue/PR Draft

Title:

```text
Handle null-base accented atoms in ScriptsAtom
```

Body:

````markdown
`AccentedAtom::createBox` handles a null base atom by using an empty `StrutBox`,
but `ScriptsAtom::createBox` has an accented-base special case that dereferences
`acc->_base` directly.

Malformed input can produce an accented atom with no base and then attach a
script to it. One fuzzed repro is:

```tex
\frac{a+q{\r}'st{x1+0}}
```

Suggested fix:

```diff
- auto box = acc->_base->createBox(*(env.crampStyle()));
+ auto box = (
+   acc->_base == nullptr
+   ? sptrOf<StrutBox>(0.f, 0.f, 0.f, 0.f)
+   : acc->_base->createBox(*(env.crampStyle()))
+ );
```

This matches the existing null-base behavior in `AccentedAtom::createBox`.
````

## MathAtom Null Base

MicroTeX can also produce a `MathAtom` wrapper with no base atom for malformed
inline-math delimiters. Fuzzing found this repro:

```tex
\fr"ca+{}{\(}}
```

`MathAtom::createBox` dereferences `_base` directly, while several other atom
types already use an empty `StrutBox` when their content is absent.

## Local Patch

The local patch changes:

```cpp
auto box = _base->createBox(e);
```

to:

```cpp
auto box = (
  _base == nullptr
  ? sptrOf<StrutBox>(0.f, 0.f, 0.f, 0.f)
  : _base->createBox(e)
);
```

## Upstream Issue/PR Draft

Title:

```text
Handle null-base MathAtom layout
```

Body:

````markdown
Malformed inline-math delimiters can produce a `MathAtom` with no base atom.
`MathAtom::createBox` currently dereferences `_base` directly.

One fuzzed repro is:

```tex
\fr"ca+{}{\(}}
```

Suggested fix:

```diff
- auto box = _base->createBox(e);
+ auto box = (
+   _base == nullptr
+   ? sptrOf<StrutBox>(0.f, 0.f, 0.f, 0.f)
+   : _base->createBox(e)
+ );
```

This follows the empty-content handling used by other atom layout paths.
````

## RotateAtom Null Base

MicroTeX's `\T` macro can construct a `RotateAtom` around an empty formula when
the macro input is malformed. Fuzzing found this repro:

```tex
\T$icnKaiTi?{x^FHe}}
e}}
```

`RotateAtom` dereferences the incoming base in its constructors to copy the atom
type, then dereferences it again during layout.

## Local Patch

The local patch changes the constructor setup from:

```cpp
_type = base->_type;
```

to:

```cpp
_type = base == nullptr ? AtomType::ordinary : base->_type;
```

and changes rotate layout calls from direct `_base->createBox(env)` use to an
empty `StrutBox` fallback when `_base` is null.

## Upstream Issue/PR Draft

Title:

```text
Handle null-base RotateAtom construction and layout
```

Body:

````markdown
Malformed `\T` / rotate macro input can construct a `RotateAtom` with a null
base atom. The constructors currently copy `base->_type`, and `createBox`
dereferences `_base` again during layout.

One fuzzed repro is:

```tex
\T$icnKaiTi?{x^FHe}}
e}}
```

Suggested constructor fix:

```diff
- _type = base->_type;
+ _type = base == nullptr ? AtomType::ordinary : base->_type;
```

Suggested layout fix:

```diff
- _base->createBox(env)
+ (_base == nullptr ? sptrOf<StrutBox>(0.f, 0.f, 0.f, 0.f) : _base->createBox(env))
```

This follows the empty-content handling used by other atom wrappers.
````

## UnderOverAtom Null Base

Malformed accent/limit commands can produce an `UnderOverAtom` with no base
atom. Fuzzing found this repro:

```tex
\b@dMMMMMMMMM+
```

`UnderOverAtom::createBox` already has an empty `StrutBox` fallback later in the
layout path, but `leftType()`, `rightType()` and an early limits check read the
base first.

## Local Patch

The local patch returns `AtomType::ordinary` from `leftType()`/`rightType()` when
the base is null, and skips the early limits branch unless `_base` is present.

## Upstream Issue/PR Draft

Title:

```text
Guard null-base UnderOverAtom type and limits reads
```

Body:

````markdown
Malformed accent/limit commands can produce an `UnderOverAtom` with a null base.
`UnderOverAtom::createBox` has an empty-content fallback later, but
`leftType()`, `rightType()` and the early limits branch dereference `_base`
before reaching it.

One fuzzed repro is:

```tex
\b@dMMMMMMMMM+
```

Suggested fix:

- Return `AtomType::ordinary` from `leftType()`/`rightType()` when `_base` is
  null.
- Only evaluate `_base->_limitsType` when `_base` is present.
````

## CedillaAtom Null Base

Malformed cedilla accent input can produce a `CedillaAtom` with no base atom.
Fuzzing found this repro:

```tex
\c@@pp[o\
```

`BoldAtom`, immediately above `CedillaAtom` in MicroTeX's atom implementation,
already renders a null base as an empty `StrutBox`; `CedillaAtom` can follow the
same behavior.

## Local Patch

The local patch changes:

```cpp
auto b = _base->createBox(env);
```

to:

```cpp
auto b = _base != nullptr ? _base->createBox(env) : sptrOf<StrutBox>(0.f, 0.f, 0.f, 0.f);
```

## Upstream Issue/PR Draft

Title:

```text
Handle null-base CedillaAtom layout
```

Body:

````markdown
Malformed cedilla accent input can produce a `CedillaAtom` with a null base.
`CedillaAtom::createBox` dereferences `_base`, while neighboring wrapper atoms
such as `BoldAtom` already use an empty `StrutBox` fallback for absent content.

One fuzzed repro is:

```tex
\c@@pp[o\
```

Suggested fix:

```diff
- auto b = _base->createBox(env);
+ auto b = _base != nullptr ? _base->createBox(env) : sptrOf<StrutBox>(0.f, 0.f, 0.f, 0.f);
```
````

## Inline Wrapper Null Bases

Several inline wrapper atoms in `atom_impl.h` share the same direct
`_base->createBox(env)` assumption. Fuzzing found the same null-base pattern in
`SmallCapAtom` with this repro:

```tex
backs$a\lc{\sc$
```

## Local Patch

The local patch rewrites the remaining common inline wrapper forms:

```cpp
auto box = _base->createBox(env);
auto base = _base->createBox(env);
auto bbase = _base->createBox(env);
```

to use an empty `StrutBox` when `_base` is null. The CMake patch fails
configuration if a direct `_base->createBox(env);` call remains in
`atom_impl.h`.
