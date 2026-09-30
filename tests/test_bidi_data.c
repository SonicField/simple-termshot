/* Exhaustive checks for generated bidi lookup tables and their accessors. */
#include "bidi.h"
#include "bidi_data.h"

#include <stdio.h>

int main(void) {
    int range = 0;
    int mirror = 0;

    for (int i = 0; i < BIDI_RANGES_COUNT; i++) {
        if (bidi_ranges[i].lo > bidi_ranges[i].hi ||
            (i > 0 && bidi_ranges[i - 1].hi >= bidi_ranges[i].lo)) {
            fprintf(stderr, "invalid bidi range at index %d\n", i);
            return 1;
        }
    }
    for (uint32_t cp = 0; cp <= 0x10FFFF; cp++) {
        while (range < BIDI_RANGES_COUNT && cp > bidi_ranges[range].hi) range++;
        bidi_type_t expected = BIDI_L;
        if (range < BIDI_RANGES_COUNT && cp >= bidi_ranges[range].lo)
            expected = bidi_ranges[range].type;
        if (bidi_type(cp) != expected) {
            fprintf(stderr, "bidi class mismatch at U+%04X\n", cp);
            return 1;
        }

        while (mirror < BIDI_MIRRORS_COUNT &&
               cp > bidi_mirrors[mirror].codepoint) mirror++;
        uint32_t expected_mirror = cp;
        if (mirror < BIDI_MIRRORS_COUNT &&
            cp == bidi_mirrors[mirror].codepoint)
            expected_mirror = bidi_mirrors[mirror].mirror;
        if (bidi_mirror(cp) != expected_mirror) {
            fprintf(stderr, "bidi mirror mismatch at U+%04X\n", cp);
            return 1;
        }
    }

    for (int i = 0; i < BIDI_BRACKETS_COUNT; i++) {
        const struct bidi_bracket_entry *entry = &bidi_brackets[i];
        uint32_t pair = 0;
        if (i > 0 && bidi_brackets[i - 1].codepoint >= entry->codepoint) {
            fprintf(stderr, "unsorted bidi bracket at index %d\n", i);
            return 1;
        }
        if (bidi_paired_bracket(entry->codepoint, &pair) != entry->kind ||
            pair != entry->pair) {
            fprintf(stderr, "bidi bracket mismatch at U+%04X\n",
                    entry->codepoint);
            return 1;
        }
        if (bidi_type(entry->codepoint) != BIDI_ON) {
            fprintf(stderr, "paired bracket U+%04X is not ON\n",
                    entry->codepoint);
            return 1;
        }
    }

    printf("bidi data: %d ranges, %d brackets, %d mirrors verified\n",
           BIDI_RANGES_COUNT, BIDI_BRACKETS_COUNT, BIDI_MIRRORS_COUNT);
    return 0;
}
