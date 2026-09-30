/* Run the official Unicode BidiTest and BidiCharacterTest corpora. */
#include "bidi.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_ITEMS 256
#define LINE_SIZE 8192
#define MAX_FAILURES 20

static long cases_run;
static int failures;

static char *trim(char *text) {
    while (isspace((unsigned char)*text)) text++;
    char *end = text + strlen(text);
    while (end > text && isspace((unsigned char)end[-1])) end--;
    *end = '\0';
    return text;
}

static int parse_numbers(char *text, int base, int allow_x,
                         int *values, int *count) {
    int n = 0;
    char *save = NULL;
    for (char *token = strtok_r(text, " \t", &save); token;
         token = strtok_r(NULL, " \t", &save)) {
        if (n >= MAX_ITEMS) return 0;
        if (allow_x && strcmp(token, "x") == 0) values[n++] = -1;
        else {
            char *end;
            errno = 0;
            long value = strtol(token, &end, base);
            if (errno || *end || value < 0 || value > 0x10FFFF) return 0;
            values[n++] = (int)value;
        }
    }
    *count = n;
    return 1;
}

static uint32_t representative(const char *name) {
    static const struct { const char *name; uint32_t cp; } values[] = {
        {"L",0x0041},{"R",0x05D0},{"AL",0x0627},{"EN",0x0030},
        {"ES",0x002B},{"ET",0x0024},{"AN",0x0660},{"CS",0x002C},
        {"NSM",0x0300},{"BN",0x00AD},{"B",0x2029},{"S",0x0009},
        {"WS",0x0020},{"ON",0x0021},{"LRE",0x202A},{"RLE",0x202B},
        {"LRO",0x202D},{"RLO",0x202E},{"PDF",0x202C},{"LRI",0x2066},
        {"RLI",0x2067},{"FSI",0x2068},{"PDI",0x2069},
    };
    for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); i++)
        if (strcmp(name, values[i].name) == 0) return values[i].cp;
    return UINT32_MAX;
}

static int parse_types(char *text, uint32_t *codepoints, int *count) {
    int n = 0;
    char *save = NULL;
    for (char *token = strtok_r(text, " \t", &save); token;
         token = strtok_r(NULL, " \t", &save)) {
        if (n >= MAX_ITEMS) return 0;
        uint32_t cp = representative(token);
        if (cp == UINT32_MAX) return 0;
        codepoints[n++] = cp;
    }
    *count = n;
    return 1;
}

static void report_mismatch(const char *file, long line_number,
                            const char *field, int position,
                            int expected, int actual) {
    if (failures < MAX_FAILURES)
        fprintf(stderr, "%s:%ld: %s[%d]: expected %d, got %d\n",
                file, line_number, field, position, expected, actual);
    failures++;
}

static void check_case(const char *file, long line_number,
                       const uint32_t *codepoints, int count, int base_dir,
                       int expected_para, const int *expected_levels,
                       int level_count, const int *expected_map, int map_count) {
    int levels[MAX_ITEMS], map[MAX_ITEMS], para = -1, visual_count = -1;
    bidi_status_t status = bidi_resolve(codepoints, count, map, levels,
                                        base_dir, &para, &visual_count);
    cases_run++;
    if (status != BIDI_SUCCESS) {
        report_mismatch(file, line_number, "status", 0, BIDI_SUCCESS, status);
        return;
    }
    if (expected_para >= 0 && para != expected_para)
        report_mismatch(file, line_number, "paragraph", 0, expected_para, para);
    if (level_count != count) {
        report_mismatch(file, line_number, "level-count", 0, count, level_count);
        return;
    }
    for (int i = 0; i < count; i++)
        if (levels[i] != expected_levels[i])
            report_mismatch(file, line_number, "level", i,
                            expected_levels[i], levels[i]);
    if (visual_count != map_count) {
        report_mismatch(file, line_number, "map-count", 0, map_count, visual_count);
        return;
    }
    for (int i = 0; i < map_count; i++)
        if (map[i] != expected_map[i])
            report_mismatch(file, line_number, "map", i, expected_map[i], map[i]);
}

