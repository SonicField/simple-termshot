# Changelog

## Unreleased

- Replace the approximate bidi resolver with a complete UAX #9 pipeline,
  including isolating run sequences, paired-bracket resolution, real embedding
  levels, explicit failure reporting, and generated Unicode 13.0.0 property
  tables.
- Add both official Unicode 13.0 bidi conformance corpora: all 861,940 test
  executions verify exact paragraph levels, resolved levels, and visual order.
- Document the exact terminal-control, SGR, Unicode-width, output-normalization,
  and bidi-integration boundaries, and align the command-line help with them.
- Discard Unicode `Bidi_Control` input characters so unsupported directional
  controls cannot leak into visual-order snapshots.
- Correct failed-test accounting in the core test runner.

## 0.1.0 — 2026-09-29

- Extract the terminal renderer from `nbs-framework` with its filtered history.
- Render captured terminal output from a file or standard input.
- Support configurable screen dimensions and optional SGR preservation.
- Preserve the original terminal-emulation test suite and add CLI, streaming,
  boundary, sanitizer, and static-analysis checks.
- Add Linux/macOS continuous integration and a dependency-free Make build.
