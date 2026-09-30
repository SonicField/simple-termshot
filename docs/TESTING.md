# Terminal contract verification

This document maps each externally observable claim in
[`TERMINAL.md`](TERMINAL.md) to a test that would fail if the behavior changed.
Test names refer to `tests/test_term_snapshot.c`; quoted descriptions refer to
assertions in `tests/test_cli.sh`.

## Input, screen, and output

| Contract claim | Direct evidence |
|---|---|
| Read standard input, explicit `-`, or one named file | `explicit dash reads standard input`; `reads a capture directly from a file`; `rejects more than one input file` |
| Use an 80 by 24 screen by default | `default width is 80 columns`; `default height is 24 rows` |
| Accept dimensions 1 through 10000 and reject values outside that range | `test_accepts_dimension_boundaries`; `test_rejects_excessive_dimensions`; `invalid dimensions return usage status` |
| Keep the final fixed-size screen rather than intermediate frames | `test_progress_bar_simulation`; `carriage-return progress update` |
| Keep no scrollback | `test_wrap_causes_scroll`; `test_scroll_up_via_newline` |
| Trim trailing spaces and trailing blank rows | `test_trailing_spaces_trimmed`; `test_trailing_blank_rows_omitted` |
| Terminate output with a newline, including an empty screen | `test_simple_text`; `test_empty_screen` |
| Treat bare LF as newline plus carriage return | `test_bare_lf`; `test_bare_lf_after_cursor_move` |

## Controls

| Contract claim | Direct evidence |
|---|---|
| Recognize 7-bit CSI but not its C1 single-byte form | `test_cursor_position`; `test_c1_csi_unsupported` |
| CUU, CUD, CUF, and CUB | `test_cursor_up`; `test_cursor_down`; `test_cursor_forward`; `test_cursor_backward` |
| CNL, CPL, CHA, CUP, HVP, and VPA | `test_cursor_next_line`; `test_cursor_previous_line`; `test_cursor_horizontal_absolute`; `test_cursor_position`; `test_hvp_cursor_position`; `test_vertical_position_absolute` |
| ED modes 0, 1, 2, and 3 | `test_erase_to_end_of_screen`; `test_erase_to_start_of_screen`; `test_erase_entire_screen`; `test_erase_scrollback_mode` |
| EL modes 0, 1, and 2, plus ECH | `test_erase_to_end_of_line`; `test_erase_to_start_of_line`; `test_erase_entire_line`; `test_erase_characters` |
| IL, DL, ICH, and DCH | `test_insert_lines`; `test_delete_lines`; `test_insert_characters`; `test_delete_characters` |
| IND, RI, SU, and SD | `test_index_control`; `test_reverse_index`; `test_scroll_up_csi`; `test_scroll_down_csi` |
| DECSTBM scrolling regions | `test_scroll_region` |
| CR, LF, NEL, BS, and delayed auto-wrap | `test_carriage_return`; `test_bare_lf`; `test_next_line_control`; `test_backspace`; `test_wrap_at_right_margin` |
| DECSC and DECRC save the cursor but not style | `test_cursor_save_restore`; `test_cursor_restore_does_not_restore_style` |
| HT defaults, HTS, and TBC modes 0 and 3 | `test_tab_default_stops`; `test_horizontal_tab_set`; `test_tab_clear_current`; `test_tab_clear_all` |
| RIS clears the screen and resets cursor, scroll region, tabs, parser, and style | `test_full_reset`; `test_reset_scroll_region`; `test_reset_tab_stops`; `test_reset_parser_state`; `test_reset_active_style` |
| OSC ends at BEL or ST; DCS ends at ST | `test_osc_title_bel`; `test_osc_title_st`; `test_dcs_consumed` |
| Unterminated OSC or DCS consumes the remainder | `test_unterminated_osc_consumes_remainder`; `test_unterminated_dcs_consumes_remainder` |

## Styling

| Contract claim | Direct evidence |
|---|---|
| Strip SGR by default | `test_sgr_stripped`; `test_preserve_sgr_off_is_default` |
| Preserve per-cell style and emit reconstructed transitions on request | `test_preserve_sgr_style_changes`; `descriptive SGR preservation option` |
| Preserve bold, dim, italic, underline, blink, inverse, strikethrough, and their resets | `test_preserve_sgr_attributes_and_resets` |
| Preserve standard, bright, and indexed foreground/background colours | `test_preserve_sgr_standard_colours`; `test_preserve_sgr_bright_colours`; `test_preserve_sgr_256_color`; `test_preserve_sgr_bg_color` |
| Reset foreground and background independently | `test_preserve_sgr_default_colours` |
| Quantise RGB foreground and background to indexed xterm colours | `truecolour input is quantised to the xterm 256-colour palette`; `test_preserve_sgr_truecolour_background` |
| End styled output rows in the default style | `test_preserve_sgr_reset_at_eol` |

