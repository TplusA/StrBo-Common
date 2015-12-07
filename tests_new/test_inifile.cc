/*
 * Copyright (C) 2018, 2019  T+A elektroakustik GmbH & Co. KG
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

#include "inifile.h"

#include "mock_messages.hh"
#include "mock_os.hh"

#include <iostream>
#include <algorithm>
#include <vector>

/*!
 * \addtogroup inifile_tests Unit tests
 * \ingroup inifile
 *
 * INI file parser and generator unit tests.
 */
/*!@{*/

TEST_SUITE_BEGIN("INI file parser");

class InifileParserTestsFixture
{
  protected:
    std::unique_ptr<MockMessages::Mock> mock_messages;
    struct ini_file ini;

  public:
    explicit InifileParserTestsFixture():
        mock_messages(std::make_unique<MockMessages::Mock>())
    {
        MockMessages::singleton = mock_messages.get();

        /* allow #inifile_free() to work in our dtor in case of early test
         * failures */
        inifile_new(&ini);
    }

    ~InifileParserTestsFixture()
    {
        try
        {
            inifile_free(&ini);
            mock_messages->done();
        }
        catch(...)
        {
            /* no throwing from dtors */
        }

        MockMessages::singleton = nullptr;
    }
};

/*!\test
 * Reading an empty file works and results in empty structures.
 */
TEST_CASE_FIXTURE(InifileParserTestsFixture, "Parse empty file from memory")
{
    std::fill(reinterpret_cast<uint8_t *>(&ini),
              reinterpret_cast<uint8_t *>(&ini) + sizeof(ini), 0xff);

    static const char dummy = '\0';
    REQUIRE(inifile_parse_from_memory(&ini, "test", &dummy, 0) == 0);
    CHECK(ini.sections_head == nullptr);
    CHECK(ini.sections_tail == nullptr);
}

/*!\test
 * Read a simple file containing two lines.
 */
TEST_CASE_FIXTURE(InifileParserTestsFixture, "Parse one section with one entry from memory")
{
    static const char text[] = "[global]\nkey = value";

    REQUIRE(inifile_parse_from_memory(&ini, "test", text, sizeof(text) - 1) == 0);
    CHECK(ini.sections_head != nullptr);
    CHECK(ini.sections_tail != nullptr);

    const auto *section = inifile_find_section(&ini, "global", 0);
    REQUIRE(section != nullptr);

    const auto *pair = inifile_section_lookup_kv_pair(section, "key", 0);
    REQUIRE(pair != nullptr);
    CHECK(pair->value == "value");
}

/*!\test
 * Read a file containing three sections, each containing four key/value pairs.
 *
 * This test is not exhaustive. It parses the file and then queries only a few
 * key/value pairs.
 */
TEST_CASE_FIXTURE(InifileParserTestsFixture, "Parse generic file from memory")
{
    static const char text[] =
        "[section 1]\n"
        "section 1 key 1 = value 1 in section 1\n"
        "section 1 key 2 = value 2 in section 1\n"
        "section 1 key 3 = value 3 in section 1\n"
        "section 1 key 4 = value 4 in section 1\n"
        "[section 2]\n"
        "section 2 key 1 = value 1 in section 2\n"
        "section 2 key 2 = value 2 in section 2\n"
        "section 2 key 3 = value 3 in section 2\n"
        "section 2 key 4 = value 4 in section 2\n"
        "[section 3]\n"
        "section 3 key 1 = value 1 in section 3\n"
        "section 3 key 2 = value 2 in section 3\n"
        "section 3 key 3 = value 3 in section 3\n"
        "section 3 key 4 = value 4 in section 3\n"
        ;

    REQUIRE(inifile_parse_from_memory(&ini, "test", text, sizeof(text) - 1) == 0);
    CHECK(ini.sections_head != nullptr);
    CHECK(ini.sections_tail != nullptr);

    const auto *section = inifile_find_section(&ini, "section 1", 0);
    REQUIRE(section != nullptr);

    const auto *pair = inifile_section_lookup_kv_pair(section, "section 1 key 1", 0);
    REQUIRE(pair != nullptr);
    CHECK(pair->value == "value 1 in section 1");

    pair = inifile_section_lookup_kv_pair(section, "section 1 key 4", 0);
    REQUIRE(pair != nullptr);
    CHECK(pair->value == "value 4 in section 1");

    section = inifile_find_section(&ini, "section 3", 0);
    REQUIRE(section != nullptr);

    pair = inifile_section_lookup_kv_pair(section, "section 3 key 2", 0);
    REQUIRE(pair != nullptr);
    CHECK(pair->value == "value 2 in section 3");

    pair = inifile_section_lookup_kv_pair(section, "section 3 key 4", 0);
    REQUIRE(pair != nullptr);
    CHECK(pair->value == "value 4 in section 3");
}

/*!\test
 * Read a file containing an assignment of nothing to some value.
 */
TEST_CASE_FIXTURE(InifileParserTestsFixture, "Parse empty values")
{
    static const char text[] =
        "[global]\n"
        "key 1 =\n"
        "key 2=\n"
        "key 3 = \n"
        "key 4 =     \n"
        ;

    REQUIRE(inifile_parse_from_memory(&ini, "test", text, sizeof(text) - 1) == 0);
    CHECK(ini.sections_head != nullptr);
    CHECK(ini.sections_tail != nullptr);

    const auto *section = inifile_find_section(&ini, "global", 0);
    REQUIRE(section != nullptr);

    const auto *pair = inifile_section_lookup_kv_pair(section, "key 1", 0);
    REQUIRE(pair != nullptr);
    CHECK(pair->value == "");

    pair = inifile_section_lookup_kv_pair(section, "key 2", 0);
    REQUIRE(pair != nullptr);
    CHECK(pair->value == "");

    pair = inifile_section_lookup_kv_pair(section, "key 3", 0);
    REQUIRE(pair != nullptr);
    CHECK(pair->value == "");

    pair = inifile_section_lookup_kv_pair(section, "key 4", 0);
    REQUIRE(pair != nullptr);
    CHECK(pair->value == "");
}

