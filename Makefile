CC ?= cc
PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin
DESTDIR ?=

CPPFLAGS ?= -Isrc
BASE_CFLAGS = -Wall -Wextra -Wshadow -Werror -std=c11 \
	-D_POSIX_C_SOURCE=200809L -D_DEFAULT_SOURCE
CFLAGS ?= -O2
ALL_CFLAGS = $(BASE_CFLAGS) $(CFLAGS)
ANALYZER_CC ?= gcc
ANALYZER_CFLAGS ?= -O0 -g -fanalyzer

TARGET = term-snapshot
BUILD_DIR = build
TEST_TARGET = $(BUILD_DIR)/test_term_snapshot
ANALYZE_TARGET = $(BUILD_DIR)/term-snapshot-analyze

CORE_SOURCES = src/term_snapshot.c src/unicode_width.c src/bidi.c \
	src/term_style.c
SOURCES = src/main.c $(CORE_SOURCES)

.PHONY: all clean install test debug sanitize analyze

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CC) $(CPPFLAGS) $(ALL_CFLAGS) -o $@ $(SOURCES)

$(BUILD_DIR):
	mkdir -p $@

$(TEST_TARGET): tests/test_term_snapshot.c $(CORE_SOURCES) | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(ALL_CFLAGS) -o $@ $^

test: $(TEST_TARGET) $(TARGET)
	./$(TEST_TARGET)
	./tests/test_cli.sh

install: $(TARGET)
	install -d "$(DESTDIR)$(BINDIR)"
	install -m 0755 $(TARGET) "$(DESTDIR)$(BINDIR)/$(TARGET)"

debug:
	$(MAKE) clean
	$(MAKE) CFLAGS='-O0 -g3' all

sanitize:
	$(MAKE) clean
	$(MAKE) CFLAGS='-O1 -g3 -fsanitize=address,undefined -fno-omit-frame-pointer' test

analyze: $(ANALYZE_TARGET)

$(ANALYZE_TARGET): $(SOURCES) | $(BUILD_DIR)
	$(ANALYZER_CC) $(CPPFLAGS) $(BASE_CFLAGS) $(ANALYZER_CFLAGS) -o $@ $(SOURCES)

clean:
	rm -f $(TARGET) $(TEST_TARGET) $(ANALYZE_TARGET)
	rmdir $(BUILD_DIR) 2>/dev/null || true
