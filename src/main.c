/*
 * main.c — simple-termshot CLI entry point.
 *
 * Reads raw PTY output from a file or stdin, processes it through the terminal
 * emulator, and outputs the final screen state as plain text on stdout.
 *
 * Usage: simple-termshot [--width=N] [--height=N] [FILE]
 *        cat output.log | simple-termshot
 */

#include "term_snapshot.h"
#include "ts_assert.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define SIMPLE_TERMSHOT_VERSION "0.1.0"

/* Exit code for invalid arguments. */
#define TS_EXIT_BAD_ARGS 4

/* Read buffer size: 64KB */
#define READ_BUF_SIZE (64 * 1024)

static void print_help(void) {
    printf(
        "simple-termshot — Virtual terminal renderer\n"
        "\n"
        "Reads raw PTY output from a file or stdin, processes it through a\n"
        "terminal emulator (cursor movement, scrolling, erase), strips\n"
        "all decoration (color, bold, italic, underline), and outputs\n"
        "the final screen state as plain UTF-8 text.\n"
        "\n"
        "USAGE:\n"
        "    simple-termshot [OPTIONS] [FILE]\n"
        "    cat output.log | simple-termshot\n"
        "    simple-termshot < output.log\n"
        "\n"
        "OPTIONS:\n"
        "    --width=N, --width N\n"
        "                  Set screen width in columns (default: %d)\n"
        "    --height=N, --height N\n"
        "                  Set screen height in rows (default: %d)\n"
        "    --preserve-sgr\n"
        "                  Preserve SGR colour/style escape sequences in output\n"
        "    --no-strip    Alias for --preserve-sgr\n"
        "    -h, --help    Show this help message and exit\n"
        "    -V, --version Show the program version and exit\n"
        "\n"
        "DESCRIPTION:\n"
        "    simple-termshot acts as a headless terminal emulator. It maintains\n"
        "    an internal screen buffer and processes escape sequences exactly\n"
        "    as a real terminal would. The output is what a human would see\n"
        "    on screen after all input has been processed.\n"
        "\n"
        "    The default dimensions are %dx%d. Use --width and --height to\n"
        "    match the terminal size that produced the captured output.\n"
        "\n"
        "SUPPORTED ESCAPE SEQUENCES:\n"
        "    Cursor movement:  CUP, CUU, CUD, CUF, CUB, CNL, CPL, CHA,\n"
        "                      VPA, HVP\n"
        "    Erase:            ED (erase display), EL (erase line),\n"
        "                      ECH (erase characters)\n"
        "    Scroll:           SU (scroll up), SD (scroll down),\n"
        "                      DECSTBM (scroll region)\n"
        "    Insert/Delete:    IL, DL, ICH, DCH\n"
        "    Tabs:             HT, HTS, TBC\n"
        "    Cursor save:      DECSC (ESC 7), DECRC (ESC 8)\n"
        "    Line control:     LF, CR, BS, IND, NEL, RI\n"
        "    Reset:            RIS (ESC c)\n"
        "    UTF-8:            Full multi-byte character support\n"
        "\n"
        "    All SGR (color/style) sequences are silently stripped.\n"
        "    OSC and DCS sequences are silently consumed.\n"
        "\n"
        "EXAMPLES:\n"
        "    # Render a captured terminal session:\n"
        "    simple-termshot output.log\n"
        "\n"
        "    # Render with custom terminal size:\n"
        "    cat output.log | simple-termshot --width=120 --height=40\n"
        "\n"
        "EXIT CODES:\n"
        "    0    Success\n"
        "    1    Runtime error (allocation failure, I/O error)\n"
        "    4    Bad arguments (invalid or missing option values)\n",
        TS_RENDER_DEFAULT_COLS, TS_RENDER_DEFAULT_ROWS,
        TS_RENDER_DEFAULT_COLS, TS_RENDER_DEFAULT_ROWS
    );
}

static int parse_int_arg(const char *arg, const char *prefix, int *out) {
    size_t plen = strlen(prefix);
    if (strncmp(arg, prefix, plen) != 0) return 0;
    const char *val = arg + plen;
    if (*val == '\0') {
        fprintf(stderr, "simple-termshot: missing value for %s\n", prefix);
        return -1;
    }
    char *end;
    errno = 0;
    long v = strtol(val, &end, 10);
    if (*end != '\0' || errno != 0 || v <= 0 ||
        v > TS_RENDER_MAX_DIMENSION) {
        fprintf(stderr,
                "simple-termshot: invalid value '%s' for %s (must be 1-%d)\n",
                val, prefix, TS_RENDER_MAX_DIMENSION);
        return -1;
    }
    *out = (int)v;
    return 1;
}