/*!\test
 * Attempting to find non-existent keys in a section returns \c nullptr pointers.
 */
TEST_CASE_FIXTURE(InifileParserTestsFixture, "Lookup nonexistent key in section returns nullptr")
{
    static const char text[] =
        "[foo]\n"
        "key 1 = bar"
        ;

    REQUIRE(inifile_parse_from_memory(&ini, "test", text, sizeof(text) - 1) == 0);
    CHECK(ini.sections_head != nullptr);

    const auto *section = inifile_find_section(&ini, "foo", 0);
    REQUIRE(section != nullptr);

    const auto *pair = inifile_section_lookup_kv_pair(section, "key 2", 0);
    CHECK(pair == nullptr);

    pair = inifile_section_lookup_kv_pair(section, "key", 0);
    CHECK(pair == nullptr);

    pair = inifile_section_lookup_kv_pair(section, "key does not exist", 0);
    CHECK(pair == nullptr);

    pair = inifile_section_lookup_kv_pair(section, "", 0);
    CHECK(pair == nullptr);
}

/*!\test
 * Assignments outside sections are ignored.
 */
TEST_CASE_FIXTURE(InifileParserTestsFixture, "Parser skips assignments before first section")
{
    static const char text[] =
        "ignore = this \n"
        "[section]\n"
        "key 1 = value 1"
        ;

    expect<MockMessages::MsgError>(mock_messages, EINVAL, LOG_ERR,
        "Expected begin of section, got junk (line 1 in \"test\") (Invalid argument)",
        false);

    REQUIRE(inifile_parse_from_memory(&ini, "test", text, sizeof(text) - 1) == 0);
    CHECK(ini.sections_head != nullptr);

    const auto *section = inifile_find_section(&ini, "section", 0);
    REQUIRE(section != nullptr);

    const auto *pair = inifile_section_lookup_kv_pair(section, "key 1", 0);
    REQUIRE(pair != nullptr);
    CHECK(pair->value == "value 1");

    pair = inifile_section_lookup_kv_pair(section, "ignore", 0);
    CHECK(pair == nullptr);
}

/*!\test
 * Empty sections are OK.
 */
TEST_CASE_FIXTURE(InifileParserTestsFixture, "Parser accepts empty sections")
{
    static const char text[] =
        "[empty section]\n"
        "[non-empty section]\n"
        "key = value\n"
        ;

    REQUIRE(inifile_parse_from_memory(&ini, "test", text, sizeof(text) - 1) == 0);
    CHECK(ini.sections_head != nullptr);

    const auto *section = inifile_find_section(&ini, "empty section", 0);
    REQUIRE(section != nullptr);
    CHECK(section->values_head == nullptr);

    const auto *pair = inifile_section_lookup_kv_pair(section, "key", 0);
    CHECK(pair == nullptr);

    section = inifile_find_section(&ini, "non-empty section", 0);
    REQUIRE(section != nullptr);

    pair = inifile_section_lookup_kv_pair(section, "key", 0);
    REQUIRE(pair != nullptr);
    CHECK(pair->value == "value");
}

/*!\test
 * Whitespace is being ignored in various places.
 */
TEST_CASE_FIXTURE(InifileParserTestsFixture, "Parser ignores insignificant spaces")
{
    static const char text[] =
        "\n"
        "  \n"
        "     [empty section]   \n"
        "\n"
        "\t\t   \t\n"
        "[ empty section]\n"
        "key a = value a\n"
        "[empty section ]\n"
        "key b = value b\n"
        "\t\t[non-empty section]\t\t\t\n"
        "\n"
        "   \t  key 1 = value 1\n"
        "key 2 = value 2  \t    \n"
        "key 3=value 3\n"
        "\t\t\n"
        "   \n"
        " \t\t  \n"
        "\n"
        ;

    REQUIRE(inifile_parse_from_memory(&ini, "test", text, sizeof(text) - 1) == 0);
    CHECK(ini.sections_head != nullptr);

    const auto *section = inifile_find_section(&ini, "empty section", 0);
    REQUIRE(section != nullptr);
    CHECK(section->values_head == nullptr);


    section = inifile_find_section(&ini, " empty section", 0);
    REQUIRE(section != nullptr);

    const auto *pair = inifile_section_lookup_kv_pair(section, "key a", 0);
    REQUIRE(pair != nullptr);
    CHECK(pair->value == "value a");

    pair = inifile_section_lookup_kv_pair(section, "key b", 0);
    CHECK(pair == nullptr);

    pair = inifile_section_lookup_kv_pair(section, "key 1", 0);
    CHECK(pair == nullptr);


    section = inifile_find_section(&ini, "empty section ", 0);
    REQUIRE(section != nullptr);

    pair = inifile_section_lookup_kv_pair(section, "key b", 0);
    REQUIRE(pair != nullptr);
    CHECK(pair->value == "value b");

    pair = inifile_section_lookup_kv_pair(section, "key a", 0);
    CHECK(pair == nullptr);

    pair = inifile_section_lookup_kv_pair(section, "key 1", 0);
    CHECK(pair == nullptr);


    section = inifile_find_section(&ini, "non-empty section", 0);
    REQUIRE(section != nullptr);

    pair = inifile_section_lookup_kv_pair(section, "key 1", 0);
    REQUIRE(pair != nullptr);
    CHECK(pair->value == "value 1");

    pair = inifile_section_lookup_kv_pair(section, "key 2", 0);
    REQUIRE(pair != nullptr);
    CHECK(pair->value == "value 2");

    pair = inifile_section_lookup_kv_pair(section, "key 3", 0);
    REQUIRE(pair != nullptr);
    CHECK(pair->value == "value 3");

    pair = inifile_section_lookup_kv_pair(section, "key a", 0);
    CHECK(pair == nullptr);

    pair = inifile_section_lookup_kv_pair(section, "key b", 0);
    CHECK(pair == nullptr);
}

/*!\test
 * In case the input file ends within a section header, that section is
 * ignored.
 */
