/*
 * Copyright (C) 2015, 2016, 2019--2023  T+A elektroakustik GmbH & Co. KG
 *
 * This file is part of the T+A Streaming Board software stack ("StrBoWare").
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301, USA.
 */

#ifndef MESSAGES_H
#define MESSAGES_H

/*!
 * Set to 0 to disable trace message.
 */
#ifndef MSG_TRACE_ENABLED
#define MSG_TRACE_ENABLED 1
#endif /* !MSG_TRACE_ENABLED */

/*!
 * Set to 1 to include the thread ID in each log message.
 */
#ifndef MSG_WITH_THREAD_ID
#define MSG_WITH_THREAD_ID 0
#endif /* !MSG_WITH_THREAD_ID */

/*!
 * Set to 0 for no specific action on #MSG_BUG() and #MSG_BUG_IF().
 * Set to 1 to dump a backtrace on #MSG_BUG() and #MSG_BUG_IF().
 * Set to 2 to call os_abort() on #MSG_BUG() and #MSG_BUG_IF().
 */
#ifndef MSG_ACTION_ON_BUG
#define MSG_ACTION_ON_BUG 0
#endif /* !MSG_ACTION_ON_BUG */

/*!
 * Set to 0 for no specific action on #MSG_UNREACHABLE().
 * Set to 1 to dump a backtrace on #MSG_UNREACHABLE().
 * Set to 2 to call os_abort() on #MSG_UNREACHABLE().
 */
#ifndef MSG_ACTION_ON_UNREACHABLE
#define MSG_ACTION_ON_UNREACHABLE 0
#endif /* !MSG_ACTION_ON_UNREACHABLE */

/*!
 * Set to 0 for no specific action on #MSG_NOT_IMPLEMENTED().
 * Set to 1 to dump a backtrace on #MSG_NOT_IMPLEMENTED().
 * Set to 2 to call os_abort() on #MSG_NOT_IMPLEMENTED().
 */
#ifndef MSG_ACTION_ON_NOT_IMPLEMENTED
#define MSG_ACTION_ON_NOT_IMPLEMENTED 0
#endif /* !MSG_ACTION_ON_NOT_IMPLEMENTED */

/*!
 * Set to 0 for no specific action on GLib failure.
 * Set to 1 to dump a backtrace on GLib failure.
 * Set to 2 to call os_abort() on GLib failure.
 */
#ifndef MSG_ACTION_ON_GLIB_FAILURE
#define MSG_ACTION_ON_GLIB_FAILURE 0
#endif /* MSG_ACTION_ON_GLIB_FAILURE */

/*!
 * Set to 0 for no specific action on #os_abort().
 * Set to 1 to dump a backtrace on #os_abort().
 */
#ifndef MSG_ACTION_ON_ABORT
#define MSG_ACTION_ON_ABORT 0
#endif /* MSG_ACTION_ON_ABORT */

#include <stdbool.h>
#include <syslog.h>

#include "os.h"

#if MSG_ACTION_ON_BUG == 1 || MSG_ACTION_ON_UNREACHABLE == 1 || MSG_ACTION_ON_NOT_IMPLEMENTED == 1
#include "backtrace.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * Verbosity levels as passed to #msg_vinfo(), #msg_vyak(), #msg_is_verbose(),
 * and #msg_set_verbose_level().
 */
enum MessageVerboseLevel
{
    /* message levels for filtering messages by importance */
    MESSAGE_LEVEL_QUIET      = -3,
    MESSAGE_LEVEL_BAD_NEWS   = -2,
    MESSAGE_LEVEL_IMPORTANT  = -1,
    MESSAGE_LEVEL_NORMAL     =  0,
    MESSAGE_LEVEL_DIAG       =  1,
    MESSAGE_LEVEL_DEBUG      =  2,
    MESSAGE_LEVEL_TRACE      =  3,

    /* stable names for special values */
    MESSAGE_LEVEL_MIN        = MESSAGE_LEVEL_QUIET,
    MESSAGE_LEVEL_MAX        = MESSAGE_LEVEL_TRACE,
    MESSAGE_LEVEL_INFO_MIN   = MESSAGE_LEVEL_BAD_NEWS,
    MESSAGE_LEVEL_INFO_MAX   = MESSAGE_LEVEL_TRACE,

    /* do not use, value is used internally */
    MESSAGE_LEVEL_IMPOSSIBLE = -4,
};

/*!
 * Whether or not to make use of syslog.
 */
void msg_enable_syslog(bool enable_syslog);

/*!
 * Whether or not to make use of colors on console output.
 */
void msg_enable_color_console(bool enable_colors);

/*!
 * How much logging should be done.
 */
void msg_set_verbose_level(enum MessageVerboseLevel level);

/*!
 * Read out verbositiy level directly.
 */
enum MessageVerboseLevel msg_get_verbose_level(void);

/*!
 * Check whether or not the given level is currently verbose.
 *
 * Use this function to enable or disable execution of code paths depending on
 * verbosity level.
 *
 * \returns
 *     True if global verbosity level is high enough to allow the given level
 *     to be verbose (debug code should be executed), false if the given level
 *     is filtered (debug code should \e not be executed).
 */
