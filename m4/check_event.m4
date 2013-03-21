dnl Check for libevent
dnl On success, HAVE_LIBEVENT is set to 1 and PKG_CONFIG_REQUIRES is filled when
dnl libevent is found using pkg-config

AC_DEFUN([CHECK_LIBEVENT],
[
  AC_MSG_CHECKING([libevent])
  HAVE_LIBEVENT=0

  # Search using pkg-config
  if test x"$HAVE_LIBEVENT" = "x0"; then
    if test x"$PKG_CONFIG" != "x"; then
      PKG_CHECK_MODULES([libevent], [libevent], [HAVE_LIBEVENT=1], [HAVE_LIBEVENT=0])
      # if test x"$HAVE_FUSE" = "x1"; then
      #   # if test x"$PKG_CONFIG_REQUIRES" != x""; then
      #   #   PKG_CONFIG_REQUIRES="$PKG_CONFIG_REQUIRES,"
      #   # fi
      #   # PKG_CONFIG_REQUIRES="$PKG_CONFIG_REQUIRES lib"
      # fi
    fi
  fi

  if test x"$HAVE_LIBEVENT" = "x0"; then
    for i in /usr /usr/local /opt/local /opt; do
      if test -f "$i/include/event.h" -a -f "$i/lib/libevent.la"; then
        libevent_CFLAGS="-I$i/include -L$i/lib"
        libevent_LIBS="-L$i/lib -levent"

        HAVE_LIBEVENT=1
      fi
    done
  fi

  # Search the library and headers directly (last chance)
  if test x"$HAVE_LIBEVENT" = "x0"; then
    AC_CHECK_HEADER(event.h, [], [AC_MSG_ERROR([The libevent headers are missing])])
    AC_CHECK_LIB(event, event_add, [], [AC_MSG_ERROR([The libevent library is missing])])

    libevent_LIBS="-levent"
    HAVE_LIBEVENT=1
  fi

  if test x"$HAVE_LIBEVENT" = "x0"; then
    AC_MSG_ERROR([libevent is mandatory.])
  fi

  AC_SUBST(libevent_LIBS)
  AC_SUBST(libevent_CFLAGS)
])
