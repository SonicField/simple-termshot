/* Unicode Bidirectional Algorithm (UAX #9), with versioned generated data. */
#include "bidi.h"
#include <stdlib.h>
#include <string.h>
#include "bidi_data.h"

#define MAX_DEPTH 125
#define MAX_BRACKET_DEPTH 63

typedef struct { int level; bidi_type_t override; int isolate; } status_t;
typedef struct { int first, last, level, next, has_previous; } level_run_t;
typedef struct { int open, close; } bracket_pair_t;

static int is_isolate(bidi_type_t t) {
    return t == BIDI_LRI || t == BIDI_RLI || t == BIDI_FSI;
}
static int is_removed(bidi_type_t t) {
    return t == BIDI_RLE || t == BIDI_LRE || t == BIDI_RLO ||
           t == BIDI_LRO || t == BIDI_PDF || t == BIDI_BN;
}
static int is_neutral(bidi_type_t t) {
    return t == BIDI_B || t == BIDI_S || t == BIDI_WS || t == BIDI_ON ||
           is_isolate(t) || t == BIDI_PDI;
}
static bidi_type_t level_dir(int level) { return (level & 1) ? BIDI_R : BIDI_L; }
static bidi_type_t strong_dir(bidi_type_t t) {
    return (t == BIDI_EN || t == BIDI_AN) ? BIDI_R : t;
}
static int next_odd(int level) { return (level & 1) ? level + 2 : level + 1; }
static int next_even(int level) { return (level & 1) ? level + 1 : level + 2; }

bidi_type_t bidi_type(uint32_t cp) {
    int lo = 0, hi = BIDI_RANGES_COUNT - 1;
    while (lo <= hi) {
        int mid = lo + (hi - lo) / 2;
        if (cp < bidi_ranges[mid].lo) hi = mid - 1;
        else if (cp > bidi_ranges[mid].hi) lo = mid + 1;
        else return bidi_ranges[mid].type;
    }
    return BIDI_L;
}

static const struct bidi_bracket_entry *bracket_entry(uint32_t cp) {
    int lo = 0, hi = BIDI_BRACKETS_COUNT - 1;
    while (lo <= hi) {
        int mid = lo + (hi - lo) / 2;
        if (cp < bidi_brackets[mid].codepoint) hi = mid - 1;
        else if (cp > bidi_brackets[mid].codepoint) lo = mid + 1;
        else return &bidi_brackets[mid];
    }
    return NULL;
}

bidi_bracket_type_t bidi_paired_bracket(uint32_t cp, uint32_t *paired_cp) {
    const struct bidi_bracket_entry *entry = bracket_entry(cp);
    if (!entry) {
        if (paired_cp) *paired_cp = cp;
        return BIDI_BRACKET_NONE;
    }
    if (paired_cp) *paired_cp = entry->pair;
    return entry->kind;
}

/* UAX #9 BD16 compares bracket pairs after canonical equivalence. */
static uint32_t canonical_bracket(uint32_t cp) {
    if (cp == 0x2329) return 0x3008;
    if (cp == 0x232A) return 0x3009;
    return cp;
}

uint32_t bidi_mirror(uint32_t cp) {
    int lo = 0, hi = BIDI_MIRRORS_COUNT - 1;
    while (lo <= hi) {
        int mid = lo + (hi - lo) / 2;
        if (cp < bidi_mirrors[mid].codepoint) hi = mid - 1;
        else if (cp > bidi_mirrors[mid].codepoint) lo = mid + 1;
        else return bidi_mirrors[mid].mirror;
    }
    return cp;
}

/* BD9 and P2/P3: ignore text enclosed by isolate initiators. */
static int first_strong(const bidi_type_t *types, int first, int limit) {
    int depth = 0;
    for (int i = first; i < limit; i++) {
        if (is_isolate(types[i])) depth++;
        else if (types[i] == BIDI_PDI) { if (depth > 0) depth--; }
        else if (depth == 0 && types[i] == BIDI_L) return 0;
        else if (depth == 0 && (types[i] == BIDI_R || types[i] == BIDI_AL)) return 1;
    }
    return 0;
}

