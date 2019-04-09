/*
 * Copyright (C) 2016, 2017, 2019  T+A elektroakustik GmbH & Co. KG
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

#ifndef MESSAGES_DBUS_H
#define MESSAGES_DBUS_H

#include "debug_dbus.h"

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * Handler for \c de.tahifi.Debug.Logging.DebugLevel method.
 */
gboolean msg_dbus_handle_debug_level(tdbusdebugLogging *object,
                                     GDBusMethodInvocation *invocation,
                                     const gchar *arg_new_level,
                                     void *user_data);
/*!
 * Handler for \c de.tahifi.Debug.LoggingConfig.GlobalDebugLevelChanged signal
 */
void msg_dbus_handle_global_debug_level_changed(GDBusProxy *proxy,
                                                const gchar *sender_name,
                                                const gchar *signal_name,
                                                GVariant *parameters,
                                                gpointer user_data);

#ifdef __cplusplus
}
#endif

#endif /* !MESSAGES_DBUS_H */
