/*
 * Copyright (C) 2018--2020, 2022, 2023  T+A elektroakustik GmbH & Co. KG
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include <vector>
#include <list>
#include <sstream>

class Result
{
  public:
    enum class Outcome
    {
        SUCCESS,
        FAILURE,
        CRASH,
        UNKNOWN,
        SKIPPED,
    };

    const std::string name_;
    const double time_;
    const unsigned int assertions_;
    const Outcome outcome_;
    const std::vector<std::string> failure_messages_;
    const doctest::TestCaseFailureReason::Enum failure_reason_;

    Result(const Result &) = delete;
    Result(Result &&) = default;
    Result &operator=(const Result &) = delete;
    Result &operator=(Result &&) = delete;

  private:
    explicit Result(const char *name, double time, unsigned int assertions,
                    Outcome outcome, std::vector<std::string> &&failure_messages,
                    doctest::TestCaseFailureReason::Enum failure_reason):
        name_(name),
        time_(time),
        assertions_(assertions),
        outcome_(outcome),
        failure_messages_(std::move(failure_messages)),
        failure_reason_(failure_reason)
    {}

  public:
    /*!
     * Successful completion of a test.
     */
    explicit Result(const char *name, double time, unsigned int assertions):
        Result(name, time, assertions, Outcome::SUCCESS,
               {}, doctest::TestCaseFailureReason::None)
    {}

    /*!
     * Test was skipped.
     */
    explicit Result(const char *name):
        Result(name, -1.0, 0, Outcome::SKIPPED,
               {}, doctest::TestCaseFailureReason::None)
    {}

    /*!
     * Test failed, either by failing assertion or by crashing.
     */
    explicit Result(const char *name, double time, unsigned int assertions,
                    bool fatal_failure, std::vector<std::string> &&failure_messages,
                    doctest::TestCaseFailureReason::Enum failure_reason):
        Result(name, time, assertions,
               fatal_failure ? Outcome::CRASH : Outcome::FAILURE,
               std::move(failure_messages), failure_reason)
    {}

    /*!
     * Test failed for some obscure, unknown reason.
     */
    explicit Result(const char *name, double time, unsigned int assertions,
                    std::vector<std::string> &&failure_messages,
                    doctest::TestCaseFailureReason::Enum failure_reason):
        Result(name, time, assertions,
               Outcome::UNKNOWN, std::move(failure_messages), failure_reason)
    {}
};

static const char *failure_reason_to_string(doctest::TestCaseFailureReason::Enum reason)
{
    switch(reason)
    {
      case doctest::TestCaseFailureReason::None:                     return "";
      case doctest::TestCaseFailureReason::AssertFailure:            return "assertion";
      case doctest::TestCaseFailureReason::Exception:                return "exception";
      case doctest::TestCaseFailureReason::Crash:                    return "crash";
      case doctest::TestCaseFailureReason::TooManyFailedAsserts:     return "too_many_failed_asserts";
      case doctest::TestCaseFailureReason::Timeout:                  return "timeout";
      case doctest::TestCaseFailureReason::ShouldHaveFailedButDidnt: return "should_have_failed";
      case doctest::TestCaseFailureReason::ShouldHaveFailedAndDid:   return "failed_when_it_should";
      case doctest::TestCaseFailureReason::DidntFailExactlyNumTimes: return "unexpected_fail_count";
      case doctest::TestCaseFailureReason::FailedExactlyNumTimes:    return "expected_fail_count";
      case doctest::TestCaseFailureReason::CouldHaveFailedAndDid:    return "failed_when_it_could";
    }

    return "UNSUPPORTED";
}

/*!
 * Little helper class for buffer-less escaping of data for XML character data
 * while generating XML.
 */
class XmlEscape
{
  public:
    const char *const src_;

    XmlEscape(const XmlEscape &) = delete;
    XmlEscape &operator=(const XmlEscape &) = delete;

    explicit XmlEscape(const char *src): src_(src) {}
    explicit XmlEscape(const std::string &src): src_(src.c_str()) {}
};

