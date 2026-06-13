---
name: Bug report
about: Something renders wrong, crashes, or fails to build
labels: bug
---

**Describe the bug**

A clear description of what goes wrong.

**To reproduce**

The smallest LaTeX input and code that shows the problem:

```cpp
// ...
```

**Expected / actual behaviour**

For rendering bugs, a screenshot (or the PPM from `texgui_offscreen`) helps a lot.

**Environment**

- OS and compiler (with versions):
- texgui version or commit:
- Backend: raylib / headless / WebAssembly
- How texgui was consumed: FetchContent / add_subdirectory / building this repo

**`TEXGUI_DEBUG=1` stderr output** (if relevant)

```
...
```
