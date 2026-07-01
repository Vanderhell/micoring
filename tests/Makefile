CC ?= cc
CPPFLAGS ?=
CFLAGS ?=
LDFLAGS ?=

TEST_CPPFLAGS = $(CPPFLAGS) -I../include -DMRING_TEST=1
TEST_CFLAGS = $(CFLAGS) -std=c99 -Wall -Wextra -Wpedantic -Werror
TEST_LDFLAGS = $(LDFLAGS)

.PHONY: all clean

all: test_all test_harness_selftest

test_all: test_all.c ../src/mring.c ../include/mring.h ../include/mring_config.h
	$(CC) $(TEST_CPPFLAGS) $(TEST_CFLAGS) ../src/mring.c test_all.c $(TEST_LDFLAGS) -o $@

test_harness_selftest: test_harness_selftest.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -std=c99 -Wall -Wextra -Wpedantic -Werror $(LDFLAGS) test_harness_selftest.c -o $@

clean:
	rm -f test_all test_all.exe test_harness_selftest test_harness_selftest.exe
