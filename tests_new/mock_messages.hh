/*
 * Copyright (C) 2018  T+A elektroakustik GmbH & Co. KG
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

#ifndef MOCK_MESSAGES_HH
#define MOCK_MESSAGES_HH

#include "messages.h"
#include "mock_expectation.hh"

namespace MockMessages
{

/*! Base class for expectations. */
class Expectation
{
  public:
    Expectation(const Expectation &) = delete;
    Expectation(Expectation &&) = default;
    Expectation &operator=(const Expectation &) = delete;
    Expectation &operator=(Expectation &&) = default;
    Expectation() {}
    virtual ~Expectation() {}
};

class Mock
{
  public:
    bool ignore_all_;

  private:
    enum MessageVerboseLevel ignore_message_level_;
    enum MessageVerboseLevel mock_level_;
    MockExpectationsTemplate<Expectation> expectations_;

  public:
    Mock(const Mock &) = delete;
    Mock &operator=(const Mock &) = delete;

    explicit Mock():
        ignore_all_(false),
        ignore_message_level_(MESSAGE_LEVEL_IMPOSSIBLE),
        mock_level_(MESSAGE_LEVEL_NORMAL),
        expectations_("MockMessages")
    {}

    ~Mock() {}

    void expect(std::unique_ptr<Expectation> expectation)
    {
        expectations_.add(std::move(expectation));
    }

    void expect(Expectation *expectation)
    {
        expectations_.add(std::unique_ptr<Expectation>(expectation));
    }

    void done() const { expectations_.done(); }

    template <typename T, typename ... Args>
    typename T::CheckReturnType check_next(Args ... args)
    {
        return expectations_.check_and_advance<T>(args...);
    }

    template <typename T>
    const T &next(const char *caller)
    {
        return expectations_.next<T>(caller);
    }

    template <typename T>
    const T &next(const char *format_string, va_list va)
    {
        return expectations_.next<T>(format_string, va);
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
        retval_(retval),
        level_(level)
    {}

    virtual ~MsgIsVerbose() {}

    using CheckReturnType = bool;

    CheckReturnType check(MessageVerboseLevel level) const
    {
        CHECK(level_ == level);
        return retval_;
    }
};

class Message: public Expectation
{
  public:
    const std::string msg_;
    const std::string msg_end_;
    const bool is_format_string_;
    const bool is_complete_string_;

  protected:
    explicit Message(std::string &&msg, bool is_format_string):
        msg_(std::move(msg)),
        is_format_string_(is_format_string),
        is_complete_string_(true)
    {}

    explicit Message(std::string &&prefix, std::string &&suffix,
                     bool is_format_string):
        msg_(std::move(prefix)),
        msg_end_(std::move(suffix)),
        is_format_string_(is_format_string),
        is_complete_string_(false)
    {}

    void check_generic(const char *format_string, va_list va, int error_code) const;
};

class MsgInfo: public Message
{
  public:
    explicit MsgInfo(const char *msg, bool is_format_string):
        Message(msg, is_format_string)
    {}

    explicit MsgInfo(const char *prefix, const char *suffix,
                     bool is_format_string):
        Message(prefix, suffix, is_format_string)
    {}

    using CheckReturnType = void;

    CheckReturnType check(const char *format_string, va_list va) const
    {
        check_generic(format_string, va, 0);
    }
};

class MsgError: public Message
{
  public:
    const int error_code_;
    const int priority_;

    explicit MsgError(int error_code, int priority,
                      const char *msg, bool is_format_string):
        Message(msg, is_format_string),
        error_code_(error_code),
        priority_(priority)
    {}

    explicit MsgError(int error_code, int priority,
                      const char *prefix, const char *suffix,
                      bool is_format_string):
        Message(prefix, suffix, is_format_string),
        error_code_(error_code),
        priority_(priority)
    {}

    using CheckReturnType = void;

    CheckReturnType check(int error_code, int priority,
                          const char *format_string, va_list va) const
    {
        CHECK(error_code == error_code_);
        CHECK(priority == priority_);
        check_generic(format_string, va, error_code);
    }
};

class MsgOOM: public MsgError
{
  public:
    explicit MsgOOM(const char *msg):
        MsgError(ENOMEM, LOG_EMERG, msg, false)
    {}

    using CheckReturnType = void;

    CheckReturnType check(const char *msg) const
    {
        CHECK(msg == msg_);
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
