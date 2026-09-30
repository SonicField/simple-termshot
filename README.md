# simple-termshot

`simple-termshot` turns captured terminal output into the final screen a person
would have seen. It understands cursor movement, overwriting, clearing,
scrolling, colours, Unicode width, and bidirectional text instead of treating
the input as an ordinary text stream.

It is a small C program with no runtime dependencies beyond a POSIX-like
system. The normal output is plain UTF-8, which makes it useful in scripts,
tests, logs, and AI workflows.

For example, a carriage-return progress display contains both updates:

```sh
printf 'Progress 10%%\rProgress 100%%\n' | simple-termshot
```

The result contains only the final visible state:

```text
Progress 100%
```

## Build and install

```sh
make
make test
```

This produces `./simple-termshot`. To install it:

```sh
make install PREFIX="$HOME/.local"
```

`PREFIX` defaults to `/usr/local`. Packagers can stage an installation with
`DESTDIR`. CI builds and tests Ubuntu 24.04 on x86-64 and ARM64 with GCC and
Clang, and macOS 15 on Intel and Apple silicon with Apple Clang.

## Usage

```sh
simple-termshot capture.log
simple-termshot < capture.log
command-producing-terminal-codes | simple-termshot
simple-termshot --width=120 --height=40 capture.log
```

Options:

```text
--width=COLUMNS   Screen width (default: 80; range: 1–10000)
--height=ROWS     Screen height (default: 24; range: 1–10000)
--preserve-sgr    Re-create colour and style sequences in the result
--no-strip        Compatibility alias for --preserve-sgr
-h, --help        Show help
-V, --version     Show the version
```

Use `-` as the filename to explicitly read standard input. Choose dimensions
that match the terminal which produced the capture: wrapping and cursor
positions depend on them.

By default, decoration is removed. `--preserve-sgr` emits terminal escape
sequences, so redirect its output to a file first when handling untrusted data.

## What it understands

- VT100/xterm cursor positioning and relative movement
- Screen and line erase operations
- Scrolling and scroll regions
- Line and character insertion/deletion
- Tabs, cursor save/restore, reset, CR, LF, backspace, and reverse index
- UTF-8, combining marks, CJK/emoji display widths, and bidirectional text
- SGR attributes and 16/256-colour forms when preservation is requested
- Safe consumption of OSC and DCS strings

This is deliberately a focused terminal renderer, not a complete xterm. It
produces the final fixed-size screen, not scrollback or a time-based replay.
Unknown and unsupported controls are ignored. Alternate-screen mode switches
are ignored, although the clear-and-home sequences applications normally send
with them are handled.

## Tests

```sh
make test       # 104 core cases plus black-box CLI checks
make sanitize   # tests under AddressSanitizer and UBSan
make analyze    # GCC path-sensitive static analysis
make test-bidi-conformance # official Unicode 13.0 UAX #9 corpora
```

Warnings fail the build. The suite covers realistic terminal sessions,
malformed sequences, split input chunks, Unicode, wide characters, bidi,
scrolling, cursor operations, and SGR handling.

Bidirectional processing implements UAX #9 with bundled Unicode 13.0.0
properties. Its algorithm, data provenance, and conformance evidence are
documented in [`docs/BIDI.md`](docs/BIDI.md).

## Code layout

- `src/term_snapshot.c` — streaming terminal state machine and screen buffer
- `src/unicode_width.c` — display widths for Unicode characters
- `src/bidi.c` — bidirectional reordering and bracket mirroring
- `src/term_style.c` — controlled SGR reconstruction
- `src/main.c` — file/stdin command-line interface

The extraction history and its verification evidence are recorded in
[`docs/EXTRACTION.md`](docs/EXTRACTION.md).

## Origin and license

`simple-termshot` was extracted from
[`SonicField/nbs-framework`](https://github.com/SonicField/nbs-framework).
The path-filtered Git history retains the original authorship and commit
messages.

Released under the [MIT License](LICENSE). Generated Unicode property data is
covered by the [Unicode Data Files and Software License](LICENSE-UNICODE).
