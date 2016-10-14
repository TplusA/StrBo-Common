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

#include <cppcutter.h>

#include <vector>
#include <string>
#include <memory>
#include <cstdarg>
#include <cstring>
#include <cstdio>

#include "mock_messages.hh"

class MockMessages::Expectation
{
  public:
    Expectation(const Expectation &) = delete;
    Expectation(Expectation &&) = default;
    Expectation &operator=(const Expectation &) = delete;

    enum MessageVerboseLevel level_;
    const int error_code_;
    const int priority_;

    const bool is_message_function_;
    const bool is_format_string_;
    const bool is_complete_string_;
    const std::string string_;
    const std::string suffix_;
    const int ret_bool_;

    explicit Expectation(enum MessageVerboseLevel level,
                         int error_code, int priority,
                         const char *string, bool is_format_string):
        level_(level),
        error_code_(error_code),
        priority_(priority),
        is_message_function_(true),
        is_format_string_(is_format_string),
        is_complete_string_(true),
        string_(string),
        ret_bool_(false)
    {}

    explicit Expectation(enum MessageVerboseLevel level,
                         int error_code, int priority,
                         const char *prefix, const char *suffix,
                         bool is_format_string):
        level_(level),
        error_code_(error_code),
        priority_(priority),
        is_message_function_(true),
        is_format_string_(is_format_string),
        is_complete_string_(false),
        string_(prefix),
        suffix_(suffix),
        ret_bool_(false)
    {}

    explicit Expectation(bool retval, enum MessageVerboseLevel level):
        level_(level),
        error_code_(-60),
        priority_(123),
        is_message_function_(false),
        is_format_string_(false),
        is_complete_string_(false),
        ret_bool_(retval)
    {}
};


MockMessages::MockMessages():
    ignore_message_level_(MESSAGE_LEVEL_IMPOSSIBLE),
    ignore_all_(false)
{
    expectations_ = new MockExpectations();
}

MockMessages::~MockMessages()
{
    delete expectations_;
}

void MockMessages::init()
{
    cppcut_assert_not_null(expectations_);
    expectations_->init();
}

void MockMessages::check() const
{
    cppcut_assert_not_null(expectations_);
    expectations_->check();
}

void MockMessages::ignore_messages_above(enum MessageVerboseLevel level)
{
    if(level >= MESSAGE_LEVEL_MAX)
        ignore_message_level_ = MESSAGE_LEVEL_IMPOSSIBLE;
    else
        ignore_message_level_ = static_cast<enum MessageVerboseLevel>(level + 1);
}

void MockMessages::ignore_messages_with_level_or_above(enum MessageVerboseLevel level)
{
    ignore_message_level_ = level;
}

