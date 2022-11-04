/*
 * Copyright (C) 2018, 2019, 2022  T+A elektroakustik GmbH & Co. KG
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

#include <doctest.h>

#include "mock_messages.hh"

#include <array>
#include <iostream>
#include <algorithm>
#include <cstring>

void MockMessages::Mock::ignore_messages_above(enum MessageVerboseLevel level)
{
    if(level >= MESSAGE_LEVEL_MAX)
        ignore_message_level_ = MESSAGE_LEVEL_IMPOSSIBLE;
    else
        ignore_message_level_ = static_cast<enum MessageVerboseLevel>(level + 1);
}

void MockMessages::Mock::ignore_messages_with_level_or_above(enum MessageVerboseLevel level)
{
    ignore_message_level_ = level;
}

bool MockMessages::Mock::is_level_ignored(enum MessageVerboseLevel level) const
{
    return ignore_message_level_ != MESSAGE_LEVEL_IMPOSSIBLE &&
           level >= ignore_message_level_;
}

void MockMessages::Mock::set_verbose_level(enum MessageVerboseLevel level)
{
    REQUIRE(static_cast<int>(level) <= static_cast<int>(MESSAGE_LEVEL_MIN));
    REQUIRE(static_cast<int>(level) >= static_cast<int>(MESSAGE_LEVEL_MAX));

    mock_level_ = level;
}

enum MessageVerboseLevel MockMessages::Mock::get_verbose_level() const
{
    return mock_level_;
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

MockMessages::Mock *MockMessages::singleton = nullptr;


static void check_prefix_and_suffix(const std::string &expected_prefix,
                                    const std::string &expected_suffix,
                                    const std::string &str)
{
    CHECK(str.length() >= expected_prefix.length() + expected_suffix.length());
    CHECK(str.substr(0, expected_prefix.length()) == expected_prefix);
    CHECK(str.substr(str.length() - expected_suffix.length()) == expected_suffix);
}

void MockMessages::Message::check_generic(MessageVerboseLevel level,
                                          const char *format_string, va_list va,
                                          int error_code) const
{
    if(is_format_string_)
    {
        if(is_complete_string_)
            CHECK(format_string == msg_);
        else
            check_prefix_and_suffix(msg_, msg_end_, format_string);
    }
    else
    {
        char buffer[512];
        size_t len = std::vsnprintf(buffer, sizeof(buffer), format_string, va);

        if(error_code != 0 && len < sizeof(buffer))
            std::snprintf(buffer + len, sizeof(buffer) - len,
                          " (%s)", strerror(error_code));

        if(is_complete_string_)
            CHECK(buffer == msg_);
        else
            check_prefix_and_suffix(msg_, msg_end_, buffer);
    }

    CHECK(level == level_);
}

void msg_enable_syslog(bool enable_syslog) {}

void msg_set_verbose_level(enum MessageVerboseLevel level)
{
    REQUIRE(MockMessages::singleton != nullptr);
    MockMessages::singleton->set_verbose_level(level);
}

enum MessageVerboseLevel msg_get_verbose_level(void)
{
    REQUIRE(MockMessages::singleton != nullptr);
    return MockMessages::singleton->get_verbose_level();
}

bool msg_is_verbose(enum MessageVerboseLevel level)
{
    REQUIRE(MockMessages::singleton != nullptr);
    return MockMessages::singleton->check_next<MockMessages::MsgIsVerbose>(level);
}

/* order of strings must match the values listed in the #MessageVerboseLevel
 * enumeration */
static constexpr std::array<const char *, MESSAGE_LEVEL_MAX - MESSAGE_LEVEL_MIN + 1>
messages_verbosity_level_names =
{
    "quiet",
    "important",
    "normal",
    "diag",
    "debug",
    "trace",
};

enum MessageVerboseLevel msg_verbose_level_name_to_level(const char *name)
{
    const auto found(std::find_if(messages_verbosity_level_names.cbegin(),
                                  messages_verbosity_level_names.cend(),
                                  [name] (const char *level_name)
                                  {
                                      return strcmp(name, level_name) == 0;
                                  }));
    const auto idx(std::distance(messages_verbosity_level_names.begin(), found));