## Unicode and bidirectional text

| Contract claim | Direct evidence |
|---|---|
| Accept valid two-, three-, and four-byte UTF-8 | `test_utf8_2byte`; `test_utf8_3byte`; `test_utf8_4byte` |
| Accept UTF-8 split at arbitrary feed boundaries | `test_feed_split_utf8`; `test_feed_chunk_invariance` |
| Discard invalid leaders, stray continuations, and interrupted sequences | `test_invalid_utf8_bytes_discarded`; `test_esc_interrupts_utf8` |
| Discard every Unicode `Bidi_Control`, including across feed boundaries | `test_bidi_controls_discarded`; `test_bidi_control_split_feed` |
| Give zero width to combining and selected format characters | `test_combining_character`; `test_emoji_zwj_not_composed` |
| Give width two to bundled CJK and emoji ranges | `test_cjk_two_columns`; `test_emoji_two_columns` |
| Give East Asian Ambiguous characters width one | `test_east_asian_ambiguous_width_one` |
| Attach zero-width bytes to the preceding cell only while its eight-byte storage permits | `test_combining_character`; `test_zero_width_without_base_discarded`; `test_zero_width_cell_capacity` |
| Do not compose emoji ZWJ or regional-indicator flag sequences | `test_emoji_zwj_not_composed`; `test_flag_sequence_not_composed` |
| Do not apply Arabic joining or presentation-form shaping | `test_arabic_ordering_without_shaping` |
| Apply visual ordering per row and keep combining bytes with their base cell | `test_bidi_hebrew_multiline`; `test_combining_character` |
| Resolve representative LTR, RTL, mixed, number, and mirrored-bracket text | `test_bidi_ltr_unchanged`; `test_bidi_pure_hebrew`; `test_bidi_mixed_english_hebrew`; `test_bidi_hebrew_with_number`; `test_bidi_bracket_mirroring` |
| Conform to the resolver's stated Unicode 13.0 UAX #9 scope | `make test-bidi-conformance` runs 861,940 official cases and exhaustive generated-table checks |

## Ignored behavior and failures

| Contract claim | Direct evidence |
|---|---|
| Ignore private CSI prefixes `?`, `>`, and `!` | `test_private_mode_ignored` |
| Ignore SM/RM and DSR | `test_standard_modes_ignored`; `test_device_status_report_ignored` |
| Ignore alternate-screen, cursor-visibility, origin, insert, and auto-wrap mode changes | `test_alternate_screen_mode_ignored`; `test_private_mode_ignored`; `test_origin_mode_ignored`; `test_insert_mode_ignored`; `test_autowrap_mode_ignored` |
| Ignore character-set designation | `test_character_set_designation_ignored` |
| Ignore mouse protocol mode changes | `test_mouse_protocol_mode_ignored` |
| Consume hyperlink and clipboard OSC payloads | `test_osc_hyperlink_and_clipboard_consumed` |
| Consume image/sixel DCS payloads | `test_sixel_payload_consumed` |
| Ignore unknown ESC and CSI sequences | `test_unknown_esc_ignored`; `test_unknown_csi_ignored` |
| Return 0 on success, 1 on runtime/I/O failure, and 4 on bad arguments | every successful CLI assertion; `reports an unreadable input file as a runtime error`; `reports an output failure as a runtime error`; `invalid dimensions return usage status`; `rejects more than one input file` |

## Claims verified outside behavior tests

Some statements are constraints or provenance, not input/output behavior:

- The caller must choose dimensions matching the source terminal.
- The renderer does not retain timing and receives no resize-event channel.
- The width tables identify their source as Unicode 15.1 and use no locale API.
- Preserved control sequences are unsafe to display blindly, and the renderer is
  not a security boundary.

These remain explicit review points. They are not counted as behavior covered by
an unrelated test.

Run all ordinary contract tests with `make test`; run the upstream bidi corpus
separately with `make test-bidi-conformance`.
