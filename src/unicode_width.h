/*
 * unicode_width.h — Unicode character width lookup.
 *
 * Returns display width of a Unicode codepoint:
 *   0  combining mark, zero-width character
 *   1  normal width
 *   2  wide (CJK, fullwidth, emoji)
 *  -1  non-printable control character
 */

#ifndef TERM_SNAPSHOT_UNICODE_WIDTH_H
#define TERM_SNAPSHOT_UNICODE_WIDTH_H

#include <stdint.h>

int unicode_width(uint32_t cp);

#endif /* TERM_SNAPSHOT_UNICODE_WIDTH_H */
