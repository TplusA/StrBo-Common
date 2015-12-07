/*
 * Copyright (C) 2015  T+A elektroakustik GmbH & Co. KG
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

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include "inifile.h"
#include "messages.h"
#include "os.h"

enum parser_state
{
    STATE_EXPECT_SECTION_BEGIN = 0,
    STATE_EXPECT_SECTION_NAME,
    STATE_EXPECT_ASSIGNMENT,

    STATE_LAST_PARSER_STATE = STATE_EXPECT_ASSIGNMENT,
};

enum skip_result
{
    SKIP_RESULT_OK,
    SKIP_RESULT_EOL,
    SKIP_RESULT_EOF,
};

struct parser_data
{
    const char *const source;
    const char *const content;
    const size_t size;

    struct ini_file *const inifile;
    struct ini_section *current_section;
    size_t pos;
    size_t line;
    enum parser_state state;
};

void inifile_new(struct ini_file *inifile)
{
    log_assert(inifile != NULL);

    inifile->sections_head = NULL;
    inifile->sections_tail = NULL;
}

static inline char peek_character(const struct parser_data *data)
{
    return data->content[data->pos];
}

/*!
 * Assume current character is newline, so increase position and line number.
 *
 * \retval #SKIP_RESULT_EOL Next character is beyond the line feed.
 */
static enum skip_result enter_next_line(struct parser_data *data)
{
    log_assert(peek_character(data) == '\n');

    ++data->pos;
    ++data->line;

    return SKIP_RESULT_EOL;
}

/*!
 * Skip all whitespace characters from current position in current line.
 *
 * \retval #SKIP_RESULT_OK  Skipped spaces, next character is available.
 * \retval #SKIP_RESULT_EOL Skipped spaces until newline, next character is
 *                          beyond the line feed.
 * \retval #SKIP_RESULT_EOF End of file reached.
 */
static enum skip_result skip_spaces(struct parser_data *data)
{
    for(/* nothing */; data->pos < data->size; ++data->pos)
    {
        const char ch = peek_character(data);

        if(!isblank(ch))
            return ch != '\n' ? SKIP_RESULT_OK : enter_next_line(data);
    }

    return SKIP_RESULT_EOF;
}

/*!
 * Skip all whitespace characters from current position in current line, going
 * from right to left.
 *
 * \retval #SKIP_RESULT_OK  Skipped spaces, next character is available.
 * \retval #SKIP_RESULT_EOL Next character is beyond the line feed.
 */
static enum skip_result skip_spaces_reverse(struct parser_data *data)
{
    while(1)
    {
        const char ch = peek_character(data);

        if(!isblank(ch))
        {
            if(ch != '\n')
                return SKIP_RESULT_OK;

            ++data->pos;

            return SKIP_RESULT_EOL;
        }

        if(data->pos > 0)
            --data->pos;
        else
            break;
    }

    return SKIP_RESULT_EOL;
}

/*!
 * Skip characters until given character or end of line is reached.
 *
 * \retval #SKIP_RESULT_OK  Found given character, current position points to
 *                          the character.
 * \retval #SKIP_RESULT_EOL Skipped characters until newline, next character is
 *                          beyond the line feed. The return code is
 *                          #SKIP_RESULT_OK in case \p until was the newline
 *                          character, not #SKIP_RESULT_EOL.
 * \retval #SKIP_RESULT_EOF End of file reached.
 */
static enum skip_result skip_until(struct parser_data *data, char until)
{
    for(/* nothing */; data->pos < data->size; ++data->pos)
    {
        const char ch = peek_character(data);

        if(ch == until)
            return SKIP_RESULT_OK;

        if(ch == '\n')
            return enter_next_line(data);
    }

    return SKIP_RESULT_EOF;
}

/*!
 * Skip until next line is reached.
 *
 * \retval #SKIP_RESULT_EOL Skipped characters until newline, next character is
 *                          beyond the line feed.
 * \retval #SKIP_RESULT_EOF End of file reached.
 */
