# Terminal behavior

This document defines the terminal behavior that `simple-termshot` supports.
Behavior outside this subset is not an xterm compatibility guarantee.

## Input and output

The command reads one byte stream from standard input or one named file. It
maintains one fixed-size primary screen and emits a snapshot after end of input.
The default dimensions are 80 columns by 24 rows; each dimension must be
between 1 and 10000. Dimensions must match the terminal that produced the
capture because cursor positions and wrapping depend on them.

The renderer keeps only the screen grid. It does not retain scrollback, event
timing, or intermediate frames. Output rows have trailing spaces removed and
trailing blank rows omitted. Successful output is newline-terminated; an empty
screen is one newline.

Bare LF uses newline mode: it moves down one row and returns to column zero.
This deliberate behavior matches common captured Unix output, but it is not a
claim that every source terminal had LNM enabled.

## Supported controls

Only 7-bit ESC-prefixed forms are recognized. C1 single-byte forms are not
supported.

| Area | Supported controls and behavior |
|---|---|
| Cursor movement | CUU, CUD, CUF, CUB, CNL, CPL, CHA, CUP, HVP, and VPA |
| Erasure | ED modes 0, 1, 2, and 3; EL modes 0, 1, and 2; ECH |
| Editing | IL, DL, ICH, and DCH |
| Scrolling | IND, RI, SU, SD, and DECSTBM scroll regions |
| Lines and cursor | CR, LF with implied CR, NEL, BS, delayed auto-wrap, DECSC, and DECRC |
| Tabs | HT with stops every eight columns, HTS, and TBC modes 0 and 3 |
| Reset | RIS clears the screen and resets cursor, scroll region, tabs, parser state, and active style |
| Styling | The SGR subset described below |
| Strings | OSC content through BEL or ST; DCS content through ST |

ED mode 3 has the same visible effect as mode 2 because the renderer has no
scrollback to erase. DECSC and DECRC save and restore the cursor position, not
the full collection of terminal attributes.

## Styling

SGR is ignored by default, so normal output contains no reconstructed colour or
attribute escapes. With `--preserve-sgr`, the renderer stores supported style
state with each cell and emits the transitions needed to reproduce that state
in visual output. It does not reproduce the original SGR byte stream.

Supported attributes are bold, dim, italic, underline, blink, inverse, and
strikethrough, including their documented reset forms. Supported colours are
the standard and bright 16 colours and indexed 256-colour foreground and
background forms. RGB foreground and background input is quantised to the
xterm 256-colour palette; output remains indexed colour.

Preserved SGR output contains terminal escape sequences. Store or inspect that
output safely before displaying data from an untrusted source.

## Unicode model

Input text is expected to be valid UTF-8. The parser accepts two-, three-, and
four-byte sequences across arbitrary input chunks. Invalid leading bytes,
stray continuation bytes, and interrupted sequences are discarded; malformed
scalar encodings are outside the supported input contract.

Cell widths use bundled Unicode 15.1 East Asian Width and General Category
data. Combining and selected format characters have width zero. East Asian
Wide and Fullwidth characters, together with the bundled emoji ranges, have
width two. Other printable scalar values, including East Asian Ambiguous
characters, have width one. Width selection does not depend on the host locale.

A zero-width character is appended to the preceding primary cell when the
cell's eight-byte storage permits it. A zero-width character with no preceding
cell, or one that exceeds the cell storage, is discarded. The renderer does
not implement general grapheme segmentation, emoji ZWJ composition, flag
composition, font shaping, or Arabic joining.

At snapshot time, the renderer applies its Unicode 13.0 UAX #9 resolver to the
first scalar value in each screen cell and emits visual-order text. Combining
bytes stored with a base character move with that cell. Directional formatting
characters stored as zero-width additions are not independent resolver items,
so embeddings, overrides, and isolates from the terminal input are not a
supported way to control snapshot ordering. The resolver itself and its
conformance scope are documented in [`BIDI.md`](BIDI.md).

## Ignored and unsupported behavior

Private CSI forms beginning with `?`, `>`, or `!` are ignored. SM and RM mode
changes and device-status reports are also ignored. This includes
alternate-screen, cursor-visibility, origin, insert, and auto-wrap mode
switches. The renderer neither sends terminal replies nor changes behavior in
response to those modes.

Character-set designation, terminal resize events, mouse protocols,
hyperlinks, clipboard operations, images, sixel graphics, and other
unsupported ESC or CSI sequences are not interpreted. OSC and DCS payloads are
consumed rather than displayed; an unterminated payload consumes the remainder
of the input. Unknown sequences are ignored after their bytes are consumed.

The renderer is not a security boundary. Plain output removes the terminal
controls recognized by this parser, but it can retain meaningful Unicode
format characters and must not be treated as safe shell, HTML, or programming
language input.

## Exit status

| Status | Meaning |
|---|---|
| 0 | The input was rendered successfully. |
| 1 | An allocation, input, output, open, or close operation failed. |
| 4 | Command-line arguments were invalid. |
