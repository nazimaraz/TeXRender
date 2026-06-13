# Security Policy

## Supported versions

| Version        | Supported |
| -------------- | --------- |
| latest release | ✅        |

## Reporting a vulnerability

Please **do not** open a public issue for security problems. Use GitHub's
private vulnerability reporting on this repository (Security tab → "Report a
vulnerability"). You can expect an initial response within a week.

texgui is explicitly designed to accept **untrusted LaTeX input** through
`LatexDocument::try_render` (for example, the live editor feeds it raw user
keystrokes). The simple `try_render()` overload applies default guardrails for
input size, text size and wrap width; callers with different requirements can
set explicit `RenderOptions` limits.

Crashes, memory corruption, guardrail bypasses, or unbounded resource
consumption triggered by crafted input are therefore in scope and treated as
security bugs, not just rendering bugs. See [`docs/fuzzing.md`](docs/fuzzing.md)
for the sanitizer-backed fuzz harness used to exercise this boundary.