/*!
 * Escape XML character data on the fly.
 */
static std::ostream &operator<<(std::ostream &os, const XmlEscape &data)
{
    size_t i = 0;

    while(1)
    {
        const char ch = data.src_[i++];

        if(ch == '\0')
            break;

        if(ch == '&')
            os << "&amp;";
        else if(ch == '<')
            os << "&lt;";
        else if(ch == '>')
            os << "&gt;";
        else if(ch == '\'')
            os << "&apos;";
        else if(ch == '"')
            os << "&quot;";
        else
            os << ch;
    }

    return os;
}

static void emit_failures(std::ostream &out, unsigned int indentation,
                          const char *tag, const Result &result,
                          bool is_failure_known = true)
{
    const std::string indent(indentation, ' ');

    for(const auto &msg : result.failure_messages_)
    {
        out << indent << '<' << tag << " type=\"";

        if(is_failure_known)
            out << failure_reason_to_string(result.failure_reason_);
        else
            out << "UNKNOWN: " << int(result.failure_reason_);

        out << "\" message=\"" << XmlEscape(msg) << "\"/>\n";
    }
}

class XmlReporter: public doctest::IReporter
{
  private:
    std::ostream &out;
    const doctest::ContextOptions &opt;
    const doctest::TestCaseData *test_case_data_;
    std::mutex mutex;

    std::vector<doctest::SubcaseSignature> subcases_stack;

    std::vector<std::string> failure_messages_;
    std::map<std::string, std::list<Result>> results_;

  public:
    XmlReporter(const doctest::ContextOptions &in):
        out(*in.cout),
        opt(in)
    {}

    void report_query(const doctest::QueryData& in) override {}
    void test_run_start() override {}

    void test_run_end(const doctest::TestRunStats &stats) override
    {
        out << "<testsuites>\n";

        for(const auto &ts : results_)
        {
            out << "  <testsuite"
                << " name=\"" << XmlEscape(ts.first) << '"'
                << " tests=\"" << ts.second.size() << "\">\n"
                ;

            for(const auto &result : ts.second)
            {
                out << "    <testcase"
                    << " name=\"" << XmlEscape(result.name_) << '"'
                    << " assertions=\"" << result.assertions_ << '"'
                    ;

                switch(result.outcome_)
                {
                  case Result::Outcome::SUCCESS:
                    out << " time=\"" << std::fixed << result.time_ << "\"/>\n";
                    break;

                  case Result::Outcome::FAILURE:
                    out << " time=\"" << std::fixed << result.time_ << "\">\n";
                    emit_failures(out, 6, "failure", result);
                    out << "    </testcase>\n";
                    break;

                  case Result::Outcome::CRASH:
                    out << ">\n";
                    emit_failures(out, 6, "error", result);
                    out << "    </testcase>\n";
                    break;

                  case Result::Outcome::UNKNOWN:
                    out << ">\n";
                    emit_failures(out, 6, "error", result, false);
                    out << "    </testcase>\n";
                    break;

                  case Result::Outcome::SKIPPED:
                    out << ">\n"
                        << "      <skipped/>\n"
                        << "    </testcase>\n"
                        ;
                    break;
                }
            }

            out << "  </testsuite>\n";
        }

        out << "</testsuites>\n";
    }

    void test_case_start(const doctest::TestCaseData &data) override
    {
        test_case_data_ = &data;
    }

    void test_case_reenter(const doctest::TestCaseData &data) override {}

