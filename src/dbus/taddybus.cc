/*
 * Copyright (C) 2022, 2024  T+A elektroakustik GmbH & Co. KG
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

#include "taddybus.hh"
#include "messages.h"
#include "gerrorwrapper.hh"

void TDBus::log_connect_proxy_bug(const char *object_path, const char *name)
{
    MSG_BUG("Cannot create proxy for D-Bus object %s at %s while busy",
            object_path, name);
}

void TDBus::log_method_done_exception_bug(const char *what)
{
    MSG_BUG("Exception thrown by method done handler: %s\n"
            "Method result is lost", what);
}

void TDBus::log_async_user_data_ptr(bool should_be_null, const char *fn)
{
    MSG_BUG("AsyncCall user data is %snullptr, but should %sbe (%s())",
            should_be_null ? "not " : "", should_be_null ? "" : "not ", fn);
}

void TDBus::log_bad_async_user_data_ptr()
{
    MSG_BUG("AsyncCall user data pointer does not match passed user data");
}

void TDBus::log_async_result_leak()
{
    MSG_BUG("Leaking async D-Bus result");
}

void TDBus::log_proxy_not_connected(const char *method_name)
{
    msg_error(0, LOG_NOTICE,
              "Failed calling D-Bus method %s: not connected", method_name);
}

TDBus::Bus::Bus(const char *object_name, Type t):
    object_name_(object_name),
    bus_type_(t),
    on_connect_(nullptr),
    on_name_acquired_(nullptr),
    on_name_lost_(nullptr),
    owner_id_(0)
{}

TDBus::Bus::~Bus()
{
    if(owner_id_ == 0)
        return;

    g_bus_unown_name(owner_id_);

    on_connect_ = nullptr;
    on_name_acquired_ = nullptr;
    on_name_lost_ = nullptr;
    owner_id_ = 0;
}

bool TDBus::Bus::connect(std::function<void(GDBusConnection *)> &&on_connect,
                         std::function<void(GDBusConnection *)> &&on_name_acquired,
                         std::function<void(GDBusConnection *)> &&on_name_lost)
{
    if(owner_id_ != 0)
        g_bus_unown_name(owner_id_);

    on_connect_ = std::move(on_connect);
    on_name_acquired_ = std::move(on_name_acquired);
    on_name_lost_ = std::move(on_name_lost);

    owner_id_ =
        g_bus_own_name(bus_type_ == Type::SESSION
                       ? G_BUS_TYPE_SESSION
                       : G_BUS_TYPE_SYSTEM,
                       object_name_.c_str(), G_BUS_NAME_OWNER_FLAGS_NONE,
                       bus_acquired, name_acquired, name_lost,
                       this, nullptr);

    if(owner_id_ != 0)
        return true;

    msg_error(0, LOG_ERR,
              "Failed owning D-Bus name \"%s\" (%s)",
              object_name_.c_str(),
              bus_type_ == Type::SESSION ? "session" : "system");

    on_connect_ = nullptr;
    on_name_acquired_ = nullptr;
    on_name_lost_ = nullptr;

    return false;
}

void TDBus::Bus::bus_acquired(GDBusConnection *connection,
                              const gchar *name, gpointer user_data)
{
    auto &bus(*static_cast<Bus *>(user_data));

    if(bus.on_connect_ != nullptr)
        bus.on_connect_(connection);

    for(const auto &i : bus.interfaces_)
        i->export_interface(connection);

    for(auto &w : bus.watchers_)
        w.start(connection);
}

void TDBus::Bus::name_acquired(GDBusConnection *connection,
                               const gchar *name, gpointer user_data)
{
    auto &bus(*static_cast<Bus *>(user_data));

    if(bus.on_name_acquired_ != nullptr)
        bus.on_name_acquired_(connection);
}

void TDBus::Bus::name_lost(GDBusConnection *connection,
                           const gchar *name, gpointer user_data)
{
    auto &bus(*static_cast<Bus *>(user_data));

    if(bus.on_name_lost_ != nullptr)
        bus.on_name_lost_(connection);
}

void TDBus::IfaceBase::method_fail(GDBusMethodInvocation *invocation,
                                   const char *message, ...)
{
    va_list va;

    va_start(va, message);
    g_dbus_method_invocation_return_error_valist(invocation, G_DBUS_ERROR,
                                                 G_DBUS_ERROR_INVALID_ARGS,
                                                 message, va);
    va_end(va);
}

void TDBus::PeerWatcher::appeared(GDBusConnection *connection, const gchar *name,
                                  const gchar *name_owner, gpointer user_data)
{
    auto &watcher(*static_cast<TDBus::PeerWatcher *>(user_data));

    if(watcher.name_appeared_ != nullptr)
        watcher.name_appeared_(connection, name);
}

void TDBus::PeerWatcher::vanished(GDBusConnection *connection, const gchar *name,
                                  gpointer user_data)
{
    auto &watcher(*static_cast<TDBus::PeerWatcher *>(user_data));

    if(watcher.name_vanished_ != nullptr)
        watcher.name_vanished_(connection, name);
}

void TDBus::PeerWatcher::start(GDBusConnection *connection)
{
    stop();
    watcher_id_ = g_bus_watch_name_on_connection(connection, name_.c_str(),
                                                 G_BUS_NAME_WATCHER_FLAGS_NONE,
                                                 appeared, vanished,
                                                 this, nullptr);
}

void TDBus::PeerWatcher::stop()
{
    if(watcher_id_ == 0)
        return;

    g_bus_unwatch_name(watcher_id_);
    watcher_id_ = 0;
}
