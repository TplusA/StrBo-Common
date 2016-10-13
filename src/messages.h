/*
 * Copyright (C) 2015, 2016  T+A elektroakustik GmbH & Co. KG
 *
 * This file is part of the T+A Streaming Board software stack ("StrBoWare").
 *
 * StrBoWare is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 3 as
 * published by the Free Software Foundation.
 *
 * StrBoWare is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with StrBoWare.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MESSAGES_H
#define MESSAGES_H

#include <stdbool.h>
#include <syslog.h>

#include "os.h"

#ifdef __cplusplus
extern "C" {
#endif

enum MessageVerboseLevel
{
    /* message levels for filtering messages by importance */
    MESSAGE_LEVEL_QUIET      = -2,
    MESSAGE_LEVEL_IMPORTANT  = -1,
    MESSAGE_LEVEL_NORMAL     =  0,
    MESSAGE_LEVEL_DIAG       =  1,
    MESSAGE_LEVEL_DEBUG      =  2,
    MESSAGE_LEVEL_TRACE      =  3,

    /* stable names for special values */
    MESSAGE_LEVEL_MIN        = MESSAGE_LEVEL_QUIET,
    MESSAGE_LEVEL_MAX        = MESSAGE_LEVEL_TRACE,
    MESSAGE_LEVEL_INFO_MIN   = MESSAGE_LEVEL_IMPORTANT,
    MESSAGE_LEVEL_INFO_MAX   = MESSAGE_LEVEL_TRACE,

    /* do not use, value is used internally */
    MESSAGE_LEVEL_IMPOSSIBLE = -3,
};

/*!
 * Whether or not to make use of syslog.
 */
void msg_enable_syslog(bool enable_syslog);

/*!
 * How much logging should be done.
 */
void msg_set_verbose_level(enum MessageVerboseLevel level);

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

#define BUG(...) msg_error(0, LOG_CRIT, "BUG: " __VA_ARGS__)

#ifdef NDEBUG
#define log_assert(EXPR) do {} while(0)
#else /* !NDEBUG */
#define log_assert(EXPR) \
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

#endif /* !MESSAGES_H */
