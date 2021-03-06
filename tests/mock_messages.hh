/*
 * Copyright (C) 2015, 2016, 2019  T+A elektroakustik GmbH & Co. KG
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

class MockMessages
{
  private:
    enum MessageVerboseLevel ignore_message_level_;
    enum MessageVerboseLevel mock_level_;

  public:
    MockMessages(const MockMessages &) = delete;
    MockMessages &operator=(const MockMessages &) = delete;

    class Expectation;
    typedef MockExpectationsTemplate<Expectation> MockExpectations;
    MockExpectations *expectations_;

    bool ignore_all_;

    explicit MockMessages();
    ~MockMessages();

    void init();
    void check() const;

    void ignore_messages_above(enum MessageVerboseLevel level);
    void ignore_messages_with_level_or_above(enum MessageVerboseLevel level);
    bool is_level_ignored(enum MessageVerboseLevel level) const;
    void set_verbose_level(enum MessageVerboseLevel level);
    enum MessageVerboseLevel get_verbose_level() const;

    void expect_msg_is_verbose(bool retval, enum MessageVerboseLevel level);
    void expect_msg_error_formatted(int error_code, int priority, const char *string);
    void expect_msg_error_formatted(int error_code, int priority, const char *prefix, const char *suffix);
    void expect_msg_error(int error_code, int priority, const char *string);
    void expect_msg_info_formatted(const char *string);
    void expect_msg_info(const char *string);
    void expect_msg_vinfo_formatted(enum MessageVerboseLevel level, const char *string);
    void expect_msg_vinfo(enum MessageVerboseLevel level, const char *string);
    void expect_msg_vinfo_formatted_if_not_ignored(enum MessageVerboseLevel level, const char *string);
    void expect_msg_vinfo_if_not_ignored(enum MessageVerboseLevel level, const char *string);
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
extern MockMessages *mock_messages_singleton;

#endif /* !MOCK_MESSAGES_HH */
