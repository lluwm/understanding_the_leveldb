CC = g++

CFLAGS = -c -std=c++11 -stdlib=libc++

LIBOBJECTS = \
		./db/memtable.o

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