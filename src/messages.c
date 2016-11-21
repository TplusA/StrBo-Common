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

#if HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

#include "messages.h"

static bool use_syslog;
static enum MessageVerboseLevel current_verbosity;

/*!
 * All verbosity levels as strings.
 *
 * \attention
 *     This array must match the values listed in the #MessageVerboseLevel
 *     enumeration.
 */
static const char *verbosity_level_names[] =
{
    "quiet",
    "important",
    "normal",
    "diag",
    "debug",
    "trace",
    NULL
};

void msg_enable_syslog(bool enable_syslog)
{
    use_syslog = enable_syslog;
}

void msg_set_verbose_level(enum MessageVerboseLevel level)
{
    if(level >= MESSAGE_LEVEL_MIN && level <= MESSAGE_LEVEL_MAX)
        current_verbosity = level;
}

enum MessageVerboseLevel msg_get_verbose_level(void)
{
    return current_verbosity;
}

bool msg_is_verbose(enum MessageVerboseLevel level)
{
    return level <= current_verbosity;
}

enum MessageVerboseLevel msg_verbose_level_name_to_level(const char *name)
{
    enum MessageVerboseLevel level = MESSAGE_LEVEL_MIN;
    const char *const *names = verbosity_level_names;

    for(const char *n = *names; n != NULL; n = *++names)
    {
        if(strcmp(name, n) == 0)
            return level;
        else
            ++level;
    }

    return MESSAGE_LEVEL_IMPOSSIBLE;
}

const char *const *msg_get_verbose_level_names(void)
{
    return verbosity_level_names;
}

static void show_message(enum MessageVerboseLevel level, int error_code,
                         int priority, const char *format_string, va_list va)
{
    if(level > current_verbosity)
        return;

    char buffer[1024];
    size_t len = vsnprintf(buffer, sizeof(buffer), format_string, va);

    if(error_code != 0 && len < sizeof(buffer))
        len += snprintf(buffer + len, sizeof(buffer) - len,
                        " (%s)", strerror(error_code));

    if(len > sizeof(buffer))
        len = sizeof(buffer);

    if(use_syslog)
    {
        if(len <= 256)
            syslog(priority, "%s", buffer);
        else
        {
            int part = 1;
            size_t i = 0;

            syslog(priority, "[split long message]");

            while(i < len)
            {
                syslog(priority, "[part %d] %.256s", part, &buffer[i]);
                ++part;
                i += 256;
            }

            syslog(priority, "[end of long message]");
        }
    }
    else
    {
        if(error_code == 0)
            fprintf(stderr, "Info: %s\n", buffer);
        else
            fprintf(stderr, "Error: %s\n", buffer);
    }
}

static enum MessageVerboseLevel map_syslog_prio_to_verbose_level(int priority)
{
    switch(priority)
    {
      case LOG_EMERG:
      case LOG_ALERT:
      case LOG_CRIT:
        return MESSAGE_LEVEL_QUIET;

      case LOG_ERR:
      case LOG_WARNING:
        return MESSAGE_LEVEL_IMPORTANT;

      case LOG_NOTICE:
        return MESSAGE_LEVEL_NORMAL;

      case LOG_INFO:
        return MESSAGE_LEVEL_DIAG;

      case LOG_DEBUG:
        return MESSAGE_LEVEL_DEBUG;
    }

    return MESSAGE_LEVEL_IMPOSSIBLE;
}

void msg_error(int error_code, int priority, const char *error_format, ...)
{
    va_list va;

    va_start(va, error_format);
    show_message(map_syslog_prio_to_verbose_level(priority),
                 error_code, priority, error_format, va);
    va_end(va);
}

void msg_info(const char *format_string, ...)
{
    va_list va;

    va_start(va, format_string);
    show_message(MESSAGE_LEVEL_NORMAL, 0, LOG_INFO, format_string, va);
    va_end(va);
}

void msg_vinfo(enum MessageVerboseLevel level, const char *format_string, ...)
{
    if(level < MESSAGE_LEVEL_INFO_MIN || level > MESSAGE_LEVEL_INFO_MAX)
        return;

    va_list va;

    va_start(va, format_string);
    show_message(level, 0, LOG_INFO, format_string, va);
    va_end(va);
}

int msg_out_of_memory(const char *what)
{
    msg_error(ENOMEM, LOG_EMERG, "Failed allocating memory for %s", what);
    return -1;
}