static enum skip_result skip_line(struct parser_data *data)
{
    for(/* nothing */; data->pos < data->size; ++data->pos)
    {
        if(peek_character(data) == '\n')
            return enter_next_line(data);
    }

    return SKIP_RESULT_EOF;
}

/*!
 * Partial format string constant for uniform error messages.
 */
#define ERROR_LOCATION_FMTSTR  " (line %zu in \"%s\")"

/*!
 * Recognize beginning of a section header.
 *
 * Possible next states:
 * - #STATE_EXPECT_SECTION_NAME  on success, causing the parser to read the
 *                               rest of the line.
 * - #STATE_EXPECT_SECTION_BEGIN on error, causing the parser to find the next
 *                               section header.
 */
static int parse_section_begin(struct parser_data *data)
{
    log_assert(data->state == STATE_EXPECT_SECTION_BEGIN);

    switch(skip_spaces(data))
    {
      case SKIP_RESULT_EOF:
      case SKIP_RESULT_EOL:
        return 0;

      case SKIP_RESULT_OK:
        break;
    }

    if(peek_character(data) != '[')
    {
        msg_error(EINVAL, LOG_ERR,
                  "Expected begin of section, got junk" ERROR_LOCATION_FMTSTR,
                  data->line, data->source);
        return 1;
    }

    ++data->pos;
    data->state = STATE_EXPECT_SECTION_NAME;

    return 0;
}

/*!
 * Read section name, closing bracket, and rest of line.
 *
 * Possible next states:
 * - #STATE_EXPECT_ASSIGNMENT    on success, causing the parser to read
 *                               key/value pairs for this section.
 * - #STATE_EXPECT_SECTION_BEGIN on error, causing the parser to find the next
 *                               section header.
 */
static int parse_section_name(struct parser_data *data)
{
    log_assert(data->state == STATE_EXPECT_SECTION_NAME);

    const size_t start_of_name = data->pos;

    /* in case of any error... */
    data->state = STATE_EXPECT_SECTION_BEGIN;

    switch(skip_until(data, ']'))
    {
      case SKIP_RESULT_EOF:
        msg_error(EINVAL, LOG_ERR,
                  "End of file within section header" ERROR_LOCATION_FMTSTR,
                  data->line, data->source);
        return 0;

      case SKIP_RESULT_EOL:
        msg_error(EINVAL, LOG_ERR,
                  "End of line within section header" ERROR_LOCATION_FMTSTR,
                  data->line - 1, data->source);
        return 0;

      case SKIP_RESULT_OK:
        break;
    }

    const size_t length = data->pos - start_of_name;

    if(length == 0)
    {
        msg_error(EINVAL, LOG_ERR, "Empty section name" ERROR_LOCATION_FMTSTR,
                  data->line, data->source);
        return 1;
    }

    ++data->pos;

    switch(skip_spaces(data))
    {
      case SKIP_RESULT_OK:
        msg_error(EINVAL, LOG_ERR,
                  "Got junk after section header" ERROR_LOCATION_FMTSTR,
                  data->line, data->source);
        return 1;

      case SKIP_RESULT_EOF:
      case SKIP_RESULT_EOL:
        break;
    }

    data->current_section =
        inifile_new_section(data->inifile,
                            data->content + start_of_name, length);

    if(data->current_section == NULL)
        return -1;

    data->state = STATE_EXPECT_ASSIGNMENT;

    return 0;
}

static int parse_key_or_value(struct parser_data *data, size_t start_of_token,
                              const char *what, size_t *length_of_token)
{
    if(data->pos == start_of_token)
    {
        msg_error(EINVAL, LOG_ERR, "Expected %s" ERROR_LOCATION_FMTSTR,
                  what, data->line, data->source);
        return -1;
    }

    const size_t temp = data->pos;

    --data->pos;

    (void)skip_spaces_reverse(data);
    log_assert(data->pos >= start_of_token);

    *length_of_token = data->pos + 1 - start_of_token;

    data->pos = temp + 1;

    return 0;
}

