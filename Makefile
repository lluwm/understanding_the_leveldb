# Copyright (c) 2011 The LevelDB Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file. See the AUTHORS file for names of contributors.

CC = g++

CFLAGS = -c -I. -std=c++11 -stdlib=libc++

LIBOBJECTS = \
		./db/memtable.o\
		./db/skiplist.o\
		./util/arena.o

PROGRAMS = leveldb.a

all: $(PROGRAMS)

leveldb.a: $(LIBOBJECTS)
	$(AR) $(ARFLAGS) $@ $^

%.o: %.cc
	$(CC) $(CFLAGS) $< -o $@

.PHONY: clean
clean:
	rm -f */*.o
	rm $(PROGRAMS)