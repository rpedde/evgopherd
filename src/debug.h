/*
 * license
 */

#ifndef _DEBUG_H_
#define _DEBUG_H_

#define DBG_FATAL 0
#define DBG_ERROR 1
#define DBG_WARN  2
#define DBG_INFO  3
#define DBG_DEBUG 4

#define DBG_OUTPUT_SYSLOG 0
#define DBG_OUTPUT_STDERR 1

#if defined(NDEBUG)

#define DEBUG(format, args...)
#define INFO(format, args...)
#define WARN(format, args...)
#define ERROR(format, args...) debug_printf(DBG_ERROR, "[ERROR] %s:%d (%s): " format "\n", __FILE__, __LINE__, __FUNCTION__, ##args)
#define FATAL(format, args...) debug_printf(DBG_FATAL, "[FATAL] %s:%d (%s): " format "\n", __FILE__, __LINE__, __FUNCTION__, ##args)

#define DPRINTF(level, format, args...);

#else

#define DEBUG(format, args...) debug_printf(DBG_DEBUG, "[DEBUG] %s:%d (%s): " format "\n", __FILE__, __LINE__, __FUNCTION__, ##args)
#define INFO(format, args...) debug_printf(DBG_INFO, "[INFO] %s:%d (%s): " format "\n", __FILE__, __LINE__, __FUNCTION__, ##args)
#define WARN(format, args...) debug_printf(DBG_WARN, "[WARN] %s:%d (%s): " format "\n", __FILE__, __LINE__, __FUNCTION__, ##args)
#define ERROR(format, args...) debug_printf(DBG_ERROR, "[ERROR] %s:%d (%s): " format "\n", __FILE__, __LINE__, __FUNCTION__, ##args)
#define FATAL(format, args...) debug_printf(DBG_FATAL, "[FATAL] %s:%d (%s): " format "\n", __FILE__, __LINE__, __FUNCTION__, ##args)

#define DPRINTF(level, format, args...)  debug_printf(level, "[%s] %s:%d (%s): " format "\n", #level, __FILE__, __LINE__, __FUNCTION__, ##args)

#endif /* NDEBUG */

extern void debug_printf(int level, char *format, ...);
extern void debug_level(int newlevel);
extern void debug_output(int what, char *param);

#endif /* _DEBUG_H_ */