TEST_CASE_FIXTURE(InifileParserTestsFixture, "End of file within section header ignores section")
{
    static const char text[] =
        "[section]\n"
        "key = value\n"
        "qux = qoo\n"
        "[foo"
        ;

    expect<MockMessages::MsgError>(mock_messages, EINVAL, LOG_ERR,
        "End of file within section header (line 4 in \"test\") (Invalid argument)",
        false);

    REQUIRE(inifile_parse_from_memory(&ini, "test", text, sizeof(text) - 1) == 0);
    CHECK(ini.sections_head != nullptr);

    const auto *section = inifile_find_section(&ini, "section", 0);
    REQUIRE(section != nullptr);

    const auto *pair = inifile_section_lookup_kv_pair(section, "key", 0);
    REQUIRE(pair != nullptr);
    CHECK(pair->value == "value");

    pair = inifile_section_lookup_kv_pair(section, "qux", 0);
    REQUIRE(pair != nullptr);
    CHECK(pair->value == "qoo");

    section = inifile_find_section(&ini, "foo", 0);
    CHECK(section == nullptr);
}

/*!\test
 * In case there is a line break within a section header, that section is
 * ignored.
 */
TEST_CASE_FIXTURE(InifileParserTestsFixture, "End of line within section header ignores section")
{
    static const char text[] =
        "[section]\n"
        "key = value\n"
        "qux = qoo\n"
        "[foo\n"
        "]\n"
        "foo key 1 = foo value 1\n"
        "foo key 2 = foo value 2\n"
        "[bar]\n"
        "bar key 1 = bar value 1\n"
        "bar key 2 = bar value 2\n"
        ;

    expect<MockMessages::MsgError>(mock_messages, EINVAL, LOG_ERR,
        "End of line within section header (line 4 in \"test\") (Invalid argument)",
        false);
    expect<MockMessages::MsgError>(mock_messages, EINVAL, LOG_ERR,
        "Expected begin of section, got junk (line 5 in \"test\") (Invalid argument)",
        false);
    expect<MockMessages::MsgError>(mock_messages, EINVAL, LOG_ERR,
        "Expected begin of section, got junk (line 6 in \"test\") (Invalid argument)",
        false);
    expect<MockMessages::MsgError>(mock_messages, EINVAL, LOG_ERR,
        "Expected begin of section, got junk (line 7 in \"test\") (Invalid argument)",
        false);

    REQUIRE(inifile_parse_from_memory(&ini, "test", text, sizeof(text) - 1) == 0);
    CHECK(ini.sections_head != nullptr);

    const auto *section = inifile_find_section(&ini, "section", 0);
    REQUIRE(section != nullptr);

    const auto *pair = inifile_section_lookup_kv_pair(section, "key", 0);
    REQUIRE(pair != nullptr);
    CHECK(pair->value == "value");

    pair = inifile_section_lookup_kv_pair(section, "foo key 1", 0);
    CHECK(pair == nullptr);

    pair = inifile_section_lookup_kv_pair(section, "foo key 1", 0);
    CHECK(pair == nullptr);


    section = inifile_find_section(&ini, "foo", 0);
    CHECK(section == nullptr);


    section = inifile_find_section(&ini, "bar", 0);
    REQUIRE(section != nullptr);

    pair = inifile_section_lookup_kv_pair(section, "bar key 1", 0);
    REQUIRE(pair != nullptr);
    CHECK(pair->value == "bar value 1");

    pair = inifile_section_lookup_kv_pair(section, "bar key 2", 0);
    REQUIRE(pair != nullptr);
    CHECK(pair->value == "bar value 2");

    pair = inifile_section_lookup_kv_pair(section, "foo key 1", 0);
    CHECK(pair == nullptr);

    pair = inifile_section_lookup_kv_pair(section, "foo key 1", 0);
    CHECK(pair == nullptr);
}

/*!\test
 * Line numbering in error messages is not confused if there are multiple
 * parser errors.
 */
TEST_CASE_FIXTURE(InifileParserTestsFixture, "Line numbers in error messages remain accurate")
{
    static const char text[] =
        "[section]\n"
        "key = value\n"
        "qux = qoo\n"
        "[foo\n"
        "]\n"
        "foo key 1 = foo value 1\n"
        "[bar]\n"
        "bar key 1 = bar value 1\n"
        "[foobar\n"
        "\n"
        " \n"
        "foobar key 1 = foobar value 1\n"
        "foobar key 2 = foobar value 2\n"
        "\n"
        "  [  broken"
        ;

    expect<MockMessages::MsgError>(mock_messages, EINVAL, LOG_ERR,
        "End of line within section header (line 4 in \"test\") (Invalid argument)",
        false);
    expect<MockMessages::MsgError>(mock_messages, EINVAL, LOG_ERR,
        "Expected begin of section, got junk (line 5 in \"test\") (Invalid argument)",
        false);
    expect<MockMessages::MsgError>(mock_messages, EINVAL, LOG_ERR,
        "Expected begin of section, got junk (line 6 in \"test\") (Invalid argument)",
        false);
    expect<MockMessages::MsgError>(mock_messages, EINVAL, LOG_ERR,
        "End of line within section header (line 9 in \"test\") (Invalid argument)",
        false);
    expect<MockMessages::MsgError>(mock_messages, EINVAL, LOG_ERR,
        "Expected begin of section, got junk (line 12 in \"test\") (Invalid argument)",
        false);
    expect<MockMessages::MsgError>(mock_messages, EINVAL, LOG_ERR,
        "Expected begin of section, got junk (line 13 in \"test\") (Invalid argument)",
        false);
    expect<MockMessages::MsgError>(mock_messages, EINVAL, LOG_ERR,
        "End of file within section header (line 15 in \"test\") (Invalid argument)",
        false);

    REQUIRE(inifile_parse_from_memory(&ini, "test", text, sizeof(text) - 1) == 0);
    CHECK(ini.sections_head != nullptr);

    const auto *section = inifile_find_section(&ini, "section", 0);
    CHECK(section != nullptr);

    section = inifile_find_section(&ini, "foo", 0);
    CHECK(section == nullptr);

    section = inifile_find_section(&ini, "bar", 0);
    CHECK(section != nullptr);

    section = inifile_find_section(&ini, "foobar", 0);
    CHECK(section == nullptr);
}

