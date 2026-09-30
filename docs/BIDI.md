# Bidirectional text verification

The bundled bidi resolver implements Unicode Standard Annex #9. The resolver
includes explicit embeddings and overrides, isolates, isolating run sequences,
weak and neutral resolution, paired-bracket rule N0, implicit levels, line
resets, reordering, and mirrored-character lookup.

The terminal renderer applies the resolver independently to each screen row
when it creates a snapshot. It auto-detects the paragraph direction and emits
the row in visual order. A consumer that applies its own bidi algorithm to this
already reordered text can therefore display a different order.

## Terminal integration boundary

The UAX #9 conformance claim applies to the resolver in `src/bidi.c`. The
fixed-cell terminal model passes the first Unicode scalar value stored in each
screen cell to that resolver. Combining marks stored with a base character move
with that cell.

The terminal input path discards every character with the Unicode
`Bidi_Control` property: ALM, LRM, RLM, LRE, RLE, PDF, LRO, RLO, LRI, RLI, FSI,
and PDI. These controls neither affect snapshot ordering nor appear in output.
The resolver API still implements their UAX #9 semantics for callers that pass
logical codepoint sequences directly, and the conformance suite covers them.

UAX #9 determines ordering, not complete text shaping. `simple-termshot` does
not perform Arabic joining, ligature formation, font selection, or general
grapheme-cluster shaping. It applies the bundled mirroring mapping to characters
at odd resolved levels. See [`TERMINAL.md`](TERMINAL.md) for the wider Unicode
and fixed-cell behavior.

## Unicode data

The bundled bidi classes, paired brackets, and mirroring mappings are pinned to
Unicode 13.0.0 in `src/bidi_data.h`. They are generated directly from the
checked-in Unicode Character Database files:

```sh
perl tools/generate_bidi_data.pl > src/bidi_data.h
```

The generator verifies the version headers and `make test-bidi-conformance`
regenerates and compares the header before testing. An upgrade is therefore an
explicit source-data change rather than an accidental consequence of the build
host. The generated properties are covered by `LICENSE-UNICODE`.

## Conformance suite

Development and CI run both official Unicode 13.0 conformance corpora:

```sh
make test-bidi-conformance
```

`BidiTest.txt` exhaustively exercises bidi-class combinations up to its stated
limit and adds longer pathological cases. `BidiCharacterTest.txt` covers real
codepoints, including paired brackets and canonical equivalents. Together they
currently execute 861,940 cases and compare the exact paragraph level, resolved
levels (including X9 removals), and visual ordering required by Unicode.

The checked-in source files and their SHA-256 digests are documented in
`tests/unicode/13.0.0/README.md`. Passing these corpora is strong evidence for
the resolver, not a mathematical proof and not evidence that the terminal
input path accepts directional formatting controls. As the corpus headers
note, rules L3 and L4 are rendering responsibilities.

The same target also exhaustively checks lookup behavior for all Unicode scalar
values against the generated tables: 699 bidi ranges, 120 paired brackets, and
420 character-mirroring mappings.