int main(int argc, char *argv[]) {
    int width = TS_RENDER_DEFAULT_COLS;
    int height = TS_RENDER_DEFAULT_ROWS;
    int preserve_sgr = 0;
    int options_done = 0;
    const char *input_path = NULL;

    for (int i = 1; i < argc; i++) {
        if (!options_done &&
            (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0)) {
            print_help();
            return 0;
        }
        if (!options_done &&
            (strcmp(argv[i], "--version") == 0 || strcmp(argv[i], "-V") == 0)) {
            printf("simple-termshot %s\n", SIMPLE_TERMSHOT_VERSION);
            return 0;
        }
        if (!options_done && strcmp(argv[i], "--") == 0) {
            options_done = 1;
            continue;
        }

        int result;
        result = options_done ? 0 : parse_int_arg(argv[i], "--width=", &width);
        if (result == -1) return TS_EXIT_BAD_ARGS;
        if (result == 1) continue;

        result = options_done ? 0 : parse_int_arg(argv[i], "--height=", &height);
        if (result == -1) return TS_EXIT_BAD_ARGS;
        if (result == 1) continue;

        if (!options_done &&
            (strcmp(argv[i], "--preserve-sgr") == 0 ||
             strcmp(argv[i], "--no-strip") == 0)) {
            preserve_sgr = 1;
            continue;
        }

        /* Support space-separated form: --width N / --height N */
        if (!options_done && strcmp(argv[i], "--width") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "simple-termshot: --width requires a value\n");
                return TS_EXIT_BAD_ARGS;
            }
            char prefixed[64];
            snprintf(prefixed, sizeof(prefixed), "--width=%s", argv[++i]);
            result = parse_int_arg(prefixed, "--width=", &width);
            if (result == -1) return TS_EXIT_BAD_ARGS;
            continue;
        }

        if (!options_done && strcmp(argv[i], "--height") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "simple-termshot: --height requires a value\n");
                return TS_EXIT_BAD_ARGS;
            }
            char prefixed[64];
            snprintf(prefixed, sizeof(prefixed), "--height=%s", argv[++i]);
            result = parse_int_arg(prefixed, "--height=", &height);
            if (result == -1) return TS_EXIT_BAD_ARGS;
            continue;
        }

        if (!options_done && argv[i][0] == '-' && strcmp(argv[i], "-") != 0) {
            fprintf(stderr, "simple-termshot: unknown option '%s'\n"
                            "Try 'simple-termshot --help' for usage.\n", argv[i]);
            return TS_EXIT_BAD_ARGS;
        }
        if (input_path != NULL) {
            fprintf(stderr, "simple-termshot: only one input file may be specified\n");
            return TS_EXIT_BAD_ARGS;
        }
        input_path = argv[i];
    }

    FILE *input = stdin;
    if (input_path != NULL && strcmp(input_path, "-") != 0) {
        input = fopen(input_path, "rb");
        if (!input) {
            fprintf(stderr, "simple-termshot: cannot open '%s': %s\n",
                    input_path, strerror(errno));
            return 1;
        }
    }

    ts_render_t *t = ts_render_create(height, width);
    if (!t) {
        fprintf(stderr, "simple-termshot: failed to allocate terminal buffer (%dx%d)\n",
                width, height);
        if (input != stdin) fclose(input);
        return 1;
    }

    if (preserve_sgr) {
        ts_render_set_preserve_sgr(t, 1);
    }

    /* Read stdin in 64KB chunks and feed to emulator */
    char *buf = malloc(READ_BUF_SIZE);
    if (!buf) {
        fprintf(stderr, "simple-termshot: failed to allocate read buffer\n");
        ts_render_destroy(t);
        if (input != stdin) fclose(input);
        return 1;
    }

    size_t n;
    while ((n = fread(buf, 1, READ_BUF_SIZE, input)) > 0) {
        ts_render_feed(t, buf, n);
    }

    free(buf);

    if (ferror(input)) {
        fprintf(stderr, "simple-termshot: read error on %s: %s\n",
                input_path != NULL ? input_path : "stdin", strerror(errno));
        if (input != stdin) fclose(input);
        ts_render_destroy(t);
        return 1;
    }
    if (input != stdin && fclose(input) != 0) {
        fprintf(stderr, "simple-termshot: failed to close '%s': %s\n",
                input_path, strerror(errno));
        ts_render_destroy(t);
        return 1;
    }

    /* Output final screen state */
    char *output = ts_render_snapshot(t);
    if (!output) {
        fprintf(stderr, "simple-termshot: failed to allocate snapshot buffer\n");
        ts_render_destroy(t);
        return 1;
    }

    fputs(output, stdout);

    free(output);
    ts_render_destroy(t);
    return 0;
}