/*!\test
 * Line gets ignored if the assignment character is missing where an assignment
 * is expected.
 */
TEST_CASE_FIXTURE(InifileParserTestsFixture, "Missing assignment character is detected")
{
    static const char text[] =
        "[section]\n"
        "key value\n"
        "a = b\n"
        ;

    /* EOL */
    expect<MockMessages::MsgError>(mock_messages, EINVAL, LOG_ERR,
        "Expected assignment (line 2 in \"test\") (Invalid argument)",
        false);

    REQUIRE(inifile_parse_from_memory(&ini, "test", text, sizeof(text) - 1) == 0);
    CHECK(ini.sections_head != nullptr);

    const auto *section = inifile_find_section(&ini, "section", 0);
    REQUIRE(section != nullptr);

    const auto *pair = inifile_section_lookup_kv_pair(section, "key", 0);
    CHECK(pair == nullptr);

    inifile_free(&ini);

    /* EOF */
    expect<MockMessages::MsgError>(mock_messages, EINVAL, LOG_ERR,
        "Expected assignment (line 2 in \"test\") (Invalid argument)",
        false);

    REQUIRE(inifile_parse_from_memory(&ini, "test", text, sizeof(text) - 8) == 0);
    CHECK(ini.sections_head != nullptr);

    section = inifile_find_section(&ini, "section", 0);
    REQUIRE(section != nullptr);

    pair = inifile_section_lookup_kv_pair(section, "key", 0);
    CHECK(pair == nullptr);
}

/*!\test
 * Line gets ignored if there is no value after the assignment character.
 */
TEST_CASE_FIXTURE(InifileParserTestsFixture, "Missing value after assignment is detected")
{
    static const char text[] =
        "[section]\n"
        "key =\n"
        "a = b\n"
        ;

    /* EOL */
    REQUIRE(inifile_parse_from_memory(&ini, "test", text, sizeof(text) - 1) == 0);
    CHECK(ini.sections_head != nullptr);

    const auto *section = inifile_find_section(&ini, "section", 0);
    REQUIRE(section != nullptr);

    const auto *pair = inifile_section_lookup_kv_pair(section, "key", 0);
    REQUIRE(pair != nullptr);
    CHECK(pair->value == "");

    pair = inifile_section_lookup_kv_pair(section, "a", 0);
    REQUIRE(pair != nullptr);
    CHECK(pair->value == "b");

    inifile_free(&ini);

    /* EOF */
    REQUIRE(inifile_parse_from_memory(&ini, "test", text, sizeof(text) - 8) == 0);
    CHECK(ini.sections_head != nullptr);

    section = inifile_find_section(&ini, "section", 0);
    REQUIRE(section != nullptr);

    pair = inifile_section_lookup_kv_pair(section, "key", 0);
    CHECK(pair != nullptr);
}

/*!\test
 * Line gets ignored if there is no key name in front of the assignment
 * character.
 */
TEST_CASE_FIXTURE(InifileParserTestsFixture, "Missing key name before assignment is detected")
{
    static const char text[] =
        "[section]\n"
        "= value\n"
        "a = b\n"
        ;

    /* EOL */
    expect<MockMessages::MsgError>(mock_messages, EINVAL, LOG_ERR,
        "Expected key name (line 2 in \"test\") (Invalid argument)", false);

    REQUIRE(inifile_parse_from_memory(&ini, "test", text, sizeof(text) - 1) == 0);
    CHECK(ini.sections_head != nullptr);

    const auto *section = inifile_find_section(&ini, "section", 0);
    REQUIRE(section != nullptr);

    const auto *pair = inifile_section_lookup_kv_pair(section, "key", 0);
    CHECK(pair == nullptr);

    pair = inifile_section_lookup_kv_pair(section, "a", 0);
    REQUIRE(pair != nullptr);
    CHECK(pair->value == "b");

    inifile_free(&ini);

    /* EOF */
    expect<MockMessages::MsgError>(mock_messages, EINVAL, LOG_ERR,
        "Expected key name (line 2 in \"test\") (Invalid argument)", false);

    REQUIRE(inifile_parse_from_memory(&ini, "test", text, sizeof(text) - 8) == 0);
    CHECK(ini.sections_head != nullptr);

    section = inifile_find_section(&ini, "section", 0);
    REQUIRE(section != nullptr);

    pair = inifile_section_lookup_kv_pair(section, "key", 0);
    CHECK(pair == nullptr);
}

/*!\test
 * Values may contain the assignment character.
 */
TEST_CASE_FIXTURE(InifileParserTestsFixture, "Second assignment character is part of value")
{
    static const char text[] =
        "[section]\n"
        "key = value = foo\n";

    REQUIRE(inifile_parse_from_memory(&ini, "test", text, sizeof(text) - 1) == 0);
    CHECK(ini.sections_head != nullptr);

    const auto *section = inifile_find_section(&ini, "section", 0);
    REQUIRE(section != nullptr);

    const auto *pair = inifile_section_lookup_kv_pair(section, "key", 0);
    REQUIRE(pair != nullptr);
    CHECK(pair->value == "value = foo");
}

/*!\test
 * A section header with empty name is invalid and causes the parser to search
 * for the next valid section header.
 *
 * Anything betwee the empty section header and the next valid section header
 * gets ignored.
 */
TEST_CASE_FIXTURE(InifileParserTestsFixture, "Sections with empty section name are skipped")
{
    static const char text[] =
        "[section]\n"
        "key = value\n"
        "[]\n"
        "foo = bar\n"
        "[section 2]\n"
        "key 2 = value 2\n"
        ;

    expect<MockMessages::MsgError>(mock_messages, EINVAL, LOG_ERR,
        "Empty section name (line 3 in \"test\") (Invalid argument)", false);
    expect<MockMessages::MsgError>(mock_messages, EINVAL, LOG_ERR,
        "Expected begin of section, got junk (line 4 in \"test\") (Invalid argument)",
        false);

    REQUIRE(inifile_parse_from_memory(&ini, "test", text, sizeof(text) - 1) == 0);
    CHECK(ini.sections_head != nullptr);

    const auto *section = inifile_find_section(&ini, "section", 0);
    REQUIRE(section != nullptr);

    const auto *pair = inifile_section_lookup_kv_pair(section, "key", 0);
    REQUIRE(pair != nullptr);
    CHECK(pair->value == "value");

    section = inifile_find_section(&ini, "", 0);
    CHECK(section == nullptr);

    section = inifile_find_section(&ini, "section 2", 0);
    REQUIRE(section != nullptr);

    pair = inifile_section_lookup_kv_pair(section, "key 2", 0);
    REQUIRE(pair != nullptr);
    CHECK(pair->value == "value 2");
}

