/*
 * license
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <syslog.h>

#include "main.h"
#include "debug.h"

static int debug_threshold=2;
static int debug_output_destination = DBG_OUTPUT_STDERR;
static int syslog_map[] = {
    LOG_CRIT,
    LOG_ERR,
    LOG_WARNING,
    LOG_INFO,
    LOG_DEBUG
};

/**
 * change logging destination.  Usually just shift from stderr
 * to syslog when daemonizing, but could be expanded to log to a file
 * or something else interesting.
 *
 * THIS IS NOT THREADSAFE
 *
 * @param what new log destination (DBG_OUTPUT_*)
 * @param param type specific parameter (filename, syslog ident, etc )
 */
void debug_output(int what, char *param) {
    /* must be serialized on multi-threaded apps */
    assert(what == DBG_OUTPUT_STDERR || what == DBG_OUTPUT_SYSLOG);

    if(what != DBG_OUTPUT_STDERR && what != DBG_OUTPUT_SYSLOG)
        return;

    /* terminate old logging method */
    switch(debug_output_destination) {
    case DBG_OUTPUT_STDERR:
        /* if we were switching from stderr, don't have to do anything */
        break;
    case DBG_OUTPUT_SYSLOG:
        closelog();
        break;
    default:
        break;
    }

    /* initialize new logging method */
    switch(what) {
    case DBG_OUTPUT_SYSLOG:
        openlog(param, LOG_PID, LOG_DAEMON);
        break;
    default:
        break;
    }

    debug_output_destination = what;
}

/**
 * set new debuglevel.  0: error only, 5: debug.  Note: must
 * not be compiled with NDEBUG for error levels > 2 to do anything.
 *
 * @param newlevel new logging level
 */
void debug_level(int newlevel) {
    assert(newlevel >= 0 && newlevel <= 5);

    if(newlevel >= 0 && newlevel <= 5)
        debug_threshold = newlevel;
}

/**
 * printf-type interface for emitting debug messages
 *
 * @param level what loglevel (DBG_FATAL, DBG_*)
 * @param format printf-style format
 */
void debug_printf(int level, char *format, ...) {
    va_list args;
    char *stringp;
    char *format_new;

    assert(format);
    assert(level >= 0 && level <= 5);

    if(!format || level < 0 || level > 5 || level > debug_threshold)
        return;

    format_new = strdup(format);

    if(!format_new)
        return;

    va_start(args, format);

    switch(debug_output_destination) {
    case DBG_OUTPUT_STDERR:
        /* should be serialized if multi-threaded */
        vfprintf(stderr, format_new, args);
        break;
    case DBG_OUTPUT_SYSLOG:
        vasprintf(&stringp, format, args);
        if(format_new[strlen(format_new)-1] == '\n') {
            format_new[strlen(format_new)-1] = '\0';
        }
        syslog(syslog_map[level], stringp);
        free(stringp);
        break;

    default:
        break;
    }

    free(format_new);
    va_end(args);
}
