/*
 * Copyright (C) 2018, 2019, 2022, 2023  T+A elektroakustik GmbH & Co. KG
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

#ifndef MOCK_MESSAGES_HH
#define MOCK_MESSAGES_HH

#include "messages.h"
#include "mock_expectation.hh"

namespace MockMessages
{

/*! Base class for MockMessages expectations. */
class Expectation: public MockExpectationBase
{
  public:
    Expectation(std::string &&name): MockExpectationBase(std::move(name)) {}
    virtual ~Expectation() {}
};

class Mock: public MockBase
{
  public:
    bool ignore_all_;

  private:
    enum MessageVerboseLevel ignore_message_level_;
    enum MessageVerboseLevel mock_level_;

  public:
    Mock(const Mock &) = delete;
    Mock &operator=(const Mock &) = delete;

    explicit Mock(std::shared_ptr<MockExpectationSequence> eseq = nullptr):
        MockBase("MockMessages", eseq),
        ignore_all_(false),
        ignore_message_level_(MESSAGE_LEVEL_IMPOSSIBLE),
        mock_level_(MESSAGE_LEVEL_NORMAL)
    {}

    ~Mock() {}

    void expect(std::unique_ptr<Expectation> expectation)
    {
        add(std::move(expectation));
    }

    void expect(Expectation *expectation)
    {
        add(std::unique_ptr<Expectation>(expectation));
    }

    template <typename T, typename ... Args>
    auto &expect(Args ... args)
    {
        static_assert(std::is_base_of_v<Expectation, T> == true);
        return *static_cast<T *>(add(std::make_unique<T>(std::forward<Args>(args)...)));
    }

    template <typename T>
    void ignore(std::unique_ptr<Expectation> default_result)
    {
        ignore<T>(std::move(default_result));
    }

    template <typename T>
    void ignore(Expectation *default_result)
    {
        ignore<T>(std::unique_ptr<Expectation>(default_result));
    }

    void ignore_messages_above(enum MessageVerboseLevel level);
    void ignore_messages_with_level_or_above(enum MessageVerboseLevel level);
    bool is_level_ignored(enum MessageVerboseLevel level) const;
    void set_verbose_level(enum MessageVerboseLevel level);
    enum MessageVerboseLevel get_verbose_level() const;
};


class MsgIsVerbose: public Expectation
{
  private:
    const bool retval_;
    const MessageVerboseLevel level_;

  public:
    explicit MsgIsVerbose(bool retval, MessageVerboseLevel level):
        Expectation("MsgIsVerbose"),
        retval_(retval),
        level_(level)
    {}

    virtual ~MsgIsVerbose() {}

    bool check(MessageVerboseLevel level) const
    {
        CHECK(level_ == level);
        return retval_;
    }

    static auto make_from_check_parameters(MessageVerboseLevel level)
    {
        return std::make_unique<MsgIsVerbose>(false, level);
    }
};

class Message: public Expectation
{
  public:
    const std::string msg_;
    const std::string msg_end_;
    const bool is_format_string_;
    const bool is_complete_string_;
    const MessageVerboseLevel level_;

  protected:
    explicit Message(std::string &&name, std::string &&msg,
                     bool is_format_string, MessageVerboseLevel level):
        Expectation(std::move(name)),
        msg_(std::move(msg)),
        is_format_string_(is_format_string),
        is_complete_string_(true),
        level_(level)
    {}

    explicit Message(std::string &&name, std::string &&prefix,
                     std::string &&suffix,
                     bool is_format_string, MessageVerboseLevel level):
        Expectation(std::move(name)),
        msg_(std::move(prefix)),
        msg_end_(std::move(suffix)),
        is_format_string_(is_format_string),
        is_complete_string_(false),
        level_(level)
    {}

    void check_generic(MessageVerboseLevel level, const char *format_string,
                       va_list va, int error_code) const;

  public:
    std::string get_details() const final override
    {
        return "\"" + msg_ + (is_complete_string_ ? msg_end_ : "") + "\"";
    }
};

class MsgVinfo: public Message
{
  public:
    explicit MsgVinfo(MessageVerboseLevel level,
                      const char *msg, bool is_format_string,
                      std::string &&name = "MsgVinfo"):
        Message(std::move(name), msg, is_format_string, level)
    {}

    explicit MsgVinfo(MessageVerboseLevel level, const char *prefix,
                      const char *suffix, bool is_format_string,
                      std::string &&name = "MsgVinfo"):
        Message(std::move(name), prefix, suffix, is_format_string, level)
    {}

    void check(MessageVerboseLevel level, const char *format_string, va_list va) const
    {
        check_generic(level, format_string, va, 0);
    }

    static auto make_from_check_parameters(MessageVerboseLevel level,
                                           const char *format_string, va_list va)
    {
        return std::make_unique<MsgVinfo>(level, format_string, false);
    }
};

class MsgInfo: public MsgVinfo
{
  public:
    explicit MsgInfo(const char *msg, bool is_format_string):
        MsgVinfo(MESSAGE_LEVEL_NORMAL, msg, is_format_string, "MsgInfo")
    {}

    explicit MsgInfo(const char *prefix, const char *suffix,
                     bool is_format_string):
        MsgVinfo(MESSAGE_LEVEL_NORMAL, prefix, suffix, is_format_string, "MsgInfo")
    {}

    void check(const char *format_string, va_list va) const
    {
        check_generic(MESSAGE_LEVEL_NORMAL, format_string, va, 0);
    }

    static auto make_from_check_parameters(const char *format_string, va_list va)
    {
        return std::make_unique<MsgInfo>(format_string, false);
    }
};

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

class MsgError: public Message
{
  public:
    const int error_code_;
    const int priority_;

    explicit MsgError(int error_code, int priority,
                      const char *msg, bool is_format_string,
                      std::string &&name = "MsgError"):
        Message(std::move(name), msg, is_format_string,
                map_syslog_prio_to_verbose_level(priority)),
        error_code_(error_code),
        priority_(priority)
    {}

    explicit MsgError(int error_code, int priority,
                      const char *prefix, const char *suffix,
                      bool is_format_string,
                      std::string &&name = "MsgError"):
        Message(std::move(name), prefix, suffix, is_format_string,
                map_syslog_prio_to_verbose_level(priority)),
        error_code_(error_code),
        priority_(priority)
    {}

    void check(int error_code, int priority,
               const char *format_string, va_list va) const
    {
        CHECK(error_code == error_code_);
        CHECK(priority == priority_);
        check_generic(map_syslog_prio_to_verbose_level(priority),
                      format_string, va, error_code);
    }

    static auto make_from_check_parameters(int error_code, int priority,
                                           const char *format_string, va_list va)
    {
        return std::make_unique<MsgError>(error_code, priority, format_string, true);
    }
};

class MsgOOM: public MsgError
{
  public:
    explicit MsgOOM(const char *msg):
        MsgError(ENOMEM, LOG_EMERG, msg, false, "MsgOOM")
    {}

    void check(const char *msg) const
    {
        CHECK(msg == msg_);
    }

    static auto make_from_check_parameters(const char *msg)
    {
        return std::make_unique<MsgOOM>(msg);
    }
};

/*!
 * One messages mock to rule them all...
 *
 * This is necessary because there are only free C function tested by this
 * mock, and there is no simple way for these functions to pick a suitable mock
 * object from a set of those. It should be possible to use TLD for this
 * purpose, but for now let's go with a simpler version.
 *
 * \note Having this singleton around means that running tests in multiple
 *     threads in NOT possible.
 */
extern Mock *singleton;

}

#endif /* !MOCK_MESSAGES_HH */
