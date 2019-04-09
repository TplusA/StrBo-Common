/*
 * Copyright (C) 2016, 2019  T+A elektroakustik GmbH & Co. KG
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

#ifndef MESSAGES_GLIB_H
#define MESSAGES_GLIB_H

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * Redirect all GLib log messages to our own logging API.
 */
void msg_enable_glib_message_redirection(void);

#ifdef __cplusplus
}
#endif

#endif /* !MESSAGES_GLIB_H */
