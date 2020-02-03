/*
 * Copyright (C) 2015, 2016, 2019, 2020  T+A elektroakustik GmbH & Co. KG
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

#if HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>

#include "messages.h"

static bool use_syslog;
static bool use_colors = true;
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

void msg_enable_color_console(bool enable_colors)
{
    use_colors = enable_colors;
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

const char *msg_verbose_level_to_level_name(enum MessageVerboseLevel level)
{
    if(level >= MESSAGE_LEVEL_MIN && level <= MESSAGE_LEVEL_MAX)
        return verbosity_level_names[level - MESSAGE_LEVEL_MIN];

    return NULL;
}

const char *const *msg_get_verbose_level_names(void)
{
    return verbosity_level_names;
}

static const char *generate_timestamp(void)
{
    _Thread_local static char tbuf[64];
    struct timespec ts;

    if(os_clock_gettime(CLOCK_REALTIME, &ts) < 0)
    {
        tbuf[0] = '\0';
        return tbuf;
    }

    struct tm t;
    const size_t len = localtime_r(&ts.tv_sec, &t) == NULL
        ? 0
        : strftime(tbuf, sizeof(tbuf), "%T", &t);

    if(len > 0)
        snprintf(tbuf + len, sizeof(tbuf) - len, ".%.9ld", ts.tv_nsec);
    else
        snprintf(tbuf, sizeof(tbuf),
                 "%lld.%.9ld", (long long)ts.tv_sec, ts.tv_nsec);

    return tbuf;
}

static void show_message(enum MessageVerboseLevel level, int error_code,
                         int priority, const char *format_string, va_list va)
{
    if(level > current_verbosity)
        return;

    _Thread_local static char buffer[2048];
    size_t len = vsnprintf(buffer, sizeof(buffer), format_string, va);

    if(error_code > 0 && len < sizeof(buffer))
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
    else if(!use_colors)
    {
        if(error_code == 0)
            fprintf(stderr, "%s - Info: %s\n", generate_timestamp(), buffer);
        else
            fprintf(stderr, "%s - Error: %s\n", generate_timestamp(), buffer);
    }
    else
    {
        enum Color
        {
            COLOR_OFF,
            COLOR_TIME,
            COLOR_INFO,
            COLOR_ERROR,
            COLOR_PRIO_TRACE,
            COLOR_PRIO_DEBUG,
            COLOR_PRIO_DIAG,
            COLOR_PRIO_INFO,
            COLOR_PRIO_NOTICE,
            COLOR_PRIO_WARNING,
            COLOR_PRIO_ERR,
            COLOR_PRIO_CRIT,
            COLOR_PRIO_ALERT,
            COLOR_PRIO_EMERG,

            LAST_COLOR = COLOR_PRIO_EMERG,
        };

        static const char *colors[LAST_COLOR + 1] =
        {
            [COLOR_OFF]          = "\x1b[0m",
            [COLOR_TIME]         = "\x1b[38;5;28m",
            [COLOR_INFO]         = "\x1b[38;5;2m",
            [COLOR_ERROR]        = "\x1b[38;5;160m",
            [COLOR_PRIO_TRACE]   = "\x1b[38;5;239m",
            [COLOR_PRIO_DEBUG]   = "\x1b[38;5;242m",
            [COLOR_PRIO_DIAG]    = "\x1b[38;5;245m",
            [COLOR_PRIO_INFO]    = "\x1b[38;5;7m",
            [COLOR_PRIO_NOTICE]  = "\x1b[38;5;45m",
            [COLOR_PRIO_WARNING] = "\x1b[38;5;11m",
            [COLOR_PRIO_ERR]     = "\x1b[38;5;202m",
            [COLOR_PRIO_CRIT]    = "\x1b[38;5;9m",
            [COLOR_PRIO_ALERT]   = "\x1b[38;5;1m",
            [COLOR_PRIO_EMERG]   = "\x1b[38;5;201m",
        };

        enum Color color = (enum Color)(COLOR_PRIO_EMERG - priority);
        if(priority == LOG_INFO)
        {
            if(level > MESSAGE_LEVEL_NORMAL)
                color -= level;
            else if(level < MESSAGE_LEVEL_NORMAL)
                color = COLOR_PRIO_NOTICE;
        }

        if(error_code == 0)
            fprintf(stderr, "%s%s -%s %sInfo:%s %s%s%s\n",
                    colors[COLOR_TIME], generate_timestamp(), colors[COLOR_OFF],
                    colors[COLOR_INFO], colors[COLOR_OFF],
                    colors[color], buffer, colors[COLOR_OFF]);
        else
            fprintf(stderr, "%s%s -%s %sError:%s %s%s%s\n",
                    colors[COLOR_TIME], generate_timestamp(), colors[COLOR_OFF],
                    colors[COLOR_ERROR], colors[COLOR_OFF],
                    colors[color], buffer, colors[COLOR_OFF]);
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
                 error_code != 0 ? error_code : INT_MIN,
                 priority, error_format, va);
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