static void match_isolates(const bidi_type_t *types, int count,
                           int *matching_pdi, int *stack) {
    int depth = 0;
    for (int i = 0; i < count; i++) {
        matching_pdi[i] = -1;
        if (is_isolate(types[i])) {
            stack[depth++] = i;
        } else if (types[i] == BIDI_PDI && depth > 0) {
            matching_pdi[stack[--depth]] = i;
        }
    }
}

static int fsi_direction(const bidi_type_t *types, int count, int pos,
                         const int *matching_pdi) {
    int limit = matching_pdi[pos] >= 0 ? matching_pdi[pos] : count;
    return first_strong(types, pos + 1, limit);
}

/* X1-X8. Original types remain available for X9 and L1. */
static void resolve_explicit(const bidi_type_t *types, bidi_type_t *resolved,
                             int *levels, int count, int para,
                             const int *matching_pdi) {
    status_t stack[MAX_DEPTH + 2] = {{para, BIDI_ON, 0}};
    int top = 0, overflow_isolate = 0, overflow_embedding = 0, valid_isolate = 0;

    for (int i = 0; i < count; i++) {
        bidi_type_t type = types[i], override = BIDI_ON;
        int new_level = -1, isolate = 0;
        levels[i] = stack[top].level;
        resolved[i] = type;

        if (type == BIDI_RLE || type == BIDI_RLO) {
            new_level = next_odd(stack[top].level);
            if (type == BIDI_RLO) override = BIDI_R;
        } else if (type == BIDI_LRE || type == BIDI_LRO) {
            new_level = next_even(stack[top].level);
            if (type == BIDI_LRO) override = BIDI_L;
        } else if (is_isolate(type)) {
            int rtl = type == BIDI_RLI ||
                (type == BIDI_FSI && fsi_direction(types, count, i, matching_pdi));
            new_level = rtl ? next_odd(stack[top].level) : next_even(stack[top].level);
            isolate = 1;
            if (stack[top].override != BIDI_ON) resolved[i] = stack[top].override;
        }

        if (new_level >= 0) {
            if (new_level <= MAX_DEPTH && overflow_isolate == 0 &&
                overflow_embedding == 0) {
                if (isolate) valid_isolate++;
                top++;
                stack[top] = (status_t){new_level, override, isolate};
            } else if (isolate) overflow_isolate++;
            else if (overflow_isolate == 0) overflow_embedding++;
        } else if (type == BIDI_PDI) {
            if (overflow_isolate > 0) overflow_isolate--;
            else if (valid_isolate > 0) {
                overflow_embedding = 0;
                while (top > 0 && !stack[top].isolate) top--;
                if (top > 0) top--;
                valid_isolate--;
            }
            levels[i] = stack[top].level;
            if (stack[top].override != BIDI_ON) resolved[i] = stack[top].override;
        } else if (type == BIDI_PDF) {
            if (overflow_isolate == 0) {
                if (overflow_embedding > 0) overflow_embedding--;
                else if (top > 0 && !stack[top].isolate) top--;
            }
        } else if (type == BIDI_B) {
            top = overflow_isolate = overflow_embedding = valid_isolate = 0;
            levels[i] = para;
        } else if (!is_removed(type) && stack[top].override != BIDI_ON) {
            resolved[i] = stack[top].override;
        }
    }
}

static int prior_active(const int *active, int count, int logical) {
    int result = -1;
    for (int i = 0; i < count && active[i] < logical; i++) result = active[i];
    return result;
}
static int next_active(const int *active, int count, int logical) {
    for (int i = 0; i < count; i++) if (active[i] > logical) return active[i];
    return -1;
}

