# Extraction record

This repository was extracted from `SonicField/nbs-framework` at source commit
`a1084a3aaab1d933585438b2061ec8c86150f1c2` on 2026-09-29.

## Preserved source

The history was filtered to these paths before the standalone reorganisation:

```text
src/nbs-ts-render/
src/nbs-common/nbs_assert.h
src/nbs-common/nbs_term_attr.c
src/nbs-common/nbs_term_attr.h
```

The export mapped the source repository's `master` branch to `main`:

```sh
git fast-export \
  --refspec=refs/heads/master:refs/heads/main \
  master -- \
  src/nbs-ts-render \
  src/nbs-common/nbs_assert.h \
  src/nbs-common/nbs_term_attr.c \
  src/nbs-common/nbs_term_attr.h \
| git fast-import
```

This retained nine relevant historical commits. The first standalone commit
then moved the program into conventional `src/` and `tests/` directories,
renamed NBS-specific symbols, and reduced the shared style module to the code
the renderer actually uses.

## Verification evidence

Before extraction, at the source commit:

- `make -C src/nbs-ts-render clean test` passed 102/102 cases.
- Black-box checks covered carriage-return rewriting, clearing and homing,
  Unicode output, SGR preservation, and invalid dimensions.
- The source worktree was clean.

After path filtering, the unmodified nested export again passed 102/102 cases.
After reorganisation, the same 102 cases passed before new behavior was added.

The standalone repository now verifies:

- 106 core cases, including feed-chunk invariance and dimension boundaries.
- 18 black-box CLI assertions against the built executable.
- GCC with `-Wall -Wextra -Wshadow -Werror`.
- AddressSanitizer and UndefinedBehaviorSanitizer.
- GCC `-fanalyzer` and ShellCheck.
- GCC and Clang on Linux, plus Apple Clang on macOS, through CI.

The local extraction host did not have Clang installed. Consequently the
Clang and macOS claims depend on the checked-in CI jobs and should only be
considered confirmed after those jobs pass in the public repository.

## Deliberate boundaries

`simple-termshot` is not linked to NBS and has no NBS runtime or build dependency.
It does not attempt to be a full xterm, retain scrollback, or replay a session
over time. Its contract is narrower: consume a captured byte stream and return
the final visible fixed-size screen.

The original repository was not modified by this extraction.