bool MockMessages::is_level_ignored(enum MessageVerboseLevel level) const
{
    return ignore_message_level_ != MESSAGE_LEVEL_IMPOSSIBLE &&
           level >= ignore_message_level_;
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

void MockMessages::expect_msg_is_verbose(bool retval,
                                         enum MessageVerboseLevel level)
{
    expectations_->add(Expectation(retval, level));
}

void MockMessages::expect_msg_error_formatted(int error_code, int priority,
                                              const char *string)
{
    expectations_->add(Expectation(map_syslog_prio_to_verbose_level(priority),
                                   error_code, priority, string, false));
}

void MockMessages::expect_msg_error_formatted(int error_code, int priority,
                                              const char *prefix, const char *suffix)
{
    expectations_->add(Expectation(map_syslog_prio_to_verbose_level(priority),
                                   error_code, priority, prefix, suffix, false));
}

void MockMessages::expect_msg_error(int error_code, int priority,
                                    const char *string)
{
    expectations_->add(Expectation(map_syslog_prio_to_verbose_level(priority),
                                   error_code, priority, string, true));
}

void MockMessages::expect_msg_info_formatted(const char *string)
{
    expectations_->add(Expectation(MESSAGE_LEVEL_NORMAL, 0, LOG_INFO, string, false));
}

void MockMessages::expect_msg_info(const char *string)
{
    expectations_->add(Expectation(MESSAGE_LEVEL_NORMAL, 0, LOG_INFO, string, true));
}

void MockMessages::expect_msg_vinfo_formatted(enum MessageVerboseLevel level,
                                              const char *string)
{
    expectations_->add(Expectation(level, 0, LOG_INFO, string, false));
}

void MockMessages::expect_msg_vinfo(enum MessageVerboseLevel level,
                                    const char *string)
{
    expectations_->add(Expectation(level, 0, LOG_INFO, string, true));
}

MockMessages *mock_messages_singleton = nullptr;

static void check_prefix_and_suffix(const std::string &expected_prefix,
                                    const std::string &expected_suffix,
                                    const char *string)
{
    size_t string_length = strlen(string);

    cppcut_assert_operator(expected_prefix.length() + expected_suffix.length(), <=, string_length);
    cppcut_assert_equal(expected_prefix, std::string(string, expected_prefix.length()));
    cppcut_assert_equal(expected_suffix, std::string(string + string_length - expected_suffix.length(),
                                                     expected_suffix.length()));
}

static void check_message_expectation(enum MessageVerboseLevel level,
                                      int error_code, int priority,
                                      const char *format_string, va_list va)
{
    const auto &expect(mock_messages_singleton->expectations_->get_next_expectation(format_string, va));

    cut_assert_true(expect.is_message_function_);

    if(expect.is_format_string_)
    {
        if(expect.is_complete_string_)
            cppcut_assert_equal(expect.string_, std::string(format_string));
        else
            check_prefix_and_suffix(expect.string_, expect.suffix_, format_string);
    }
    else
    {
        char buffer[512];
        size_t len = vsnprintf(buffer, sizeof(buffer), format_string, va);

        if(error_code != 0 && len < sizeof(buffer))
            snprintf(buffer + len, sizeof(buffer) - len,
                     " (%s)", strerror(error_code));

        if(expect.is_complete_string_)
            cppcut_assert_equal(expect.string_, std::string(buffer));
        else
            check_prefix_and_suffix(expect.string_, expect.suffix_, buffer);
    }

    if(expect.level_ != level || expect.error_code_ != error_code ||
       expect.priority_ != priority)
    {
        /* the seemingly useless/redundant check above works around a bug in
         * Cutter that causes a memory read from a free'd location in the
         * checks below, even in case of success */
        static const char error_prefix[] = "For expected message \"";
        static const char error_suffix[] = "\":";

        cppcut_assert_equal(expect.level_, level,
                            cppcut_message() << error_prefix << expect.string_.c_str() << error_suffix);
        cppcut_assert_equal(expect.error_code_, error_code,
                            cppcut_message() << error_prefix << expect.string_.c_str() << error_suffix);
        cppcut_assert_equal(expect.priority_, priority,
                            cppcut_message() << error_prefix << expect.string_.c_str() << error_suffix);
    }
}

void msg_enable_syslog(bool enable_syslog) {}

void msg_set_verbose_level(enum MessageVerboseLevel level)
{
    cppcut_assert_operator(static_cast<int>(MESSAGE_LEVEL_MIN), <=, static_cast<int>(level));
    cppcut_assert_operator(static_cast<int>(MESSAGE_LEVEL_MAX), >=, static_cast<int>(level));
}

bool msg_is_verbose(enum MessageVerboseLevel level)
{
    const auto &expect(mock_messages_singleton->expectations_->get_next_expectation(__func__));

    cut_assert_true(expect.is_message_function_);
    cppcut_assert_equal(expect.level_, level);

    return expect.ret_bool_;
}

void msg_error(int error_code, int priority, const char *error_format, ...)
{
    if(mock_messages_singleton->ignore_all_)
        return;

    const enum MessageVerboseLevel level =
        map_syslog_prio_to_verbose_level(priority);

    if(mock_messages_singleton->is_level_ignored(level))
        return;

    va_list va;

    va_start(va, error_format);
    check_message_expectation(map_syslog_prio_to_verbose_level(priority),
                              error_code, priority, error_format, va);
    va_end(va);
}

void msg_info(const char *format_string, ...)
{
    if(mock_messages_singleton->ignore_all_)
        return;

    if(mock_messages_singleton->is_level_ignored(MESSAGE_LEVEL_NORMAL))
        return;

    va_list va;

    va_start(va, format_string);
    check_message_expectation(MESSAGE_LEVEL_NORMAL, 0, LOG_INFO, format_string, va);
    va_end(va);
}

void msg_vinfo(enum MessageVerboseLevel level, const char *format_string, ...)
{
    if(mock_messages_singleton->ignore_all_)
        return;

    if(mock_messages_singleton->is_level_ignored(level))
        return;

    va_list va;

    va_start(va, format_string);
    check_message_expectation(level, 0, LOG_INFO, format_string, va);
    va_end(va);
}

int msg_out_of_memory(const char *what)
{
    if(mock_messages_singleton->ignore_all_)
        return -1;

    const auto &expect(mock_messages_singleton->expectations_->get_next_expectation(what));

    cppcut_assert_equal(expect.string_, std::string(what));
    cut_assert_false(expect.is_format_string_);
    cppcut_assert_equal(expect.error_code_, ENOMEM);
    cppcut_assert_equal(expect.priority_, LOG_EMERG);

    return -1;
}
