/*
 * Copyright (C) 2016, 2019, 2020  T+A elektroakustik GmbH & Co. KG
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

#include <glib.h>

#include "messages_glib.h"
#include "messages.h"

#if MSG_ACTION_ON_GLIB_FAILURE == 1
#include "backtrace.h"
#endif

static inline int glib_log_level_to_syslog_priority(GLogLevelFlags log_level)
{
    switch((GLogLevelFlags)(log_level & G_LOG_LEVEL_MASK))
    {
      case G_LOG_LEVEL_ERROR:
        return LOG_EMERG;

      case G_LOG_LEVEL_CRITICAL:
        return LOG_CRIT;

      case G_LOG_LEVEL_WARNING:
        return LOG_WARNING;

      case G_LOG_LEVEL_MESSAGE:
        return LOG_NOTICE;

      case G_LOG_LEVEL_INFO:
        return LOG_INFO;

      case G_LOG_LEVEL_DEBUG:
        return LOG_DEBUG;

      /*
       * This is exactly the reason why you don't use enums for bitmasks. This
       * code handles all the irrelevant GLib-internal stuff here, implicitly
       * or explicitly, whether you like it or not. Using a default case
       * instead of listing the irrelevant cases would NOT change the situation
       * at all.
       *
       * GLib, you have come a long way to become the huge pile of shit that
       * you are now.
       */
      case G_LOG_LEVEL_MASK:
      case G_LOG_FLAG_RECURSION:
      case G_LOG_FLAG_FATAL:
        break;
    }

    return LOG_ALERT;
}

static void log_them_all(const gchar *log_domain, GLogLevelFlags log_level,
                         const gchar *message, gpointer user_data)
{
    const int syslog_prio = glib_log_level_to_syslog_priority(log_level);

    msg_error(0, syslog_prio, "From GLib (%s) %s", log_domain, message);

#if MSG_ACTION_ON_GLIB_FAILURE == 1
    if(msg_is_verbose(syslog_prio))
        backtrace_log(0, "GLib context");
#elif MSG_ACTION_ON_GLIB_FAILURE == 2
    os_abort();
#endif /* MSG_ACTION_ON_GLIB_FAILURE */
}

void msg_enable_glib_message_redirection(void)
{
    g_log_set_default_handler(log_them_all, NULL);
    g_log_set_handler(NULL,
                      G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION,
                      log_them_all, NULL);
}
