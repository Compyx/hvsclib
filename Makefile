# vim: set noet ts=8 :
#
# Makefile
#
# Two macros can be used to alter the behaviour of hvsclib:
#
# * HVSC_DEBUG    Generate debugging info on stdout
# * HVSC_USE_MD5  Use md5 functions in sldb.c and link against libgcrypt20
#
# Also a big thank you to Blacky Startdust of VICE Team for helping me with
# creating both a static and a shared lib!
#
#

# Library version number vars, these are passed to the code via defines
VERSION_MAJ = 0
VERSION_MIN = 0
VERSION_REV = 99

VPATH = src:src/lib
CC = gcc
LD = gcc

# Installation prefix
#
# TODO: change to /usr/local once the 'install' recipe is properly written
#
PREFIX = /tmp

CFLAGS = -Wall -Wextra -pedantic -std=c99 -Wshadow -Wpointer-arith \
	 -Wcast-qual -Wcast-align -Wstrict-prototypes -Wmissing-prototypes \
	 -Wswitch -Wswitch-default -Wuninitialized -Wconversion \
	 -Wredundant-decls -Wnested-externs -Wunreachable-code \
	 -O3 -g -Isrc -Isrc/lib -DHVSC_DEBUG \
	 -DHVSC_LIB_VERSION_MAJ=$(VERSION_MAJ) \
	 -DHVSC_LIB_VERSION_MIN=$(VERSION_MIN) \
	 -DHVSC_LIB_VERSION_REV=$(VERSION_REV)




# Only use this when defining HVSC_USE_MD5
# LDFLAGS = -lgcrypt

LDFLAGS = -shared
SHARED_LIB = libhvsc.so
STATIC_LIB = libhvsc.a
LIB_HEADER = hvsc.h

SRCS = src/lib/base.c src/lib/main.c src/lib/psid.c src/lib/sldb.c \
	   src/lib/stil.c src/lib/bugs.c
HEADERS = hvsc.h hvsc_defs.h

SHOBJS = $(SRCS:.c=.os)
OBJS = $(SRCS:.c=.o)

TESTER = hvsc-test
TESTER_OBJS = hvsc-test.o $(STATIC_LIB)

LIB_SONAME = $(SHARED_LIB).$(VERSION_MAJ).$(VERSION_MIN)


all: $(SHARED_LIB) $(STATIC_LIB) $(TESTER)

$(SHARED_LIB): $(SHOBJS)
	$(CC) ${LDFLAGS} -o ${SHARED_LIB} $^

$(STATIC_LIB): $(OBJS)
	ar cr ${STATIC_LIB} $^
	ranlib ${STATIC_LIB}

$(TESTER): $(TESTER_OBJS)
	$(LD) -o $(TESTER) $^

.SUFFIXES: .os

.c.os:
	$(CC) $(CFLAGS) -fPIC -c $< -o $@

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@



# dependencies of the various objects, according to headers included
base.o: base.h
bugs.o: bugs.h base.o
psid.o: psid.h base.o
sldb.o: sldb.h base.o
stil.o: stil.h base.o


.PHONY: clean
clean:
	rm -f *.o
	rm -f *.os
	rm -f $(TESTER) $(SHARED_LIB) $(STATIC_LIB)
	rm -f *.sid

.PHONY: doc
doc:
	doxygen 1>/dev/null

.PHONY: distclean
distclean: clean
	rm -rfd doc/html/*

.PHONY: install
install:
	mkdir -p $(PREFIX)/lib
	mkdir -p $(PREFIX)/include
	cp $(STATIC_LIB) $(PREFIX)/lib/
	cp $(SHARED_LIB) $(PREFIX)/lib/$(LIB_SONAME)
	ln -sf $(PREFIX)/lib/$(LIB_SONAME) $(PREFIX)/lib/$(SHARED_LIB)
	cp src/lib/$(LIB_HEADER) $(PREFIX)/include/$(LIB_HEADER)

