dnl Check for libdaemon
dnl On success, HAVE_LIBDAEMON is set to 1 and PKG_CONFIG_REQUIRES is filled when
dnl libdaemon is found using pkg-config

AC_DEFUN([CHECK_LIBDAEMON],
[
  AC_MSG_CHECKING([libdaemon])
  HAVE_LIBDAEMON=0

  # Search using pkg-config
  if test x"$HAVE_LIBDAEMON" = "x0"; then
    if test x"$PKG_CONFIG" != "x"; then
      PKG_CHECK_MODULES([libdaemon], [libdaemon], [HAVE_LIBDAEMON=1], [HAVE_LIBDAEMON=0])
    fi
  fi

  if test x"$HAVE_LIBDAEMON" = "x0"; then
    for i in /usr /usr/local /opt/local /opt; do
      if test -f "$i/include/libdaemon/daemon.h" -a -f "$i/lib/libdaemon.la"; then
        libdaemon_CFLAGS="-I$i/include -L$i/lib"
        libdaemon_LIBS="-L$i/lib -ldaemon"

        HAVE_LIBDAEMON=1
      fi
    done
  fi

  # Search the library and headers directly (last chance)
  if test x"$HAVE_LIBDAEMON" = "x0"; then
    AC_CHECK_HEADER(libdaemon/daemon.h, [], [AC_MSG_ERROR([The libdaemon headers are missing])])
    AC_CHECK_LIB(daemon, daemon_signal_next, [], [AC_MSG_ERROR([The libdaemon library is missing])])

    libdaemon_LIBS="-ldaemon"
    HAVE_LIBDAEMON=1
  fi

  if test x"$HAVE_LIBDAEMON" = "x0"; then
    AC_MSG_ERROR([libdaemon is mandatory.])
  fi

  AC_SUBST(libdaemon_LIBS)
  AC_SUBST(libdaemon_CFLAGS)
])