static int run_bidi_test(const char *path) {
    FILE *file = fopen(path, "r");
    if (!file) { perror(path); return 0; }
    char line[LINE_SIZE];
    int expected_levels[MAX_ITEMS], expected_map[MAX_ITEMS];
    int level_count = 0, map_count = 0;
    long line_number = 0;
    while (fgets(line, sizeof(line), file)) {
        line_number++;
        if (!strchr(line, '\n') && !feof(file)) {
            fprintf(stderr, "%s:%ld: line exceeds buffer\n", path, line_number);
            fclose(file); return 0;
        }
        char *comment = strchr(line, '#');
        if (comment) *comment = '\0';
        char *body = trim(line);
        if (!*body) continue;
        if (strncmp(body, "@Levels:", 8) == 0) {
            if (!parse_numbers(trim(body + 8), 10, 1,
                               expected_levels, &level_count)) goto malformed;
            continue;
        }
        if (strncmp(body, "@Reorder:", 9) == 0) {
            if (!parse_numbers(trim(body + 9), 10, 0,
                               expected_map, &map_count)) goto malformed;
            continue;
        }
        if (*body == '@') continue;
        char *semicolon = strchr(body, ';');
        if (!semicolon) goto malformed;
        *semicolon++ = '\0';
        uint32_t codepoints[MAX_ITEMS];
        int count;
        if (!parse_types(trim(body), codepoints, &count)) goto malformed;
        char *end;
        long modes = strtol(trim(semicolon), &end, 16);
        if (*trim(end) || modes < 0 || modes > 7) goto malformed;
        if (modes & 1) check_case(path, line_number, codepoints, count, 0, -1,
                                  expected_levels, level_count, expected_map, map_count);
        if (modes & 2) check_case(path, line_number, codepoints, count, 1, 0,
                                  expected_levels, level_count, expected_map, map_count);
        if (modes & 4) check_case(path, line_number, codepoints, count, 2, 1,
                                  expected_levels, level_count, expected_map, map_count);
        continue;
malformed:
        fprintf(stderr, "%s:%ld: malformed test data\n", path, line_number);
        fclose(file); return 0;
    }
    if (ferror(file)) { perror(path); fclose(file); return 0; }
    fclose(file);
    return 1;
}

static int run_character_test(const char *path) {
    FILE *file = fopen(path, "r");
    if (!file) { perror(path); return 0; }
    char line[LINE_SIZE];
    long line_number = 0;
    while (fgets(line, sizeof(line), file)) {
        line_number++;
        char *comment = strchr(line, '#');
        if (comment) *comment = '\0';
        char *fields[5], *body = trim(line);
        if (!*body) continue;
        for (int i = 0; i < 5; i++) {
            fields[i] = body;
            if (i < 4) {
                char *separator = strchr(body, ';');
                if (!separator) goto malformed;
                *separator = '\0'; body = separator + 1;
            }
            fields[i] = trim(fields[i]);
        }
        int cps[MAX_ITEMS], levels[MAX_ITEMS], map[MAX_ITEMS];
        int count, level_count, map_count;
        if (!parse_numbers(fields[0], 16, 0, cps, &count) ||
            !parse_numbers(fields[3], 10, 1, levels, &level_count) ||
            !parse_numbers(fields[4], 10, 0, map, &map_count)) goto malformed;
        uint32_t codepoints[MAX_ITEMS];
        for (int i = 0; i < count; i++) codepoints[i] = (uint32_t)cps[i];
        char *end1, *end2;
        long direction = strtol(fields[1], &end1, 10);
        long para = strtol(fields[2], &end2, 10);
        if (*trim(end1) || *trim(end2) || direction < 0 || direction > 2 ||
            para < 0 || para > 1) goto malformed;
        int base_dir = direction == 0 ? 1 : direction == 1 ? 2 : 0;
        check_case(path, line_number, codepoints, count, base_dir, (int)para,
                   levels, level_count, map, map_count);
        continue;
malformed:
        fprintf(stderr, "%s:%ld: malformed test data\n", path, line_number);
        fclose(file); return 0;
    }
    if (ferror(file)) { perror(path); fclose(file); return 0; }
    fclose(file);
    return 1;
}

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "usage: %s BidiTest.txt BidiCharacterTest.txt\n", argv[0]);
        return 2;
    }
    if (!run_bidi_test(argv[1]) || !run_character_test(argv[2])) return 2;
    if (failures) {
        fprintf(stderr, "bidi conformance: %d failures across %ld cases\n",
                failures, cases_run);
        return 1;
    }
    printf("bidi conformance: %ld cases passed\n", cases_run);
    return 0;
}