static int resolve_brackets(const uint32_t *codepoints, bidi_type_t *resolved,
                            const int *levels, const int *seq, int n,
                            bidi_type_t sos) {
    int opens[MAX_BRACKET_DEPTH], open_count = 0;
    bracket_pair_t *pairs = malloc((size_t)n * sizeof(*pairs));
    int pair_count = 0;
    if (!pairs) return 0;

    for (int p = 0; p < n; p++) {
        int logical = seq[p];
        if (resolved[logical] != BIDI_ON) continue;
        const struct bidi_bracket_entry *entry = bracket_entry(codepoints[logical]);
        if (!entry) continue;
        if (entry->kind == BIDI_BRACKET_OPEN) {
            if (open_count < MAX_BRACKET_DEPTH) opens[open_count++] = p;
            else break;
        } else {
            for (int s = open_count - 1; s >= 0; s--) {
                const struct bidi_bracket_entry *opening =
                    bracket_entry(codepoints[seq[opens[s]]]);
                if (opening &&
                    canonical_bracket(opening->pair) ==
                        canonical_bracket(codepoints[logical])) {
                    pairs[pair_count++] = (bracket_pair_t){opens[s], p};
                    open_count = s;
                    break;
                }
            }
        }
    }
    for (int i = 1; i < pair_count; i++) {
        bracket_pair_t value = pairs[i];
        int j = i;
        while (j > 0 && pairs[j - 1].open > value.open) {
            pairs[j] = pairs[j - 1]; j--;
        }
        pairs[j] = value;
    }
    for (int pair = 0; pair < pair_count; pair++) {
        int op = pairs[pair].open, cp = pairs[pair].close;
        int open_logical = seq[op], close_logical = seq[cp];
        bidi_type_t embedding = level_dir(levels[open_logical]);
        bidi_type_t opposite = embedding == BIDI_L ? BIDI_R : BIDI_L;
        int same = 0, other = 0;
        for (int p = op + 1; p < cp; p++) {
            bidi_type_t direction = strong_dir(resolved[seq[p]]);
            if (direction == embedding) same = 1;
            if (direction == opposite) other = 1;
        }
        bidi_type_t result = BIDI_ON;
        if (same) result = embedding;
        else if (other) {
            bidi_type_t preceding = sos;
            for (int p = 0; p < op; p++) {
                bidi_type_t direction = strong_dir(resolved[seq[p]]);
                if (direction == BIDI_L || direction == BIDI_R) preceding = direction;
            }
            result = preceding == opposite ? opposite : embedding;
        }
        if (result != BIDI_ON) {
            resolved[open_logical] = resolved[close_logical] = result;
            for (int p = op + 1; p < n &&
                 bidi_type(codepoints[seq[p]]) == BIDI_NSM; p++)
                resolved[seq[p]] = result;
            for (int p = cp + 1; p < n &&
                 bidi_type(codepoints[seq[p]]) == BIDI_NSM; p++)
                resolved[seq[p]] = result;
        }
    }
    free(pairs);
    return 1;
}

/* W1-W7, N0-N2 and I1-I2 for one isolating run sequence. */
static int resolve_sequence(const uint32_t *codepoints, bidi_type_t *resolved,
                            int *levels, const int *seq, int n,
                            bidi_type_t sos, bidi_type_t eos) {
    bidi_type_t previous = sos;
    for (int p = 0; p < n; p++) {                         /* W1 */
        int i = seq[p];
        if (resolved[i] == BIDI_NSM) resolved[i] = previous;
        previous = (is_isolate(resolved[i]) || resolved[i] == BIDI_PDI)
            ? BIDI_ON : resolved[i];
    }
    previous = sos;
    for (int p = 0; p < n; p++) {                         /* W2 */
        int i = seq[p];
        if (resolved[i] == BIDI_EN && previous == BIDI_AL) resolved[i] = BIDI_AN;
        if (resolved[i] == BIDI_L || resolved[i] == BIDI_R || resolved[i] == BIDI_AL)
            previous = resolved[i];
    }
    for (int p = 0; p < n; p++)                           /* W3 */
        if (resolved[seq[p]] == BIDI_AL) resolved[seq[p]] = BIDI_R;
    for (int p = 1; p + 1 < n; p++) {                     /* W4 */
        int a = seq[p - 1], b = seq[p], c = seq[p + 1];
        if (resolved[b] == BIDI_ES && resolved[a] == BIDI_EN && resolved[c] == BIDI_EN)
            resolved[b] = BIDI_EN;
        else if (resolved[b] == BIDI_CS && resolved[a] == resolved[c] &&
                 (resolved[a] == BIDI_EN || resolved[a] == BIDI_AN))
            resolved[b] = resolved[a];
    }
    for (int p = 0; p < n;) {                             /* W5 */
        if (resolved[seq[p]] != BIDI_ET) { p++; continue; }
        int first = p;
        while (p < n && resolved[seq[p]] == BIDI_ET) p++;
        if ((first > 0 && resolved[seq[first - 1]] == BIDI_EN) ||
            (p < n && resolved[seq[p]] == BIDI_EN))
            for (int q = first; q < p; q++) resolved[seq[q]] = BIDI_EN;
    }
    for (int p = 0; p < n; p++) {                         /* W6 */
        int i = seq[p];
        if (resolved[i] == BIDI_ES || resolved[i] == BIDI_ET || resolved[i] == BIDI_CS)
            resolved[i] = BIDI_ON;
    }
    previous = sos;
    for (int p = 0; p < n; p++) {                         /* W7 */
        int i = seq[p];
        if (resolved[i] == BIDI_EN && previous == BIDI_L) resolved[i] = BIDI_L;
        if (resolved[i] == BIDI_L || resolved[i] == BIDI_R) previous = resolved[i];
    }

    if (!resolve_brackets(codepoints, resolved, levels, seq, n, sos)) /* N0 */
        return 0;

    for (int p = 0; p < n;) {                             /* N1-N2 */
        if (!is_neutral(resolved[seq[p]])) { p++; continue; }
        int first = p;
        while (p < n && is_neutral(resolved[seq[p]])) p++;
        bidi_type_t before = first ? strong_dir(resolved[seq[first - 1]]) : sos;
        bidi_type_t after = p < n ? strong_dir(resolved[seq[p]]) : eos;
        bidi_type_t result = before == after ? before : level_dir(levels[seq[first]]);
        for (int q = first; q < p; q++) resolved[seq[q]] = result;
    }
    for (int p = 0; p < n; p++) {                         /* I1-I2 */
        int i = seq[p];
        if (!(levels[i] & 1)) {
            if (resolved[i] == BIDI_R) levels[i]++;
            else if (resolved[i] == BIDI_AN || resolved[i] == BIDI_EN) levels[i] += 2;
        } else if (resolved[i] == BIDI_L || resolved[i] == BIDI_AN || resolved[i] == BIDI_EN)
            levels[i]++;
    }
    return 1;
}