static int parse_nonempty_value(struct parser_data *const data,
                                const size_t start_of_key, const size_t length_of_key)
{
    const size_t start_of_value = data->pos;

    enum skip_result skipped = skip_until(data, '\n');

    switch(skipped)
    {
      case SKIP_RESULT_EOL:
        BUG("Unexpected skip result");
        return 1;

      case SKIP_RESULT_OK:
      case SKIP_RESULT_EOF:
        break;
    }

    size_t length_of_value;
    if(parse_key_or_value(data, start_of_value, "value", &length_of_value) < 0)
        return 1;

    struct ini_key_value_pair *kv =
        inifile_section_store_value(data->current_section,
                                    data->content + start_of_key,
                                    length_of_key,
                                    data->content + start_of_value,
                                    length_of_value);

    if(skipped == SKIP_RESULT_OK)
        ++data->line;

    return kv != NULL ? 0 : -1;
}

static int insert_empty_value_for_key(struct parser_data *const data,
                                const size_t start_of_key, const size_t length_of_key)
{
    struct ini_key_value_pair *kv =
        inifile_section_store_empty_value(data->current_section,
                                          data->content + start_of_key,
                                          length_of_key);

    return kv != NULL ? 0 : -1;
}

/*!
 * Read key/value pair.
 *
 * Possible next states:
 * - #STATE_EXPECT_ASSIGNMENT    either on success or on error, causing the
 *                               parser to read the next key/value pair for
 *                               this section.
 * - #STATE_EXPECT_SECTION_BEGIN in case a '[' character was found starting a
 *                               line.
 */
static int parse_assignment(struct parser_data *data)
{
    log_assert(data->state == STATE_EXPECT_ASSIGNMENT);
    log_assert(data->current_section != NULL);

    switch(skip_spaces(data))
    {
      case SKIP_RESULT_EOF:
      case SKIP_RESULT_EOL:
        return 0;

      case SKIP_RESULT_OK:
        break;
    }

    if(peek_character(data) == '[')
    {
        data->current_section = NULL;
        data->state = STATE_EXPECT_SECTION_BEGIN;
        return 0;
    }

    const size_t start_of_key = data->pos;

    switch(skip_until(data, '='))
    {
      case SKIP_RESULT_EOF:
        ++data->line;
        /* fall-through */

      case SKIP_RESULT_EOL:
        msg_error(EINVAL, LOG_ERR,
                  "Expected assignment" ERROR_LOCATION_FMTSTR,
                  data->line - 1, data->source);
        return 0;

      case SKIP_RESULT_OK:
        break;
    }

    size_t length_of_key;
    if(parse_key_or_value(data, start_of_key, "key name", &length_of_key) < 0)
        return 1;

    int ret = -1;

    switch(skip_spaces(data))
    {
      case SKIP_RESULT_EOF:
        ++data->line;
        /* fall-through */

      case SKIP_RESULT_EOL:
        ret = insert_empty_value_for_key(data, start_of_key, length_of_key);
        break;

      case SKIP_RESULT_OK:
        ret = parse_nonempty_value(data, start_of_key, length_of_key);
        break;
    }

    return ret;
}

/*!
 * Signature of handlers for parser states.
 */
typedef int (*handler_fn)(struct parser_data *);

/*!
 * Parse an INI file from memory using a previously set up parser data
 * structure.
 */
static int parse_memory(struct parser_data *data)
{
    static const handler_fn parser_state_handlers[STATE_LAST_PARSER_STATE + 1] =
    {
        parse_section_begin,
        parse_section_name,
        parse_assignment,
    };

    while(data->pos < data->size)
    {
        int ret = parser_state_handlers[data->state](data);

        if(ret == 1)
            skip_line(data);
        else if(ret < 0)
            return -1;
    }

    return 0;
}

int inifile_parse_from_file(struct ini_file *inifile, const char *filename)
{
    log_assert(inifile != NULL);
    log_assert(filename != NULL);

    struct os_mapped_file_data mapped;

    if(os_map_file_to_memory(&mapped, filename) < 0)
    {
        inifile_new(inifile);
        return 1;
    }

    int ret = inifile_parse_from_memory(inifile, filename,
                                        mapped.ptr, mapped.length);

    os_unmap_file(&mapped);

    return ret;
}