/*!\test
 * A section header name may be a simple whitespace.
 */
TEST_CASE_FIXTURE(InifileParserTestsFixture, "Sections with whitespace section names are ok")
{
    static const char text[] =
        "[ ]\n"
        "foo = bar\n"
        ;

    REQUIRE(inifile_parse_from_memory(&ini, "test", text, sizeof(text) - 1) == 0);
    CHECK(ini.sections_head != nullptr);

    const auto *section = inifile_find_section(&ini, " ", 0);
    REQUIRE(section != nullptr);

    const auto *pair = inifile_section_lookup_kv_pair(section, "foo", 0);
    REQUIRE(pair != nullptr);
    CHECK(pair->value == "bar");
}

/*!\test
 * Only blank characters and a newline may follow the closing bracket of a
 * section header, otherwise the header is considered invalid.
 */
TEST_CASE_FIXTURE(InifileParserTestsFixture, "Sections with junk after section header are skipped")
{
    static const char text[] =
        "[section]\n"
        "key = value\n"
        "[section 2] x\n"
        "foo = bar\n"
        "[section 3]\n"
        "key 3 = value 3\n"
        ;

    expect<MockMessages::MsgError>(mock_messages, EINVAL, LOG_ERR,
        "Got junk after section header (line 3 in \"test\") (Invalid argument)",
        false);
    expect<MockMessages::MsgError>(mock_messages, EINVAL, LOG_ERR,
        "Expected begin of section, got junk (line 4 in \"test\") (Invalid argument)",
        false);

    REQUIRE(inifile_parse_from_memory(&ini, "test", text, sizeof(text) - 1) == 0);
    CHECK(ini.sections_head != nullptr);

    const auto *section = inifile_find_section(&ini, "section", 0);
    REQUIRE(section != nullptr);

    const auto *pair = inifile_section_lookup_kv_pair(section, "key", 0);
    REQUIRE(pair != nullptr);
    CHECK(pair->value == "value");

    section = inifile_find_section(&ini, "section 2", 0);
    CHECK(section == nullptr);

    section = inifile_find_section(&ini, "section 3", 0);
    REQUIRE(section != nullptr);

    pair = inifile_section_lookup_kv_pair(section, "key 3", 0);
    REQUIRE(pair != nullptr);
    CHECK(pair->value == "value 3");
}

/*!\test
 * Keys can have values assigned multiple times, but only the last value is
 * taken.
 */
TEST_CASE_FIXTURE(InifileParserTestsFixture, "Multiple assignments to a key name keeps last assignment")
{
    static const char text[] =
        "[section]\n"
        "key = value\n"
        "foo = bar\n"
        "key = value 2\n"
        "foo = foobar\n"
        "key = value 3\n"
        ;

    REQUIRE(inifile_parse_from_memory(&ini, "test", text, sizeof(text) - 1) == 0);
    CHECK(ini.sections_head != nullptr);

    const auto *section = inifile_find_section(&ini, "section", 0);
    REQUIRE(section != nullptr);

    const auto *pair = inifile_section_lookup_kv_pair(section, "key", 0);
    REQUIRE(pair != nullptr);
    CHECK(pair->value == "value 3");

    pair = inifile_section_lookup_kv_pair(section, "foo", 0);
    REQUIRE(pair != nullptr);
    CHECK(pair->value == "foobar");
}

TEST_SUITE_END();


TEST_SUITE_BEGIN("INI file manipulation");

class InifileManipulationTestsFixture
{
  protected:
    std::unique_ptr<MockMessages::Mock> mock_messages;
    std::unique_ptr<MockOS::Mock> mock_os;
    struct ini_file ini;

    std::vector<uint8_t> os_write_buffer;
    static constexpr int expected_os_write_fd = 123;

  public:
    explicit InifileManipulationTestsFixture():
        mock_messages(std::make_unique<MockMessages::Mock>()),
        mock_os(std::make_unique<MockOS::Mock>())
    {
        MockMessages::singleton = mock_messages.get();
        MockOS::singleton = mock_os.get();

        os_write_buffer.clear();

        /* allow #inifile_free() to work in dtor in case of early test
         * failures */
        inifile_new(&ini);
    }

    ~InifileManipulationTestsFixture()
    {
        try
        {
            inifile_free(&ini);

            os_write_buffer.clear();
            os_write_buffer.shrink_to_fit();

            mock_messages->done();
            mock_os->done();
        }
        catch(...)
        {
            /* no throwing from dtors */
        }

        MockMessages::singleton = nullptr;
        MockOS::singleton = nullptr;
    }

    MockOS::WriteFromBuffer::Callback buffer_writer()
    {
        return
            [this] (const void *src, size_t count, int fd)
            {
                return write_from_buffer_callback(src, count, fd);
            };
    }

  private:
    int write_from_buffer_callback(const void *src, size_t count, int fd)
    {
        CHECK(fd == int(expected_os_write_fd));
        CHECK(count > 0);
        REQUIRE(src != nullptr);
        std::copy_n(static_cast<const uint8_t *>(src), count,
                    std::back_inserter(os_write_buffer));
        return 0;
    }
};

/*!\test
 * Initialize INI file structure in memory.
 */
TEST_CASE_FIXTURE(InifileManipulationTestsFixture, "Create empty file structure")
{
    std::fill(reinterpret_cast<uint8_t *>(&ini),
              reinterpret_cast<uint8_t *>(&ini) + sizeof(ini), 0xff);

    inifile_new(&ini);
    CHECK(ini.sections_head == nullptr);
    CHECK(ini.sections_tail == nullptr);
}

