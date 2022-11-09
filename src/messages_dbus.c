/*
 * Copyright (C) 2016, 2019, 2022  T+A elektroakustik GmbH & Co. KG
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

#include <stdbool.h>
#include <string.h>
#include <errno.h>

#include "messages_dbus.h"
#include "messages.h"

static enum MessageVerboseLevel do_set_debug_level(const char *new_level_name)
{
    static enum MessageVerboseLevel default_level = MESSAGE_LEVEL_IMPOSSIBLE;

    if(default_level == MESSAGE_LEVEL_IMPOSSIBLE)
        default_level = msg_get_verbose_level();

    enum MessageVerboseLevel old_level = msg_get_verbose_level();
    enum MessageVerboseLevel new_level;

    if(new_level_name == NULL || new_level_name[0] == '\0')
    {
        new_level = old_level;
        new_level_name = msg_verbose_level_to_level_name(new_level);
    }
    else if(strcmp(new_level_name, "default") == 0)
    {
        new_level = default_level;
        new_level_name = msg_verbose_level_to_level_name(new_level);
    }
    else
    {
        new_level = msg_verbose_level_name_to_level(new_level_name);

        if(new_level == MESSAGE_LEVEL_IMPOSSIBLE)
            old_level = MESSAGE_LEVEL_IMPOSSIBLE;
    }

    if(new_level != old_level)
    {
        msg_vinfo(MESSAGE_LEVEL_INFO_MIN,
                  "Set debug level \"%s\"", new_level_name);
        msg_set_verbose_level(new_level);
    }
    else if(old_level == MESSAGE_LEVEL_IMPOSSIBLE)
        msg_error(0, LOG_ERR, "Log level \"%s\" invalid", new_level_name);

    return old_level;
}

gboolean msg_dbus_handle_debug_level(tdbusdebugLogging *object,
                                     GDBusMethodInvocation *invocation,
                                     const gchar *arg_new_level,
                                     void *user_data)
{
    const enum MessageVerboseLevel prev = do_set_debug_level(arg_new_level);
    const char *prev_name = msg_verbose_level_to_level_name(prev);

    if(prev_name == NULL)
        prev_name = "";

    tdbus_debug_logging_complete_debug_level(object, invocation, prev_name);

    return TRUE;
}

void msg_dbus_handle_global_debug_level_changed(GDBusProxy *proxy,
                                                const gchar *sender_name,
                                                const gchar *signal_name,
                                                GVariant *parameters,
                                                gpointer user_data)
{
    static const char iface_name[] = "de.tahifi.Debug.LoggingConfig";

    if(strcmp(signal_name, "GlobalDebugLevelChanged") == 0)
    {
        msg_log_assert(g_variant_type_is_tuple(g_variant_get_type(parameters)));
        msg_log_assert(g_variant_n_children(parameters) == 1);

        const gchar *new_level_name;
        g_variant_get(parameters, "(&s)", &new_level_name);

        do_set_debug_level(new_level_name);
    }
    else
        msg_error(ENOSYS, LOG_NOTICE, "Got unknown signal %s.%s from %s",
                  iface_name, signal_name, sender_name);
}
