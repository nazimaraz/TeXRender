# Security Threat Model

TeXRender accepts LaTeX-like formula strings and passes them to MicroTeX for parse
and layout. Treat caller-supplied formulas as untrusted input and prefer
`LatexDocument::try_render()` or `try_render_detailed()` at application
boundaries.

## In Scope

- Crashes or memory corruption from crafted formula input.
- Inputs that bypass `RenderOptions` limits.
- Unbounded CPU or memory growth from malformed formulas inside the documented
  guardrails.
- Resource path handling bugs that load unexpected files.
- Thread-safety bugs in shared MicroTeX or glyph-cache paths.
- WebAssembly live-editor crashes caused by arbitrary formula text.

Report these privately through GitHub vulnerability reporting; see
[`SECURITY.md`](../SECURITY.md).

## Out Of Scope

- Full TeX sandboxing inside the caller's process.
- Preventing an application from intentionally passing huge trusted-input
  limits.
- Isolating GUI toolkit bugs in raylib, SDL2, graphics drivers, browsers or
  operating-system font stacks.
- Protecting secrets in formulas that the application itself logs, stores or
  publishes.
- Running arbitrary user workloads with no request limits around texrender.

## Existing Mitigations

- `try_render()` is `noexcept` and catches parser failures at the TeXRender API
  boundary.
- The simple `try_render()` overload applies default limits for input size,
  text size and wrap width.
- `try_render_detailed()` returns a machine-readable `RenderErrorCode` so
  callers can reject unsafe input without parsing exception text.
- Shared render paths are serialized while GUI backends keep their toolkit
  threading rules explicit.
- CI runs guarded render tests, structural visual regression, sanitizer jobs,
  a fuzz smoke, nightly seeded fuzzing, and focused ThreadSanitizer smoke.

## Recommended Application Boundary

- Use `try_render()` or `try_render_detailed()` for user-controlled formulas.
- Set stricter `RenderOptions` for public web forms, chat input, multiplayer
  content, or batch preview services.
- Keep request body limits and rate limits outside texrender.
- Prefer a worker process for hostile internet-scale workloads. The
  [`texrender_untrusted_worker`](../examples/untrusted_worker.cpp) example reads
  one formula from stdin, renders one PNG and exits with a small status code.
  Run it from a parent process that enforces CPU, memory, wall-clock and request
  limits. If the worker crashes or exceeds a limit, kill it, discard the request
  and start a fresh worker for the next formula.
- Log `RenderErrorCode` values, but avoid logging raw private user formulas
  unless your application policy allows it.

## Worker Contract

- Treat each worker process as disposable and single-request by default.
- Send formula text on stdin or another bounded channel rather than command-line
  arguments, which may appear in process listings.
- Apply OS-level timeout and memory limits in the parent process. `RenderOptions`
  reduce parser/layout risk, but they are not a sandbox.
- Store worker outputs in a temporary path controlled by the parent process and
  move successful files into durable storage only after a zero exit status.
- Do not reuse a worker after unexpected termination, sanitizer failure or
  timeout.

## Maintainer Checklist

- Add malformed-input regressions to tests or fuzz corpus when bugs are found.
- Keep `docs/fuzzing.md`, `SECURITY.md`, and this document aligned when the
  guardrail API changes.
- Treat newly discovered parser crashes as security-relevant until triaged.
- Re-run sanitizer and fresh-clone release checks before publishing a fix.