static int l1_space(bidi_type_t t) {
    return t == BIDI_WS || is_isolate(t) || t == BIDI_PDI || is_removed(t);
}

bidi_status_t bidi_resolve(const uint32_t *codepoints, int count,
                           int *visual_map, int *out_levels, int base_dir,
                           int *out_para, int *out_visual_count) {
    if (count < 0 || base_dir < 0 || base_dir > 2 || !out_para ||
        !out_visual_count || (count && (!codepoints || !visual_map || !out_levels)))
        return BIDI_ERROR_INVALID_ARGUMENT;
    if ((size_t)count > SIZE_MAX / sizeof(level_run_t))
        return BIDI_ERROR_NO_MEMORY;
    for (int i = 0; i < count; i++) {
        if (codepoints[i] > 0x10FFFF ||
            (codepoints[i] >= 0xD800 && codepoints[i] <= 0xDFFF))
            return BIDI_ERROR_INVALID_ARGUMENT;
    }
    int para = base_dir == 2 ? 1 : 0;
    if (!count) { *out_para = para; *out_visual_count = 0; return BIDI_SUCCESS; }

    bidi_type_t *types = malloc((size_t)count * sizeof(*types));
    bidi_type_t *resolved = malloc((size_t)count * sizeof(*resolved));
    int *levels = malloc((size_t)count * sizeof(*levels));
    int *matching = malloc((size_t)count * sizeof(*matching));
    int *isolate_stack = malloc((size_t)count * sizeof(*isolate_stack));
    int *active = malloc((size_t)count * sizeof(*active));
    int *run_of = malloc((size_t)count * sizeof(*run_of));
    level_run_t *runs = malloc((size_t)count * sizeof(*runs));
    int *sequence = malloc((size_t)count * sizeof(*sequence));
    int *map = malloc((size_t)count * sizeof(*map));
    if (!types || !resolved || !levels || !matching || !isolate_stack ||
        !active || !run_of ||
        !runs || !sequence || !map) {
        free(types); free(resolved); free(levels); free(matching);
        free(isolate_stack); free(active);
        free(run_of); free(runs); free(sequence); free(map);
        return BIDI_ERROR_NO_MEMORY;
    }
    for (int i = 0; i < count; i++) types[i] = bidi_type(codepoints[i]);
    match_isolates(types, count, matching, isolate_stack);
    if (base_dir == 0) para = first_strong(types, 0, count);
    resolve_explicit(types, resolved, levels, count, para, matching);

    int active_count = 0;
    for (int i = 0; i < count; i++) {
        run_of[i] = -1;
        if (is_removed(types[i])) levels[i] = -1;
        else active[active_count++] = i;
    }
    int run_count = 0;
    for (int p = 0; p < active_count;) {
        int first = p, level = levels[active[p]];
        while (p < active_count && levels[active[p]] == level) p++;
        runs[run_count] = (level_run_t){first, p - 1, level, -1, 0};
        for (int q = first; q < p; q++) run_of[active[q]] = run_count;
        run_count++;
    }
    for (int run = 0; run < run_count; run++) {
        int last = active[runs[run].last];
        if (is_isolate(types[last]) && matching[last] >= 0) {
            int next = run_of[matching[last]];
            if (next >= 0) { runs[run].next = next; runs[next].has_previous = 1; }
        }
    }
    for (int root = 0; root < run_count; root++) {
        if (runs[root].has_previous) continue;
        int n = 0, run = root, final_run = root;
        while (run >= 0) {
            for (int p = runs[run].first; p <= runs[run].last; p++) sequence[n++] = active[p];
            final_run = run; run = runs[run].next;
        }
        int previous = prior_active(active, active_count, sequence[0]);
        int following = is_isolate(types[sequence[n - 1]]) ? -1 :
            next_active(active, active_count, sequence[n - 1]);
        int sos_level = runs[root].level, eos_level = runs[final_run].level;
        int adjacent = previous >= 0 ? runs[run_of[previous]].level : para;
        if (adjacent > sos_level) sos_level = adjacent;
        adjacent = following >= 0 ? runs[run_of[following]].level : para;
        if (adjacent > eos_level) eos_level = adjacent;
        if (!resolve_sequence(codepoints, resolved, levels, sequence, n,
                              level_dir(sos_level), level_dir(eos_level))) {
            free(types); free(resolved); free(levels); free(matching);
            free(isolate_stack); free(active);
            free(run_of); free(runs); free(sequence); free(map);
            return BIDI_ERROR_NO_MEMORY;
        }
    }
    for (int i = 0; i < count; i++) {                     /* L1 */
        if (types[i] == BIDI_B || types[i] == BIDI_S) {
            levels[i] = para;
            for (int j = i - 1; j >= 0 && l1_space(types[j]); j--) levels[j] = para;
        }
    }
    for (int i = count - 1; i >= 0 && l1_space(types[i]); i--) levels[i] = para;
    for (int i = 0; i < count; i++)
        if (is_removed(types[i])) levels[i] = -1;

    int visual_count = 0, maximum = para;                  /* L2 */
    for (int p = 0; p < active_count; p++) {
        int i = active[p]; map[visual_count++] = i;
        if (levels[i] > maximum) maximum = levels[i];
    }
    for (int level = maximum; level >= 1; level--) {
        for (int p = 0; p < visual_count;) {
            while (p < visual_count && levels[map[p]] < level) p++;
            int first = p;
            while (p < visual_count && levels[map[p]] >= level) p++;
            for (int left = first, right = p - 1; left < right; left++, right--) {
                int tmp = map[left]; map[left] = map[right]; map[right] = tmp;
            }
        }
    }
    memcpy(visual_map, map, (size_t)visual_count * sizeof(*visual_map));
    memcpy(out_levels, levels, (size_t)count * sizeof(*out_levels));
    *out_para = para; *out_visual_count = visual_count;
    free(types); free(resolved); free(levels); free(matching);
    free(isolate_stack); free(active);
    free(run_of); free(runs); free(sequence); free(map);
    return BIDI_SUCCESS;
}

int bidi_reorder_with_levels(const uint32_t *codepoints, int count,
                             int *visual_map, int *out_levels, int base_dir) {
    int para = 0, visual_count = 0;
    bidi_status_t result = bidi_resolve(codepoints, count, visual_map, out_levels,
                                        base_dir, &para, &visual_count);
    (void)visual_count;
    return result == BIDI_SUCCESS ? para : -1;
}

int bidi_reorder(const uint32_t *codepoints, int count, int *visual_map,
                 int base_dir) {
    if (count <= 0) return count == 0 ? 0 : -1;
    int *levels = malloc((size_t)count * sizeof(*levels));
    if (!levels) return -1;
    int result = bidi_reorder_with_levels(codepoints, count, visual_map, levels, base_dir);
    free(levels);
    return result;
}