/*!\test
 * Construct an INI file containing two sections with a few assignments and a
 * third, empty section.
 */
TEST_CASE_FIXTURE(InifileManipulationTestsFixture, "New_write_ini_file")
{
    auto *section = inifile_new_section(&ini, "First", 0);
    REQUIRE(section != nullptr);

    CHECK(inifile_section_store_value(section, "key 1", 0, "value 1", 0) != nullptr);
    CHECK(inifile_section_store_value(section, "key 2", 0, "value 2", 0) != nullptr);
    CHECK(inifile_section_store_value(section, "key 3", 0, "value 3", 0) != nullptr);

    section = inifile_new_section(&ini, "Second", 0);
    CHECK(section != nullptr);

    section = inifile_new_section(&ini, "Third", 0);
    CHECK(section != nullptr);

    CHECK(inifile_section_store_value(section, "foo", 0, "bar", 0) != nullptr);
    CHECK(inifile_section_store_value(section, "foobar", 0, "barfoo", 0) != nullptr);

    expect<MockOS::FileNew>(mock_os, expected_os_write_fd, 0, "outfile.config");
    for(int i = 0; i < 3 * 3 + (3 + 0 + 2) * 4; ++i)
        expect<MockOS::WriteFromBuffer>(mock_os, 0, buffer_writer());
    expect<MockOS::FileClose>(mock_os, 0, expected_os_write_fd);

    REQUIRE(inifile_write_to_file(&ini, "outfile.config") == 0);

    static const char expected_ini_file[] =
        "[First]\n"
        "key 1 = value 1\n"
        "key 2 = value 2\n"
        "key 3 = value 3\n"
        "[Second]\n"
        "[Third]\n"
        "foo = bar\n"
        "foobar = barfoo\n"
        ;

    os_write_buffer.push_back(0);
    CHECK(reinterpret_cast<const char *>(os_write_buffer.data()) == expected_ini_file);
}

/*!\test
 * Regular adding and removing of sections.
 */
TEST_CASE_FIXTURE(InifileManipulationTestsFixture, "Add and remove sections")
{
    CHECK(inifile_new_section(&ini, "Foo", 0) != nullptr);
    CHECK(ini.sections_head != nullptr);
    CHECK(ini.sections_head == ini.sections_tail);
    CHECK(inifile_remove_section_by_name(&ini, "Foo", 3));
    CHECK(ini.sections_head == nullptr);
    CHECK(ini.sections_tail == nullptr);

    CHECK(inifile_new_section(&ini, "Bar", 3) != nullptr);
    CHECK(ini.sections_head != nullptr);
    CHECK(ini.sections_head == ini.sections_tail);
    CHECK(inifile_new_section(&ini, "Foobar", 6) != nullptr);
    CHECK(ini.sections_head != nullptr);
    CHECK(ini.sections_head != ini.sections_tail);
    CHECK(inifile_new_section(&ini, "Baz", 3) != nullptr);
    CHECK(inifile_new_section(&ini, "Qux", 3) != nullptr);

    CHECK(inifile_remove_section_by_name(&ini, "Bar", 3));
    CHECK(inifile_remove_section(&ini, inifile_find_section(&ini, "Baz", 0)));
    const struct ini_section *section = inifile_find_section(&ini, "Qux", 0);
    CHECK(inifile_remove_section_by_name(&ini, section->name, section->name_length));
    CHECK(inifile_remove_section_by_name(&ini, "Foobar", 0));

    CHECK(ini.sections_head == nullptr);
    CHECK(ini.sections_tail == nullptr);
}

/*!\test
 * Trying to remove nonexistent sections by name is not fatal.
 */
TEST_CASE_FIXTURE(InifileManipulationTestsFixture, "Removing nonexistent section by name returns failure")
{
    /* OK for empty files */
    CHECK_FALSE(inifile_remove_section_by_name(&ini, "Foo", 0));

    CHECK(inifile_new_section(&ini, "Empty", 0) != nullptr);

    /* OK for files with content */
    CHECK_FALSE(inifile_remove_section_by_name(&ini, "Foo", 0));
}

/*!\test
 * Trying to remove nonexistent sections by wrong pointer is not fatal.
 */
TEST_CASE_FIXTURE(InifileManipulationTestsFixture, "Removing nonexistent section by section pointer returns failure")
{
    auto *does_not_exist = reinterpret_cast<struct ini_section *>(0x0123);

    /* OK for empty files */
    CHECK_FALSE(inifile_remove_section(&ini, does_not_exist));

    struct ini_section *section = inifile_new_section(&ini, "Empty", 0);
    CHECK(section != nullptr);

    /* OK for files with content */
    CHECK_FALSE(inifile_remove_section(&ini, does_not_exist));
}

/*!\test
 * Trying to remove \c nullptr section is not fatal.
 */
TEST_CASE_FIXTURE(InifileManipulationTestsFixture, "Removing null section returns failure")
{
    /* OK for empty files */
    CHECK_FALSE(inifile_remove_section(&ini, nullptr));

    CHECK(inifile_new_section(&ini, "Empty", 0) != nullptr);

    /* OK for files with content */
    CHECK_FALSE(inifile_remove_section(&ini, nullptr));
}

/*!\test
 * It is possible to load an INI file, remove a section, and write it back with
 * the section removed.
 */
