pkglibdir=$(libdir)/evgopherd
sbin_PROGRAMS = evgopherd

evgopherd_SOURCES = main.c main.h debug.c debug.h
evgopherd_CFLAGS = $(libevent_CFLAGS) $(libdaemon_CFLAGS)
evgopherd_LDFLAGS = $(libevent_LIBS) $(libdaemon_LIBS)

pkglib_LTLIBRARIES=dir.la file.la

dir_la_SOURCES=plugin-dir.c debug.h plugin.h
dir_la_LDFLAGS = -module -avoid-version -shared

file_la_SOURCES=plugin-file.c debug.h plugin.h
file_la_LDFLAGS = -module -avoid-version -shared