int inifile_parse_from_memory(struct ini_file *inifile, const char *source,
                              const char *content, size_t size)
{
    log_assert(inifile != NULL);
    log_assert(content != NULL);

    inifile_new(inifile);

    struct parser_data data =
    {
        .source = source,
        .content = content,
        .size = size,
        .inifile = inifile,
        .current_section = NULL,
        .pos = 0,
        .line = 1,
        .state = STATE_EXPECT_SECTION_BEGIN,
    };

    int ret = parse_memory(&data);

    if(ret < 0)
        inifile_free(inifile);

    return ret;
}

static void *parser_malloc(size_t size)
{
    void *ptr = malloc(size);

    if(ptr == NULL)
        msg_error(errno, LOG_ERR, "malloc() failed for %zu bytes", size);

    return ptr;
}

static void parser_free(void *ptr)
{
    if(ptr != NULL)
        free(ptr);
}

static char *parser_strdup(const char *string, size_t size)
{
    char *cp = parser_malloc(size + 1);

    if(cp == NULL)
        return NULL;

    memcpy(cp, string, size);
    cp[size] = '\0';

    return cp;
}

struct ini_section *inifile_new_section(struct ini_file *inifile,
                                        const char *name, size_t length)
{
    if(length == 0)
        length = strlen(name);

    if(length == 0)
        return NULL;

    struct ini_section *section = inifile_find_section(inifile, name, length);
    if(section != NULL)
        return section;

    section = parser_malloc(sizeof(*section));

    if(section == NULL)
        return NULL;

    section->next = NULL;
    section->values_head = NULL;
    section->name_length = length;
    section->name = parser_strdup(name, length);

    if(section->name == NULL)
    {
        parser_free(section);
        return NULL;
    }

    if(inifile->sections_head == NULL)
        inifile->sections_head = section;
    else
    {
        log_assert(inifile->sections_tail != NULL);
        inifile->sections_tail->next = section;
    }

    inifile->sections_tail = section;

    return section;
}

struct ini_section *inifile_find_section(const struct ini_file *inifile,
                                         const char *section_name,
                                         size_t section_name_length)
{
    log_assert(inifile != NULL);
    log_assert(section_name != NULL);

    if(section_name_length == 0)
        section_name_length = strlen(section_name);

    for(struct ini_section *s = inifile->sections_head; s != NULL; s = s->next)
    {
        if(s->name_length == section_name_length &&
           memcmp(s->name, section_name, section_name_length) == 0)
            return s;
    }

    return NULL;
}

int inifile_write_to_file(const struct ini_file *inifile,
                          const char *filename)
{
    log_assert(inifile != NULL);
    log_assert(filename != NULL);

    int fd = os_file_new(filename);

    if(fd < 0)
        return -1;

    for(const struct ini_section *s = inifile->sections_head; s != NULL; s = s->next)
    {
        if(os_write_from_buffer("[", 1, fd) < 0 ||
           os_write_from_buffer(s->name, s->name_length, fd) < 0 ||
           os_write_from_buffer("]\n", 2, fd) < 0)
        {
            goto error_exit;
        }

        for(const struct ini_key_value_pair *kv = s->values_head; kv != NULL; kv = kv->next)
        {
            const size_t value_length = strlen(kv->value);

            if(os_write_from_buffer(kv->key, kv->key_length, fd) < 0 ||
               os_write_from_buffer(" = ", 3, fd) < 0 ||
               (value_length > 0 && os_write_from_buffer(kv->value, value_length, fd) < 0) ||
               os_write_from_buffer("\n", 1, fd) < 0)
            {
                goto error_exit;
            }
        }
    }

    os_file_close(fd);

    return 0;

error_exit:
    msg_error(0, LOG_ERR,
              "Failed writing INI file \"%s\", deleting partially written file",
              filename);

    os_file_close(fd);
    os_file_delete(filename);

    return -1;
}

