#!/bin/sh
set -eu

PROGRAM=${PROGRAM:-./simple-termshot}
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

tmp_dir=$(mktemp -d "${TMPDIR:-/tmp}/simple-termshot-test.XXXXXX")
trap 'rm -rf "$tmp_dir"' EXIT HUP INT TERM

help=$($PROGRAM --help)
case "$help" in
    *"simple-termshot — Virtual terminal renderer"*) ;;
    *) fail '--help identifies the program' ;;
esac
TESTS_RUN=$((TESTS_RUN + 1))
case "$help" in
    *"documented"*"subset of terminal control sequences"*) ;;
    *) fail '--help omitted the renderer compatibility boundary' ;;
esac
TESTS_RUN=$((TESTS_RUN + 1))
case "$help" in
    *"SGR is stripped by default"*) ;;
    *) fail '--help did not distinguish default and preserved SGR output' ;;
esac
TESTS_RUN=$((TESTS_RUN + 1))
case "$help" in
    *"Unicode 15.1 cell widths"*"Unicode 13.0 UAX #9 data"*) ;;
    *) fail '--help omitted the Unicode data versions' ;;
esac
TESTS_RUN=$((TESTS_RUN + 1))
case "$help" in
    *"Bidi_Control input characters are discarded"*) ;;
    *) fail '--help omitted the bidi-control input policy' ;;
esac
TESTS_RUN=$((TESTS_RUN + 1))

actual=$($PROGRAM --version)
assert_eq 'simple-termshot 0.1.0' "$actual" '--version reports program version'

actual=$(printf 'Progress 10%%\rProgress 100%%\n' | \
    $PROGRAM --width=24 --height=4)
assert_eq 'Progress 100%' "$actual" 'carriage-return progress update'

actual=$(printf 'old\033[2J\033[Hnew 世界\n' | \
    $PROGRAM --width 24 --height 4)
assert_eq 'new 世界' "$actual" 'clear, home, and Unicode rendering'

printf 'before\rafter\n' >"$tmp_dir/capture.log"
actual=$($PROGRAM --width=12 --height=2 "$tmp_dir/capture.log")
assert_eq 'aftere' "$actual" 'reads a capture directly from a file'

actual=$(printf 'stdin\n' | $PROGRAM --width=12 --height=2 -)
assert_eq 'stdin' "$actual" 'explicit dash reads standard input'

actual=$(printf 'A\342\200\256B\n' | $PROGRAM --width=12 --height=2)
assert_eq 'AB' "$actual" 'Unicode bidi controls are discarded'

printf '\033[38;5;1mred\033[0m\n' | \
    $PROGRAM --width=12 --height=2 --no-strip >"$tmp_dir/sgr.out"
actual=$(od -An -tx1 -v "$tmp_dir/sgr.out" | tr -d ' \n')
assert_eq '1b5b33383b353b316d7265641b5b306d0a' "$actual" \
    'SGR preservation'

printf '\033[1mbold\033[0m\n' | \
    $PROGRAM --width=12 --height=2 --preserve-sgr >"$tmp_dir/preserve.out"
actual=$(od -An -tx1 -v "$tmp_dir/preserve.out" | tr -d ' \n')
assert_eq '1b5b316d626f6c641b5b306d0a' "$actual" \
    'descriptive SGR preservation option'

printf '\033[38;2;255;0;0mred\033[0m\n' | \
    $PROGRAM --width=12 --height=2 --preserve-sgr >"$tmp_dir/truecolor.out"
actual=$(od -An -tx1 -v "$tmp_dir/truecolor.out" | tr -d ' \n')
assert_eq '1b5b33383b353b3139366d7265641b5b306d0a' "$actual" \
    'truecolour input is quantised to the xterm 256-colour palette'

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

set +e
$PROGRAM "$tmp_dir/capture.log" "$tmp_dir/capture.log" \
    >"$tmp_dir/two-files.out" 2>"$tmp_dir/two-files.err"
status=$?
set -e
assert_eq '4' "$status" 'rejects more than one input file'

set +e
$PROGRAM "$tmp_dir/missing.log" \
    >"$tmp_dir/missing.out" 2>"$tmp_dir/missing.err"
status=$?
set -e
assert_eq '1' "$status" 'reports an unreadable input file as a runtime error'

printf 'CLI tests: %d/%d passed\n' "$TESTS_RUN" "$TESTS_RUN"
