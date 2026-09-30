# Bidirectional text verification

`simple-termshot` implements Unicode Standard Annex #9 for each rendered line.
The implementation includes explicit embeddings and overrides, isolates,
isolating run sequences, weak and neutral resolution, paired-bracket rule N0,
implicit levels, line resets, reordering, and glyph mirroring.

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
`tests/unicode/13.0.0/README.md`. Passing these corpora is strong conformance
evidence, not a mathematical proof of correctness outside their scope; as the
file headers note, rules L3 and L4 are rendering responsibilities.

The same target also exhaustively checks lookup behavior for all Unicode scalar
values against the generated tables: 699 bidi ranges, 120 paired brackets, and
420 character-mirroring mappings.