static void free_kv_pair(struct ini_key_value_pair *kv)
{
    parser_free(kv->key);
    parser_free(kv->value);
    parser_free(kv);
}

void inifile_free(struct ini_file *inifile)
{
    log_assert(inifile != NULL);

    struct ini_section *s = inifile->sections_head;

    if(s == NULL)
        return;

    for(struct ini_section *next_section = s->next; s != NULL; s = next_section)
    {
        next_section = s->next;

        struct ini_key_value_pair *kv = s->values_head;

        if(kv != NULL)
        {
            for(struct ini_key_value_pair *next_kv = kv->next; kv != NULL; kv = next_kv)
            {
                next_kv = kv->next;
                free_kv_pair(kv);
            }
        }

        parser_free(s->name);
        parser_free(s);
    }
}

static struct ini_key_value_pair *
do_store_value(struct ini_section *section,
               const char *key, size_t key_length,
               const char *value, size_t value_length)
{
    char *value_copy = parser_strdup(value, value_length);
    if(value_copy == NULL)
        return NULL;

    struct ini_key_value_pair *kv =
        inifile_section_lookup_kv_pair(section, key, key_length);

    if(kv != NULL)
    {
        parser_free(kv->value);
        kv->value = value_copy;
        return kv;
    }

    char *key_copy = parser_strdup(key, key_length);

    if(key_copy != NULL)
    {
        kv = parser_malloc(sizeof(*kv));

        if(kv != NULL)
        {
            kv->next = NULL;
            kv->key_length = key_length;
            kv->key = key_copy;
            kv->value = value_copy;

            if(section->values_head == NULL)
                section->values_head = kv;
            else
            {
                log_assert(section->values_tail != NULL);
                section->values_tail->next = kv;
            }

            section->values_tail = kv;
        }
    }

    if(kv == NULL)
    {
        parser_free(key_copy);
        parser_free(value_copy);
    }

    return kv;
}

struct ini_key_value_pair *
inifile_section_store_value(struct ini_section *section,
                            const char *key, size_t key_length,
                            const char *value, size_t value_length)
{
    log_assert(section != NULL);
    log_assert(key != NULL);
    log_assert(value != NULL);

    if(key_length == 0)
        key_length = strlen(key);

    if(key_length == 0)
        return NULL;

    if(value_length == 0)
        value_length = strlen(value);

    if(value_length == 0)
        return NULL;

    return do_store_value(section, key, key_length, value, value_length);
}

struct ini_key_value_pair *
inifile_section_store_empty_value(struct ini_section *section,
                                  const char *key, size_t key_length)
{
    log_assert(section != NULL);
    log_assert(key != NULL);

    if(key_length == 0)
        key_length = strlen(key);

    if(key_length == 0)
        return NULL;

    return do_store_value(section, key, key_length, "", 0);
}

bool inifile_section_remove_value(struct ini_section *section,
                                  const char *key, size_t key_length)
{
    log_assert(section != NULL);
    log_assert(key != NULL);

    if(key_length == 0)
        key_length = strlen(key);

    struct ini_key_value_pair *preceding = NULL;

    for(struct ini_key_value_pair *kv = section->values_head; kv != NULL; kv = kv->next)
    {
        if(kv->key_length == key_length && memcmp(kv->key, key, key_length) == 0)
        {
            if(section->values_head == kv)
                section->values_head = kv->next;
            else
                preceding->next = kv->next;

            if(section->values_tail == kv)
                section->values_tail = preceding;

            free_kv_pair(kv);

            return true;
        }

        preceding = kv;
    }

    return false;
}

struct ini_key_value_pair *
inifile_section_lookup_kv_pair(const struct ini_section *section,
                               const char *key, size_t key_length)
{
    log_assert(section != NULL);
    log_assert(key != NULL);

    if(key_length == 0)
        key_length = strlen(key);

    for(struct ini_key_value_pair *kv = section->values_head; kv != NULL; kv = kv->next)
    {
        if(kv->key_length == key_length && memcmp(kv->key, key, key_length) == 0)
            return kv;
    }

    return NULL;
}
