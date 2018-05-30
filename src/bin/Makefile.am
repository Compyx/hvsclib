AM_CFLAGS = -I$(top_srcdir)/src/bin \
			-I$(top_srcdir)/src/lib

bin_PROGRAMS = hvsc_test
hvsc_test_SOURCES = hvsc_test.c

hvsc_test_LDADD = $(top_builddir)/src/lib/libhvsc.a $(AM_LDFLAGS)