TEST_CASE_FIXTURE(InifileManipulationTestsFixture, "Remove section from file")
{
    static const char text[] =
        "[First]\n"
        "key 1-1 = value 1-1\n"
        "key 1-2 = value 1-2\n"
        "[Second]\n"
        "key 2-1 = value 2-1\n"
        "key 2-2 = value 2-2\n"
        "key 2-3 = value 2-3\n"
        "[Third]\n"
        "key 3-1 = value 3-1\n"
        "key 3-2 = value 3-2\n"
        ;

    REQUIRE(inifile_parse_from_memory(&ini, "test", text, sizeof(text) - 1) == 0);
    CHECK(ini.sections_head != nullptr);

    CHECK(inifile_remove_section_by_name(&ini, "Second", 0));

    expect<MockOS::FileNew>(mock_os, expected_os_write_fd, 0, "outfile.config");
    for(int i = 0; i < 2 * 3 + (2 + 2) * 4; ++i)
        expect<MockOS::WriteFromBuffer>(mock_os, 0, buffer_writer());
    expect<MockOS::FileClose>(mock_os, 0, expected_os_write_fd);

    REQUIRE(inifile_write_to_file(&ini, "outfile.config") == 0);

    static const char expected_ini_file[] =
        "[First]\n"
        "key 1-1 = value 1-1\n"
        "key 1-2 = value 1-2\n"
        "[Third]\n"
        "key 3-1 = value 3-1\n"
        "key 3-2 = value 3-2\n"
        ;

    os_write_buffer.push_back(0);
    CHECK(reinterpret_cast<const char *>(os_write_buffer.data()) == expected_ini_file);
}

/*!\test
 * It is possible to assign empty values to keys.
 */
TEST_CASE_FIXTURE(InifileManipulationTestsFixture, "Write empty value to .ini file")
{
    auto *section = inifile_new_section(&ini, "First", 0);
    REQUIRE(section != nullptr);

    CHECK(inifile_section_store_empty_value(section, "empty", 0) != nullptr);

    section = inifile_new_section(&ini, "Second", 0);
    REQUIRE(section != nullptr);

    CHECK(inifile_section_store_value(section, "foo", 0, "bar", 0) != nullptr);
    CHECK(inifile_section_store_empty_value(section, "foobar", 0) != nullptr);

    expect<MockOS::FileNew>(mock_os, expected_os_write_fd, 0, "outfile.config");
    for(int i = 0; i < 2 * 3 + 3 + 4 + 3; ++i)
        expect<MockOS::WriteFromBuffer>(mock_os, 0, buffer_writer());
    expect<MockOS::FileClose>(mock_os, 0, expected_os_write_fd);

    REQUIRE(inifile_write_to_file(&ini, "outfile.config") == 0);

    static const char expected_ini_file[] =
        "[First]\n"
        "empty = \n"
        "[Second]\n"
        "foo = bar\n"
        "foobar = \n"
        ;

    os_write_buffer.push_back(0);
    CHECK(reinterpret_cast<const char *>(os_write_buffer.data()) == expected_ini_file);
}

/*!\test
 * Parse an INI file, change a few values, write a new INI file containing the
 * changes.
 */
TEST_CASE_FIXTURE(InifileManipulationTestsFixture, "Manipulate value in file")
{
    static const char text[] =
        "[First]\n"
        "key 1-1 = value 1-1\n"
        "key 1-2 = value 1-2\n"
        "[Second]\n"
        "key 2-1 = value 2-1\n"
        "key 2-2 = value 2-2\n"
        "key 2-3 = value 2-3\n"
        "[Third]\n"
        "key 3-1 = value 3-1\n"
        "key 3-2 = value 3-2\n"
        ;

    REQUIRE(inifile_parse_from_memory(&ini, "test", text, sizeof(text) - 1) == 0);
    CHECK(ini.sections_head != nullptr);

    auto *section = inifile_find_section(&ini, "Second", 0);
    REQUIRE(section != nullptr);

    CHECK(inifile_section_store_value(section, "key 2-1", 0, "changed value", 0) != nullptr);
    CHECK(inifile_section_store_value(section, "key 11-13", 0, "new value", 0) != nullptr);

    section = inifile_find_section(&ini, "Third", 0);
    REQUIRE(section != nullptr);

    CHECK(inifile_section_store_value(section, "key 3-2", 0, "also changed", 0) != nullptr);

    expect<MockOS::FileNew>(mock_os, expected_os_write_fd, 0, "outfile.config");
    for(int i = 0; i < 3 * 3 + (2 + 4 + 2) * 4; ++i)
        expect<MockOS::WriteFromBuffer>(mock_os, 0, buffer_writer());
    expect<MockOS::FileClose>(mock_os, 0, expected_os_write_fd);

    REQUIRE(inifile_write_to_file(&ini, "outfile.config") == 0);

    static const char expected_ini_file[] =
        "[First]\n"
        "key 1-1 = value 1-1\n"
        "key 1-2 = value 1-2\n"
        "[Second]\n"
        "key 2-1 = changed value\n"
        "key 2-2 = value 2-2\n"
        "key 2-3 = value 2-3\n"
        "key 11-13 = new value\n"
        "[Third]\n"
        "key 3-1 = value 3-1\n"
        "key 3-2 = also changed\n"
        ;

    os_write_buffer.push_back(0);
    CHECK(reinterpret_cast<const char *>(os_write_buffer.data()) == expected_ini_file);
}

/*!\test
 * In case #os_file_new() fails for whatever reason, function
 * #inifile_write_to_file() returns an error.
 */
TEST_CASE_FIXTURE(InifileManipulationTestsFixture, "Write file fails if file cannot be created")
{
    CHECK(inifile_new_section(&ini, "section", 0) != nullptr);
    expect<MockOS::FileNew>(mock_os, -1, ENOSPC, "outfile.config");
    CHECK(inifile_write_to_file(&ini, "outfile.config") == -1);
    CHECK(os_write_buffer.empty());
}

/*!\test
 * In case #os_write_from_buffer() fails for whatever reason, function
 * #inifile_write_to_file() deletes the output file and returns an error.
 */
TEST_CASE_FIXTURE(InifileManipulationTestsFixture, "Write file fails if file cannot be written")
{
    CHECK(inifile_new_section(&ini, "section", 0) != nullptr);

    expect<MockOS::FileNew>(mock_os, expected_os_write_fd, 0, "outfile.config");
    expect<MockOS::WriteFromBuffer>(mock_os, -1, ENOSPC, false, 1, expected_os_write_fd);
    expect<MockOS::FileClose>(mock_os, 0, expected_os_write_fd);
    expect<MockOS::FileDelete>(mock_os, 0, 0, "outfile.config");
    expect<MockMessages::MsgError>(mock_messages, 0, LOG_ERR,
        "Failed writing INI file \"outfile.config\", deleting partially written file",
        false);

    CHECK(inifile_write_to_file(&ini, "outfile.config") == -1);

    CHECK(os_write_buffer.empty());
}

