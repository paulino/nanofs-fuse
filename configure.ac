#  -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

PACKAGE = nanofs-fuse
VERSION = 0.3.0
AC_CONFIG_MACRO_DIR([m4])
AC_INIT([nanofs-fuse], [0.3.0], [paulino@dte.us.es])

AM_INIT_AUTOMAKE([foreign])


AC_CONFIG_SRCDIR([src/mknanofs.c])
AC_CONFIG_HEADERS([config.h])
AC_LANG([C])


AC_PROG_LIBTOOL
AC_PROG_CC_STDC
AC_SYS_LARGEFILE





# Checks for programs.
AC_PROG_CC
CFLAGS="$CFLAGS -Wall -W"

LT_INIT

# Checks for libraries.
AC_CHECK_LIB([getopt])
PKG_CHECK_MODULES(FUSE, [fuse >= 2.6])

AC_CHECK_PROG(UNMOUNT_COMMAND, fusermount, fusermount -u, umount)

AM_INIT_AUTOMAKE

# Checks for typedefs, structures, and compiler characteristics.
AC_CONFIG_FILES([
 Makefile 
 src/Makefile
 man/Makefile
 ])

AC_OUTPUT