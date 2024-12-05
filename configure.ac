#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([colibri-core],[2.5.9],[proycon@anaproy.nl])
AC_CONFIG_SRCDIR([configure.ac])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE

AX_REQUIRE_DEFINED([AX_CXX_COMPILE_STDCXX_11])

# Checks for programs.
AC_PROG_CXX( [g++] )
AC_LANG([C++])
AX_CXX_COMPILE_STDCXX_11

AC_CHECK_HEADERS([bzlib.h],
                [LIBS="$LIBS -lbz2"],
                [AC_MSG_ERROR([libbz2-dev not found])] )

# use libtool
LT_INIT
AC_SUBST([LIBTOOL_DEPS])

# when running tests, use CXX
# Checks for libraries.

if test $prefix = "NONE"; then
   prefix="$ac_default_prefix"
fi



PKG_PROG_PKG_CONFIG

# Checks for timbl library.
# inspired by feh-1.3.4/configure.ac.  Tnx Tom Gilbert and feh hackers.

# ugly hack when PKG_CONFIG_PATH isn't defined.
# couldn't get it to work otherwise
if test "x$PKG_CONFIG_PATH" = x; then
     export PKG_CONFIG_PATH=""
fi

AC_ARG_WITH(folia-path,
       [  --with-folia-path=DIR       use libfolia installed in <DIR>],
       [PKG_CONFIG_PATH="$withval/lib/pkgconfig:$PKG_CONFIG_PATH"],
       [PKG_CONFIG_PATH="$prefix/lib/pkgconfig:$PKG_CONFIG_PATH"])

AC_ARG_WITH([folia], AS_HELP_STRING([--with-folia], [Build with FoLiA support (default: disabled)]))

AS_IF([test "x$with_folia" = "xyes"], [
    AC_MSG_NOTICE( [pkg-config search path: $PKG_CONFIG_PATH] )
    PKG_CHECK_MODULES([folia],[folia >= 0.10])
    CXXFLAGS="$CXXFLAGS $folia_CFLAGS"
    LIBS="$folia_LIBS $LIBS"
    AC_DEFINE([WITHFOLIA],[1],[Compile with libfolia support])
])

# Checks for library functions.
AC_CONFIG_FILES([
  colibri-core.pc
  Makefile
  src/Makefile
  include/Makefile
  scripts/Makefile
])
AC_OUTPUT
