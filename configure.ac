#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
# $Id: configure.ac 13188 2011-09-15 13:46:53Z sloot $
# $URL: https://ilk.uvt.nl/svn/sources/ucto/trunk/configure.ac $

AC_PREREQ([2.67])
AC_INIT([colibri-core],[2.4.9],[proycon@anaproy.nl])
AC_CONFIG_SRCDIR([configure.ac])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADER([config.h])
AM_INIT_AUTOMAKE

#AC_DEFINE_DIR([SYSCONF_PATH], sysconfdir, sysconfdir)

if test x"${CXXFLAGS+set}" = xset; then
  # the user set CXXFLAGS; don't override it.
  cxx_flags_were_set=true
else
  cxx_flags_were_set=false
fi

# Checks for programs.
#AC_PROG_CXX( [g++] )
AC_LANG([C++])
AX_CXX_COMPILE_STDCXX_11([ext])

AC_CHECK_HEADERS([bzlib.h],
                [LIBS="$LIBS -lbz2"],
                [AC_MSG_ERROR([libbz2-dev not found])] )

if $cxx_flags_were_set; then
  CXXFLAGS=$CXXFLAGS
fi

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
#AC_ARG_WITH(timbl,
#       [  --with-timbl=DIR       use timbl installed in <DIR>;
#               note that you can install timbl in a non-default directory with
#               ./configure --prefix=<DIR> in the timbl installation directory],
#       [PKG_CONFIG_PATH="$PKG_CONFIG_PATH:$withval/lib/pkgconfig"],
#       [PKG_CONFIG_PATH="$PKG_CONFIG_PATH:$prefix/lib/pkgconfig"])
#  AC_MSG_NOTICE( [pkg-config search path: $PKG_CONFIG_PATH] )
#PKG_CHECK_MODULES([timbl], [timbl >= 6.4.2] )
#CXXFLAGS="$CXXFLAGS $timbl_CFLAGS"
#LIBS="$LIBS $timbl_LIBS"

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


#AC_ARG_WITH(ticcutils,
#       [  --with-ticcutils=DIR       use ticcutils installed in <DIR>;
#               note that you can install ticcutils in a non-default directory with
#               ./configure --prefix=<DIR> in the ticcutils installation directory],
#       [PKG_CONFIG_PATH="$PKG_CONFIG_PATH:$withval/lib/pkgconfig"],
#       [PKG_CONFIG_PATH="$PKG_CONFIG_PATH:$prefix/lib/pkgconfig"])
##  AC_MSG_NOTICE( [pkg-config search path: $PKG_CONFIG_PATH] )
#PKG_CHECK_MODULES([ticcutils], [ticcutils >= 0.5] )
#CXXFLAGS="$CXXFLAGS $ticcutils_CFLAGS"
#LIBS="$ticcutils_LIBS $LIBS"

# Checks for library functions.
AC_CONFIG_FILES([
  colibri-core.pc
  Makefile
  src/Makefile
  include/Makefile
  scripts/Makefile
])
AC_OUTPUT
