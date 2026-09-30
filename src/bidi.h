/*
 * bidi.h — Unicode Bidirectional Algorithm (UAX #9).
 *
 * Reorders a line of Unicode codepoints from logical to visual order.
 * Implements UAX #9 using the bundled, versioned Unicode data declared by
 * BIDI_UNICODE_VERSION in bidi_data.h.
 *
 * No external dependencies. Bundled character type table.
 */

#ifndef TERM_SNAPSHOT_BIDI_H
#define TERM_SNAPSHOT_BIDI_H

#include <stdint.h>
#include <stddef.h>

/* UAX #9 Bidi character types */
typedef enum {
    /* Strong types */
    BIDI_L   = 0,   /* Left-to-Right */
    BIDI_R   = 1,   /* Right-to-Left */
    BIDI_AL  = 2,   /* Arabic Letter */

    /* Weak types */
    BIDI_EN  = 3,   /* European Number */
    BIDI_ES  = 4,   /* European Separator */
    BIDI_ET  = 5,   /* European Terminator */
    BIDI_AN  = 6,   /* Arabic Number */
    BIDI_CS  = 7,   /* Common Separator */
    BIDI_NSM = 8,   /* Non-Spacing Mark */
    BIDI_BN  = 9,   /* Boundary Neutral */

    /* Neutral types */
    BIDI_B   = 10,  /* Paragraph Separator */
    BIDI_S   = 11,  /* Segment Separator */
    BIDI_WS  = 12,  /* Whitespace */
    BIDI_ON  = 13,  /* Other Neutral */

    /* Explicit formatting types */
    BIDI_LRE = 14,  /* Left-to-Right Embedding */
    BIDI_RLE = 15,  /* Right-to-Left Embedding */
    BIDI_LRO = 16,  /* Left-to-Right Override */
    BIDI_RLO = 17,  /* Right-to-Left Override */
    BIDI_PDF = 18,  /* Pop Directional Format */
    BIDI_LRI = 19,  /* Left-to-Right Isolate */
    BIDI_RLI = 20,  /* Right-to-Left Isolate */
    BIDI_FSI = 21,  /* First Strong Isolate */
    BIDI_PDI = 22,  /* Pop Directional Isolate */
} bidi_type_t;

typedef enum {
    BIDI_SUCCESS = 0,
    BIDI_ERROR_INVALID_ARGUMENT = -1,
    BIDI_ERROR_NO_MEMORY = -2,
} bidi_status_t;

typedef enum {
    BIDI_BRACKET_NONE = 0,
    BIDI_BRACKET_OPEN = 1,
    BIDI_BRACKET_CLOSE = 2,
} bidi_bracket_type_t;

/*
 * Look up the bidi character type for a Unicode scalar value.
 */
bidi_type_t bidi_type(uint32_t cp);

/*
 * Reorder a line of codepoints from logical to visual order.
 *
 * codepoints: array of Unicode codepoints (logical order)
 * count:      number of codepoints
 * visual_map: output array of indices (visual_map[visual_pos] = logical_pos)
 *             Caller allocates, must have room for `count` entries.
 * base_dir:   0 = auto-detect, 1 = force LTR, 2 = force RTL
 *
 * Returns the resolved paragraph direction (0=LTR, 1=RTL).
 */
int bidi_reorder(const uint32_t *codepoints, int count,
                 int *visual_map, int base_dir);

/*
 * Resolve one Unicode bidi paragraph.
 *
 * out_levels receives the resolved level for every input codepoint.  Entries
 * removed by rule X9 receive -1.  visual_map receives logical indices in
 * display order; X9-removed entries are omitted and out_visual_count reports
 * the resulting length.  base_dir is 0 for auto, 1 for LTR, or 2 for RTL.
 *
 * Surrogates and values above U+10FFFF are rejected. Returns BIDI_SUCCESS or
 * an error. No partial result is valid on error.
 */
bidi_status_t bidi_resolve(const uint32_t *codepoints, int count,
                           int *visual_map, int *out_levels,
                           int base_dir, int *out_paragraph_level,
                           int *out_visual_count);

/*
 * Return the bidi mirrored glyph for a codepoint, or the codepoint
 * itself if no mirror exists. Used for brackets in RTL context (UAX #9 L4).
 */
uint32_t bidi_mirror(uint32_t cp);

/* Return the paired-bracket type and, when non-NULL, its paired codepoint. */
bidi_bracket_type_t bidi_paired_bracket(uint32_t cp,
                                        uint32_t *paired_codepoint);

/*
 * Compatibility wrapper returning the paragraph level, or -1 on failure.
 * New callers should use bidi_resolve so allocation failures and the visual
 * length after X9 removals are explicit.
 */
int bidi_reorder_with_levels(const uint32_t *codepoints, int count,
                             int *visual_map, int *out_levels,
                             int base_dir);

#endif /* TERM_SNAPSHOT_BIDI_H */