    void test_case_end(const doctest::CurrentTestCaseStats &stats) override
    {
        if(stats.failure_flags == 0)
            return add_result(Result(test_case_data_->m_name, stats.seconds,
                                     stats.numAssertsCurrentTest));

        for(int mask = 1; mask <= doctest::TestCaseFailureReason::CouldHaveFailedAndDid; mask <<= 1)
        {
            const auto reason(static_cast<doctest::TestCaseFailureReason::Enum>(stats.failure_flags & mask));
            switch(reason)
            {
              case doctest::TestCaseFailureReason::None:
                break;

              case doctest::TestCaseFailureReason::Crash:
                add_result(Result(test_case_data_->m_name, stats.seconds,
                                  stats.numAssertsCurrentTest, true,
                                  std::move(failure_messages_), reason));
                return;

              case doctest::TestCaseFailureReason::Exception:
                add_result(Result(test_case_data_->m_name, stats.seconds,
                                  stats.numAssertsCurrentTest, false,
                                  std::move(failure_messages_), reason));
                return;

              case doctest::TestCaseFailureReason::AssertFailure:
              case doctest::TestCaseFailureReason::TooManyFailedAsserts:
              case doctest::TestCaseFailureReason::Timeout:
              case doctest::TestCaseFailureReason::ShouldHaveFailedButDidnt:
              case doctest::TestCaseFailureReason::ShouldHaveFailedAndDid:
              case doctest::TestCaseFailureReason::DidntFailExactlyNumTimes:
              case doctest::TestCaseFailureReason::FailedExactlyNumTimes:
              case doctest::TestCaseFailureReason::CouldHaveFailedAndDid:
                add_result(Result(test_case_data_->m_name, stats.seconds,
                                  stats.numAssertsCurrentTest, false,
                                  std::move(failure_messages_), reason));
                return;
            }
        }

        /* reached only if more failure flags have been added to doctest */
        add_result(Result(test_case_data_->m_name, stats.seconds,
                          stats.numAssertsCurrentTest,
                          std::move(failure_messages_),
                          static_cast<doctest::TestCaseFailureReason::Enum>(stats.failure_flags)));
    }

    void test_case_exception(const doctest::TestCaseException &e) override
    {
        failure_messages_.emplace_back(e.error_string.c_str());
    }

    void test_case_skipped(const doctest::TestCaseData &data) override
    {
        add_result(Result(data.m_name));
    }

    void subcase_start(const doctest::SubcaseSignature &sig) override
    {
        std::lock_guard<std::mutex> lock(mutex);
        subcases_stack.push_back(sig);
    }

    void subcase_end() override
    {
        std::lock_guard<std::mutex> lock(mutex);
        subcases_stack.pop_back();
    }

    void log_assert(const doctest::AssertData &data) override
    {
        std::lock_guard<std::mutex> lock(mutex);
        std::ostringstream os;

        if((data.m_at & doctest::assertType::is_throws_as) == 0)
            os << doctest::assertString(data.m_at) << "( " << data.m_expr << " ) -> ";

        if(data.m_at & doctest::assertType::is_throws_as)
        {
            os << doctest::assertString(data.m_at) << "( " << data.m_expr << ", "
               << data.m_exception_type << " ) -> ";

            if(data.m_threw)
                os << (data.m_threw_as ? "" : "different ")
                   << "exception: \"" << data.m_exception << '"';
            else
                os << "no exception thrown";
        }
        else if((data.m_at & (doctest::assertType::is_throws | doctest::assertType::is_nothrow)) ||
                data.m_threw)
        {
            if(data.m_threw)
                os << "exception: \"" << data.m_exception << '"';
            else
                os << "no exception thrown";
        }
        else
            os << "values ( " << data.m_decomp << " )";

        failure_messages_.emplace_back(os.str());
    }

    void log_message(const doctest::MessageData &data) override
    {}

  private:
    void add_result(Result &&result)
    {
        std::list<Result> *results = nullptr;
        const char *testsuite = (test_case_data_->m_test_suite != nullptr
                                 ? test_case_data_->m_test_suite
                                 : test_case_data_->m_file.c_str());

        try
        {
            results = &results_.at(testsuite);
        }
        catch(const std::out_of_range &e)
        {
            results_.emplace(testsuite, std::list<Result>());
            results = &results_.at(testsuite);
        }

        results->emplace_back(std::move(result));
    }
};

DOCTEST_REGISTER_REPORTER("strboxml", 1, XmlReporter);