/*!\test
 * In case #os_write_from_buffer() fails for whatever reason, function
 * #inifile_write_to_file() tries to delete the output file. If this fails,
 * then an additional diagnostic message is emitted.
 */
TEST_CASE_FIXTURE(InifileManipulationTestsFixture, "Message on failure to delete file after failure to write to file")
{
    CHECK(inifile_new_section(&ini, "section", 0) != nullptr);

    expect<MockOS::FileNew>(mock_os, expected_os_write_fd, 0, "outfile.config");
    expect<MockOS::WriteFromBuffer>(mock_os, -1, EIO, false, 1, expected_os_write_fd);
    expect<MockOS::FileClose>(mock_os, 0, expected_os_write_fd);
    expect<MockOS::FileDelete>(mock_os, -1, EIO, "outfile.config");
    expect<MockMessages::MsgError>(mock_messages, 0, LOG_ERR,
        "Failed writing INI file \"outfile.config\", deleting partially written file",
        false);
    expect<MockMessages::MsgError>(mock_messages, EIO, LOG_ERR,
        "Failed to delete incomplete file (Input/output error)", false);

    CHECK(inifile_write_to_file(&ini, "outfile.config") == -1);

    CHECK(os_write_buffer.empty());
}

/*!\test
 * Removing a key from an empty section returns an error.
 */
TEST_CASE_FIXTURE(InifileManipulationTestsFixture, "Remove key from empty section")
{
    auto *section = inifile_new_section(&ini, "section", 0);
    REQUIRE(section != nullptr);
    CHECK_FALSE(inifile_section_remove_value(section, "key", 0));
}

/*!\test
 * Removing the only key results in an empty section.
 */
TEST_CASE_FIXTURE(InifileManipulationTestsFixture, "Remove only key from section")
{
    auto *section = inifile_new_section(&ini, "section", 0);
    REQUIRE(section != nullptr);

    CHECK(inifile_section_store_value(section, "key", 0, "value", 0) != nullptr);

    CHECK(inifile_section_remove_value(section, "key", 0));
    CHECK(section->values_head == nullptr);
    CHECK(section->values_tail == nullptr);
}

/*!\test
 * Removing the first key from a section works as expected.
 */
TEST_CASE_FIXTURE(InifileManipulationTestsFixture, "Remove existing first key from section")
{
    auto *section = inifile_new_section(&ini, "section", 0);
    REQUIRE(section != nullptr);

    CHECK(inifile_section_store_value(section, "key 1", 0, "value 1", 0) != nullptr);
    CHECK(inifile_section_store_value(section, "key 2", 0, "value 2", 0) != nullptr);
    CHECK(inifile_section_store_value(section, "key 3", 0, "value 3", 0) != nullptr);

    auto *head = section->values_head;
    auto *tail = section->values_tail;

    CHECK(inifile_section_remove_value(section, "key 1", 0));

    CHECK(inifile_section_lookup_kv_pair(section, "key 1", 0) == nullptr);
    CHECK(inifile_section_lookup_kv_pair(section, "key 2", 0) != nullptr);
    CHECK(inifile_section_lookup_kv_pair(section, "key 3", 0) != nullptr);

    CHECK(section->values_head != head);
    CHECK(section->values_tail == tail);
}

/*!\test
 * Removing a key from the middle of a section works as expected.
 */
TEST_CASE_FIXTURE(InifileManipulationTestsFixture, "Remove existing middle key from section")
{
    auto *section = inifile_new_section(&ini, "section", 0);
    REQUIRE(section != nullptr);

    CHECK(inifile_section_store_value(section, "key 1", 0, "value 1", 0) != nullptr);
    CHECK(inifile_section_store_value(section, "key 2", 0, "value 2", 0) != nullptr);
    CHECK(inifile_section_store_value(section, "key 3", 0, "value 3", 0) != nullptr);

    auto *head = section->values_head;
    auto *tail = section->values_tail;

    CHECK(inifile_section_remove_value(section, "key 2", 0));

    CHECK(inifile_section_lookup_kv_pair(section, "key 1", 0) != nullptr);
    CHECK(inifile_section_lookup_kv_pair(section, "key 2", 0) == nullptr);
    CHECK(inifile_section_lookup_kv_pair(section, "key 3", 0) != nullptr);

    CHECK(section->values_head == head);
    CHECK(section->values_tail == tail);
}

/*!\test
 * Removing the last key from a section works as expected.
 */
TEST_CASE_FIXTURE(InifileManipulationTestsFixture, "Remove existing tail key from section")
{
    auto *section = inifile_new_section(&ini, "section", 0);
    REQUIRE(section != nullptr);

    CHECK(inifile_section_store_value(section, "key 1", 0, "value 1", 0) != nullptr);
    CHECK(inifile_section_store_value(section, "key 2", 0, "value 2", 0) != nullptr);
    CHECK(inifile_section_store_value(section, "key 3", 0, "value 3", 0) != nullptr);

    auto *head = section->values_head;
    auto *tail = section->values_tail;

    CHECK(inifile_section_remove_value(section, "key 3", 0));

    CHECK(inifile_section_lookup_kv_pair(section, "key 1", 0) != nullptr);
    CHECK(inifile_section_lookup_kv_pair(section, "key 2", 0) != nullptr);
    CHECK(inifile_section_lookup_kv_pair(section, "key 3", 0) == nullptr);

    CHECK(section->values_head == head);
    CHECK(section->values_tail != tail);
}

/*!\test
 * Removing a non-existent key from a section returns an error.
 */
TEST_CASE_FIXTURE(InifileManipulationTestsFixture, "Remove nonexistant key from section")
{
    auto *section = inifile_new_section(&ini, "section", 0);
    REQUIRE(section != nullptr);

    CHECK(inifile_section_store_value(section, "key", 0, "value", 0) != nullptr);
    CHECK_FALSE(inifile_section_remove_value(section, "k", 0));
    CHECK(inifile_section_lookup_kv_pair(section, "key", 0) != nullptr);
}

TEST_SUITE_END();

/*!@}*/