    if(idx >= 0 && static_cast<size_t>(idx) < messages_verbosity_level_names.size())
        return static_cast<MessageVerboseLevel>(MESSAGE_LEVEL_MIN + idx);
    else
        return MESSAGE_LEVEL_IMPOSSIBLE;
}

const char *msg_verbose_level_to_level_name(enum MessageVerboseLevel level)
{
    if(level >= MESSAGE_LEVEL_MIN && level <= MESSAGE_LEVEL_MAX)
        return messages_verbosity_level_names[level - MESSAGE_LEVEL_MIN];

    return nullptr;
}

void msg_error(int error_code, int priority, const char *error_format, ...)
{
    REQUIRE(MockMessages::singleton != nullptr);
    if(MockMessages::singleton->ignore_all_)
        return;

    const enum MessageVerboseLevel level =
        map_syslog_prio_to_verbose_level(priority);

    if(MockMessages::singleton->is_level_ignored(level))
        return;

    va_list va;
    va_start(va, error_format);

    try
    {
        MockMessages::singleton->check_next<MockMessages::MsgError>(
                error_code, priority, error_format, va);
    }
    catch(...)
    {
        va_end(va);
        MESSAGE("Failed in " << __PRETTY_FUNCTION__ << " for string \"" << error_format << "\"");

        va_list va2;
        va_start(va2, error_format);
        thread_local static char buffer[2048];
        size_t len = std::vsnprintf(buffer, sizeof(buffer), error_format, va2);
        if(error_code != 0 && len < sizeof(buffer))
            std::snprintf(buffer + len, sizeof(buffer) - len,
                          " (%s)", strerror(error_code));
        MESSAGE("Unexpected error message: \"" << buffer << "\"");
        va_end(va2);

        throw;
    }

    va_end(va);
}

void msg_info(const char *format_string, ...)
{
    REQUIRE(MockMessages::singleton != nullptr);
    if(MockMessages::singleton->ignore_all_)
        return;

    if(MockMessages::singleton->is_level_ignored(MESSAGE_LEVEL_NORMAL))
        return;

    va_list va;
    va_start(va, format_string);

    try
    {
        MockMessages::singleton->check_next<MockMessages::MsgInfo>(format_string, va);
    }
    catch(...)
    {
        va_end(va);
        MESSAGE("Failed in " << __PRETTY_FUNCTION__ << " for string \"" << format_string << "\"");

        va_list va2;
        va_start(va2, format_string);
        thread_local static char buffer[2048];
        std::vsnprintf(buffer, sizeof(buffer), format_string, va2);
        MESSAGE("Unexpected info message: \"" << buffer << "\"");
        va_end(va2);

        throw;
    }

    va_end(va);
}

void msg_vinfo(enum MessageVerboseLevel level, const char *format_string, ...)
{
    REQUIRE(MockMessages::singleton != nullptr);
    if(MockMessages::singleton->ignore_all_)
        return;

    if(MockMessages::singleton->is_level_ignored(level))
        return;

    va_list va;
    va_start(va, format_string);

    try
    {
        MockMessages::singleton->check_next<MockMessages::MsgVinfo>(level, format_string, va);
    }
    catch(...)
    {
        va_end(va);
        MESSAGE("Failed in " << __PRETTY_FUNCTION__ << " for string \"" << format_string << "\"");

        va_list va2;
        va_start(va2, format_string);
        thread_local static char buffer[2048];
        std::vsnprintf(buffer, sizeof(buffer), format_string, va2);
        MESSAGE("Unexpected info message: \"" << buffer << "\"");
        va_end(va2);

        throw;
    }

    va_end(va);
}

int msg_out_of_memory(const char *what)
{
    REQUIRE(MockMessages::singleton != nullptr);
    if(MockMessages::singleton->ignore_all_)
        return -1;

    try
    {
        MockMessages::singleton->check_next<MockMessages::MsgOOM>(what);
    }
    catch(...)
    {
        MESSAGE("Failed in " << __PRETTY_FUNCTION__ << " for string \"" << what << "\"");
        throw;
    }

    return -1;
}
