#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_INIT([libhvsc], [1.0.0], [b.wassink@ziggo.nl])
AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE

# Checks for programs.
AC_PROG_CPP
AC_PROG_CC_C99
AC_PROG_RANLIB

AC_LANG([C])

# Checks for libraries.

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([inttypes.h limits.h stdint.h stdlib.h string.h])


AC_CONFIG_FILES([Makefile
                 src/bin/Makefile
                 src/lib/Makefile])
AC_OUTPUT
