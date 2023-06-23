# Copyright (c) 2011 The LevelDB Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file. See the AUTHORS file for names of contributors.

# On MacOS, after running command 'brew install googletest' to install libraries.
GOOGLETEST_DIR=/opt/homebrew/Cellar/googletest/1.13.0

CC = g++

CFLAGS = -c -Wall -I. -I$(GOOGLETEST_DIR)/include -std=c++14 -stdlib=libc++

LDFLAGS=-L$(GOOGLETEST_DIR)/lib -lpthread -lgtest -lgtest_main

LIBOBJECTS = \
		./db/memtable.o\
		./db/skiplist.o\
		./util/arena.o

TESTS = \
		arena_test

PROGRAMS = leveldb.a

all: $(PROGRAMS)

leveldb.a: $(LIBOBJECTS)
	$(AR) $(ARFLAGS) $@ $^

%.o: %.cc
	$(CC) $(CFLAGS) $< -o $@

.PHONY: clean
clean:
	rm -f */*.o $(PROGRAMS) $(TESTS)

test: $(TESTS)
	@ for t in $(TESTS); do echo "=== Running $$t ==="; ./$$t || exit 1; done

arena_test: ./util/arena.o ./util/arena_test.o
	$(CC) $(LDFLAGS) $^ -o $@