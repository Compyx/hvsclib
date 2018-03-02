# vim: set noet ts=8 :
VPATH = src:src/lib
CC = gcc
LD = gcc

INSTALL_PREFIX=/usr/local

CFLAGS = -Wall -Wextra -pedantic -std=c99 -Wshadow -Wpointer-arith \
	 -Wcast-qual -Wcast-align -Wstrict-prototypes -Wmissing-prototypes \
	 -Wswitch -Wswitch-default -Wuninitialized -Wconversion \
	 -Wredundant-decls -Wnested-externs -Wunreachable-code \
	 -O3 -g -Isrc -Isrc/lib -DHVSC_DEBUG

LDFLAGS = -lgcrypt


LIB = libhvsc.so
LIB_OBJS = base.o main.o sldb.o stil.o
LIB_HEADERS = hvsc_defs.h base.h

TESTER = hvsc-test
TESTER_OBJS = hvsc-test.o


all: $(TESTER)

# dependencies of the various objects, according to headers included

.PHONY: clean
clean:
	rm -f *.o
	rm -f $(TESTER) $(LIB)

.PHONY: doc
doc:
	doxygen 1>/dev/null

.PHONY: distclean
distclean: clean
	rm -rfd doc/html/*


$(LIB): $(LIB_OBJS)
	$(LD) -shared -Wl,-soname,$(LIB) -o $@ $^ $(LDFLAGS)

$(TESTER): $(TESTER_OBJS) $(LIB)
	$(LD) -o $(TESTER) $(TESTER_OBJS) $(LIB) -L. $(LDFLAGS)



%.o: %.c $(LIB_HEADERS)
	$(CC) $(CFLAGS) -fPIC -c -o $@ $< -Isrc/lib


