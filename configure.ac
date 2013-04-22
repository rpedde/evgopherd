# Required initializer
AC_PREREQ(2.59)
AC_INIT(evgopherd, 1.0.0, ron@pedde.com)
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([src/main.c])
AC_CONFIG_HEADER([config.h])
AC_CONFIG_MACRO_DIR([m4])

AC_C_INLINE

# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_LIBTOOL

PKG_PROG_PKG_CONFIG

ALL_CFLAGS="-std=c99"

DEBUG_CPPFLAGS=""
DEBUG_CFLAGS=""
DEBUG_LDFLAGS=""

HOST_CPPFLAGS=""
HOST_CFLAGS=""
HOST_LDFLAGS=""

AC_ARG_ENABLE(debug, [  --enable-debug                Enable debugging switches],
                       [ case "${enableval}" in
                         yes) DEBUG_CFLAGS="-O0 -fstack-protector-all -Wall -g";;
                         no) DEBUG_CPPFLAGS="-DNDEBUG";;
                         *) AC_MSG_ERROR(bad value ${enableval} for --enable-debug);;
                       esac ],
                       DEBUG_CFLAGS="-O0 -fstack-protector-all -Wall -g")

case $host in
     *-darwin*)
       is_darwin=yes
       dnl HOST_CPPFLAGS="-framework Cocoa"
       ;;
     *-linux*)
       is_linux=yes
       HOST_CPPFLAGS="-D_BSD_SOURCE -D_GNU_SOURCE"
       HOST_LDFLAGS="-ldl"
       ;;
esac

AM_CONDITIONAL(PLATFORM_DARWIN, test "x${is_darwin}" = xyes)
AM_CONDITIONAL(PLATFORM_LINUX, test "x${is_linux}" = xyes)

# Check libs
CHECK_LIBEVENT()
CHECK_LIBDAEMON()

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST

CFLAGS="$CFLAGS $ALL_CFLAGS $DEBUG_CFLAGS $HOST_CFLAGS"
CPPFLAGS="$CPPFLAGS $DEBUG_CPPFLAGS $HOST_CPPFLAGS"
LDFLAGS="$LDFLAGS $DEBUG_LDFLAGS $HOST_LDFLAGS"

# Write config.status and the Makefile
AC_OUTPUT(Makefile src/Makefile)