bool msg_is_verbose(enum MessageVerboseLevel level);

/*!
 * Map verbosity level name to enumeration value.
 *
 * \returns
 *     A valid verbosity level between #MESSAGE_LEVEL_MIN and
 *     #MESSAGE_LEVEL_MAX (including boundaries), or #MESSAGE_LEVEL_IMPOSSIBLE
 *     in case the passed name is unknown.
 */
enum MessageVerboseLevel msg_verbose_level_name_to_level(const char *name);

/*!
 * Map verbosity level enumeration value to verbosity level name.
 *
 * \returns
 *     A valid verbosity level name, or a \c NULL pointer in case the given
 *     numeric level is invalid.
 */
const char *msg_verbose_level_to_level_name(enum MessageVerboseLevel level);

/*!
 * Return list of supported verbosity level names.
 *
 * The list is sorted by increasing order of verbosity. It is terminated by a
 * \c NULL pointer.
 */
const char *const *msg_get_verbose_level_names(void);

/*!
 * Emit error to stderr and syslog.
 *
 * \param error_code
 *     The current error code as stored in errno.
 *
 * \param priority
 *     A log priority as expected by syslog(3). This priority is mapped to
 *     #MessageVerboseLevel values as follows:
 *
 *     - \c LOG_EMERG, \c LOG_ALERT, and \c LOG_CRIT are all mapped to
 *       #MESSAGE_LEVEL_QUIET. This means that such errors are always shown,
 *       regardless of configured verbosity.
 *     - \c LOG_ERR and \c LOG_WARNING are mapped to #MESSAGE_LEVEL_IMPORTANT.
 *     - \c LOG_NOTICE is mapped to #MESSAGE_LEVEL_NORMAL.
 *     - \c LOG_INFO is mapped to #MESSAGE_LEVEL_DIAG.
 *     - \c LOG_DEBUG is mapped to #MESSAGE_LEVEL_DEBUG.
 *
 * \param error_format
 *     Format string followed by arguments.
 */
void msg_error(int error_code, int priority, const char *error_format, ...)
    __attribute__ ((format (printf, 3, 4)));

/*!
 * Emit informative message to stderr and syslog.
 *
 * This function emits messages using the standard verbose level
 * #MESSAGE_LEVEL_NORMAL.
 */
void msg_info(const char *format_string, ...)
    __attribute__ ((format (printf, 1, 2)));

/*!
 * Emit informative message to stderr and syslog if given verbosity level does
 * not exceed the currently configured verbosity level.
 *
 * Passing a \p level lower than #MESSAGE_LEVEL_INFO_MIN or higher than
 * #MESSAGE_LEVEL_INFO_MAX is a programming error.
 *
 * \see
 *     #msg_info()
 */
void msg_vinfo(enum MessageVerboseLevel level, const char *format_string, ...)
    __attribute__ ((format (printf, 2, 3)));

/*!
 * Same as #msg_info(), but for rather unimportant messages.
 *
 * The main difference between #msg_info() and this function is that this
 * function is always ignored in unit tests. There is no need to declare
 * expectations for these.
 *
 * In general, log messages emitted via this function are usually characterized
 * by being helpful to developers to see what's going on in case of a problem,
 * but not being very helpful to others. They are basically noise which is
 * emitted as side-effect of program execution, but they are not directly
 * related to events significant to the user.
 *
 * In other words, the messages emitted by #msg_yak() is the system just
 * yakking at you, while the messages emitted by #msg_info() and #msg_error()
 * is the system informing about significant steps and error conditions.
 */
void msg_yak(const char *format_string, ...)
    __attribute__ ((format (printf, 1, 2)));

/*!
 * Same as #msg_vinfo(), but for rather unimportant messages.
 *
 * It can be useful to restrict yakking to the various message levels, so here
 * is the function to achieve this.
 *
 * \see #msg_yak()
 */
void msg_vyak(enum MessageVerboseLevel level, const char *format_string, ...)
    __attribute__ ((format (printf, 2, 3)));

/*!
 * Emit standard log message about out of memory condition.
 *
 * This message is emitted at verbosity level #MESSAGE_LEVEL_QUIET, thus always
 * shown.
 *
 * \returns
 *     Always -1 so that it is possible to emit the message and return an
 *     error in one line of code.
 */
int msg_out_of_memory(const char *what);

#ifdef __cplusplus
}
#endif

#if MSG_ACTION_ON_BUG == 1
#define MSG_BUG_ACTION_() backtrace_log(0, "bug context")
#elif MSG_ACTION_ON_BUG == 2
#define MSG_BUG_ACTION_() os_abort()
#else
#define MSG_BUG_ACTION_() do {} while(0)
#endif /* MSG_ACTION_ON_BUG */

/*!
 * Emit a bug message.
 */
#define MSG_BUG(...) \
    do \
    { \
        msg_error(0, LOG_CRIT, "BUG: " __VA_ARGS__); \
        MSG_BUG_ACTION_(); \
    } \
    while(0)

