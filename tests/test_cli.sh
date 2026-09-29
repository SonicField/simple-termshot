#!/bin/sh
set -eu

PROGRAM=${PROGRAM:-./term-snapshot}
TESTS_RUN=0

fail() {
    printf 'FAIL: %s\n' "$1" >&2
    exit 1
}

assert_eq() {
    TESTS_RUN=$((TESTS_RUN + 1))
    if [ "$1" != "$2" ]; then
        printf 'FAIL: %s\nexpected: [%s]\nactual:   [%s]\n' \
            "$3" "$1" "$2" >&2
        exit 1
    fi
}

tmp_dir=$(mktemp -d "${TMPDIR:-/tmp}/term-snapshot-test.XXXXXX")
trap 'rm -rf "$tmp_dir"' EXIT HUP INT TERM

help=$($PROGRAM --help)
case "$help" in
    *"term-snapshot — Virtual terminal renderer"*) ;;
    *) fail '--help identifies the program' ;;
esac
TESTS_RUN=$((TESTS_RUN + 1))

actual=$(printf 'Progress 10%%\rProgress 100%%\n' | \
    $PROGRAM --width=24 --height=4)
assert_eq 'Progress 100%' "$actual" 'carriage-return progress update'

actual=$(printf 'old\033[2J\033[Hnew 世界\n' | \
    $PROGRAM --width 24 --height 4)
assert_eq 'new 世界' "$actual" 'clear, home, and Unicode rendering'

printf '\033[38;5;1mred\033[0m\n' | \
    $PROGRAM --width=12 --height=2 --no-strip >"$tmp_dir/sgr.out"
actual=$(od -An -tx1 -v "$tmp_dir/sgr.out" | tr -d ' \n')
assert_eq '1b5b33383b353b316d7265641b5b306d0a' "$actual" \
    'SGR preservation'

set +e
$PROGRAM --width=0 </dev/null >"$tmp_dir/bad.out" 2>"$tmp_dir/bad.err"
status=$?
set -e
assert_eq '4' "$status" 'invalid dimensions return usage status'
case "$(cat "$tmp_dir/bad.err")" in
    *"must be 1-10000"*) ;;
    *) fail 'invalid dimensions explain the accepted range' ;;
esac
TESTS_RUN=$((TESTS_RUN + 1))

printf 'CLI tests: %d/%d passed\n' "$TESTS_RUN" "$TESTS_RUN"
