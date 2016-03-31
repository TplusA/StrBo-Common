/*
 * Copyright (C) 2016  T+A elektroakustik GmbH & Co. KG
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