/*!
 * Emit a bug message if \p COND evaluates to true.
 */
#define MSG_BUG_IF(COND, ...) \
    do \
    { \
        if(COND)\
            MSG_BUG(__VA_ARGS__); \
    } \
    while(0)

#define MSG_TODO(TICKET, FMT, ...) \
    msg_error(0, LOG_CRIT, "TODO [#%u]: " FMT, TICKET, ##__VA_ARGS__)

#if MSG_ACTION_ON_UNREACHABLE == 1
#define MSG_UNREACHABLE_ACTION_() backtrace_log(0, "unreachable context")
#elif MSG_ACTION_ON_UNREACHABLE == 2
#define MSG_UNREACHABLE_ACTION_() os_abort()
#else
#define MSG_UNREACHABLE_ACTION_() do {} while(0)
#endif /* MSG_ACTION_ON_UNREACHABLE */

#define MSG_UNREACHABLE() \
    do \
    { \
        msg_error(EFAULT, LOG_CRIT, "BUG: Reached unreachable code %s(%d)", \
                  __func__, __LINE__); \
        MSG_UNREACHABLE_ACTION_(); \
    } \
    while(0)

#if MSG_ACTION_ON_NOT_IMPLEMENTED == 1
#define MSG_NOT_IMPLEMENTED_ACTION_() backtrace_log(0, "not implemented context")
#elif MSG_ACTION_ON_NOT_IMPLEMENTED == 2
#define MSG_NOT_IMPLEMENTED_ACTION_() os_abort()
#else
#define MSG_NOT_IMPLEMENTED_ACTION_() do {} while(0)
#endif /* MSG_ACTION_ON_NOT_IMPLEMENTED */

#define MSG_NOT_IMPLEMENTED() \
    do \
    { \
        msg_error(ENOSYS, LOG_CRIT, "TODO: Not implemented: %s(%d)", \
                  __func__, __LINE__); \
        MSG_NOT_IMPLEMENTED_ACTION_(); \
    } \
    while(0)

#define MSG_APPLIANCE_BUG(...) msg_error(0, LOG_CRIT, "APPLIANCE BUG: " __VA_ARGS__)

#if !defined(MSG_TRACE_PREFIX)
#define MSG_TRACE_PREFIX "*** "
#endif /* !MSG_TRACE_PREFIX */

#if !defined(MSG_TRACE_SUFFIX)
#define MSG_TRACE_SUFFIX ""
#endif /* !MSG_TRACE_SUFFIX */

#if !defined(MSG_TRACE_FUNCTION)
#define MSG_TRACE_FUNCTION msg_yak
#endif /* !MSG_TRACE_FUNCTION */

#define MSG_TRACE() \
    MSG_PRTRACE(MSG_TRACE_PREFIX)

#define MSG_TRACE_FORMAT(FMT, ...) \
    MSG_PRTRACE_FORMAT(MSG_TRACE_PREFIX, FMT, __VA_ARGS__)

#define MSG_TRACE_THIS() MSG_TRACE_FORMAT("%p", static_cast<const void *>(this))

#if MSG_TRACE_ENABLED
#define MSG_PRTRACE(PREFIX) \
    do \
    { \
        MSG_TRACE_FUNCTION(PREFIX "%s(%d)" MSG_TRACE_SUFFIX, __func__, __LINE__); \
    } \
    while(0)

#define MSG_PRTRACE_FORMAT(PREFIX, FMT, ...) \
    do \
    { \
        MSG_TRACE_FUNCTION(PREFIX "%s(%d): " FMT MSG_TRACE_SUFFIX, __func__, __LINE__, __VA_ARGS__); \
    } \
    while(0)
#else /* !MSG_TRACE_ENABLED */
#define MSG_PRTRACE(PREFIX)                     do {} while(0)
#define MSG_PRTRACE_FORMAT(PREFIX, FMT, ...)    do {} while(0)
#endif /* MSG_TRACE_ENABLED */

#ifdef NDEBUG
#define msg_log_assert(EXPR) do {} while(0)
#else /* !NDEBUG */
#define msg_log_assert(EXPR) \
    do \
    { \
        if(!(EXPR)) \
        { \
            msg_error(0, LOG_EMERG, "Assertion failed at %s:%d: " #EXPR, \
                      __FILE__, __LINE__); \
            os_abort(); \
        } \
    } \
    while(0)
#endif /* NDEBUG */

#ifdef MSG_ENABLE_LEGACY_MACROS
#define BUG(...)                MSG_BUG(__VA_ARGS__)
#define BUG_IF(COND, ...)       MSG_BUG_IF(__VA_ARGS__)
#define TODO(TICKET, FMT, ...)  MSG_TODO(TICKET, FMT, __VA_ARGS__)
#define APPLIANCE_BUG(...)      MSG_APPLIANCE_BUG(__VA_ARGS__)
#define log_assert(EXPR)        msg_log_assert(EXPR)
#endif /* MSG_ENABLE_LEGACY_MACROS */

#endif /* !MESSAGES_H */
