#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([actor], [1.0.0], [laurent.vanbegin76@gmail.com])
AM_INIT_AUTOMAKE([subdir-objects])
AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIRS([m4])
CXXFLAGS=
# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
#AC_PROG_RANLIB
# Checks for libraries.
AM_INIT_AUTOMAKE()
#LT_INIT
AC_ENABLE_SHARED
AC_DISABLE_STATIC
AC_PROG_LIBTOOL
# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h memory.h netdb.h netinet/in.h sys/socket.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_TYPE_SIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_ERROR_AT_LINE
AC_CHECK_FUNCS([select socket])
AC_CHECK_LIB(pthread, pthread_create)
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
