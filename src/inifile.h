/*
 * Copyright (C) 2015, 2019  T+A elektroakustik GmbH & Co. KG
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

#ifndef INIFILE_H
#define INIFILE_H

#include <stdbool.h>
#include <unistd.h>

/*!
 * \addtogroup inifile INI files
 */
/*!@{*/

/*!
 * Simple structure holding a key and a value.
 *
 * Also a singly linked list of key/value pairs.
 */
struct ini_key_value_pair
{
    struct ini_key_value_pair *next;
    size_t key_length;
    char *key;
    char *value;
};

/*!
 * Structure that represents an INI file section.
 *
 * The section name is stored along with a singly linked list of key/value
 * pairs. For quick appending to the list, there is a pointer to the list's
 * tail element.
 *
 * The structure itself is also part of a singly linked list (of sections).
 */
struct ini_section
{
    struct ini_section *next;
    struct ini_key_value_pair *values_head;
    struct ini_key_value_pair *values_tail;
    size_t name_length;
    char *name;
};

/*!
 * Structure that represents an INI file.
 *
 * This is nothing more than a singly linked list of section structures. The
 * section structures only store their names and a list of keys and values.
 */
struct ini_file
{
    struct ini_section *sections_head;
    struct ini_section *sections_tail;
};

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * Initialize INI file structure.
 *
 * \param inifile
 *     A structure to be initialized. The structure must have been allocated by
 *     the caller. This function does not allocate any memory.
 */
void inifile_new(struct ini_file *inifile);

/*!
 * Parse an INI file.
 *
 * It is not necessary to call #inifile_new() before calling this function.
 *
 * \param inifile
 *     A structure to be filled by this function. The structure must have been
 *     allocated by the caller.
 *
 * \param filename
 *     Name of an INI file.
 *
 * \retval 0 on success
 * \retval 1 if file does not exist
 * \retval -1 on error
 */
int inifile_parse_from_file(struct ini_file *inifile, const char *filename);

/*!
 * Parse an INI file from memory.
 *
 * It is not necessary to call #inifile_new() before calling this function.
 *
 * \param inifile
 *     A structure to be filled by this function. The structure must have been
 *     allocated by the caller.
 *
 * \param source
 *     Name of the data source (usually a filename) for diagnostics messages.
 *
 * \param content
 *     An INI file read or mapped to memory.
 *
 * \param size
 *     Number of bytes that \p content is pointing to.
 *
 * \returns
 *     0 on success, -1 on hard error (out of memory). The parser attempts to
 *     work in a non-stop mode, ignoring parsing errors by skipping over lines
 *     the parser didn't understand.
 */
int inifile_parse_from_memory(struct ini_file *inifile, const char *source,
                              const char *content, size_t size);

/*!
 * Allocate a new section structure for given name.
 *
 * If there is already a section of that name, then that section structure is
 * returned. That is, a caller may not assume that the returned section
 * structure is empty.
 *
 * \param inifile
 *     INI file structure the section should become part of.
 *
 * \param name
 *     Section name as written to the section header.
 *
 * \param length
 *     Length of the section name in number of characters, without the trailing
 *     zero-terminator. The function will call \c strlen() for \p name in case
 *     0 is passed.
 *
 * \returns
 *     A section with the given name (either newly allocated or found in the
 *     INI file structure), or \c NULL in case no memory could be allocated.
 */
struct ini_section *inifile_new_section(struct ini_file *inifile,
                                        const char *name, size_t length);

/*!
 * Remove section from file, freeing all resources occupied by it.
 *
 * \param inifile
 *     INI file structure the section should be removed from.
 *
 * \param section
 *     Pointer to the section to be removed. The section must be part of
 *     \p inifile.
 *
 * returns
 *     True on success, false in case the section does not exist.
 */
bool inifile_remove_section(struct ini_file *inifile,
                            struct ini_section *section);

/*!
 * Remove section from file, freeing all resources occupied by it.
 *
 * \param inifile
 *     INI file structure the section should be removed from.
 *
 * \param section_name, section_name_length
 *     Name of the section and length of the section_name. Parameter
 *     \p section_name_length may be 0, in which case the length will be
 *     determined by a call of \c strlen() for \p section_name.
 *
 * returns
 *     True on success, false in case the section does not exist.
 */
bool inifile_remove_section_by_name(struct ini_file *inifile,
                                    const char *section_name,
                                    size_t section_name_length);

/*!
 * Find section by name.
 *
 * Parameters as for #inifile_new_section().
 *
 * \returns
 *     The section of given name, or \c NULL in case there is no such section.
 */
struct ini_section *inifile_find_section(const struct ini_file *inifile,
                                         const char *section_name,
                                         size_t section_name_length);

/*!
 * Write INI file to filesystem.
 *
 * \returns
 *     0 on success, -1 on error.
 */
int inifile_write_to_file(const struct ini_file *inifile,
                          const char *filename);

/*!
 * Free an INI file structure.
 *
 * Note that the memory \p inifile is pointing to is \e not freed by this
 * function. This allows callers to pass stack-allocated objects as
 * \p inifile.
 */
void inifile_free(struct ini_file *inifile);

/*!
 * Store a value with given key in given section.
 *
 * Note that passing a value for a key that is already stored in the section
 * replaces the previously stored value. It is not possible to append or
 * accumulate values with same keys.
 *
 * \param section
 *     The section the key/value pair should be stored in.
 *
 * \param key, key_length
 *     Key name and length of the key name in number of characters, without the
 *     trailing zero-terminator. The function will call \c strlen() for \p key
 *     in case 0 is passed as \p key_length.
 *
 * \param value, value_length
 *     Value and length of the value in number of characters, without the
 *     trailing zero-terminator. The function will call \c strlen() for
 *     \p value in case 0 is passed as \p value_length.
 *
 * \returns
 *     A structure with zero-terminated copies of key and values, or \c NULL in
 *     case no memory could be allocated.
 *
 * \see #inifile_section_store_empty_value()
 */
struct ini_key_value_pair *
inifile_section_store_value(struct ini_section *section,
                            const char *key, size_t key_length,
                            const char *value, size_t value_length);

/*!
 * Store empty value with given key in given section.
 *
 * This is just like #inifile_section_store_value(), but specifically intended
 * for storing an empty value (since the #inifile_section_store_value()
 * interface does not allow for storing empty values).
 */
struct ini_key_value_pair *
inifile_section_store_empty_value(struct ini_section *section,
                                  const char *key, size_t key_length);

/*!
 * Remove value with given key from the given section.
 *
 * \param section
 *     The section the key/value pair should be removed from.
 *
 * \param key, key_length
 *     Key name and length of the key name in number of characters, without the
 *     trailing zero-terminator. The function will call \c strlen() for \p key
 *     in case 0 is passed as \p key_length.
 *
 * \returns
 *     True on success, false in case \p key was not found in the section.
 */
bool inifile_section_remove_value(struct ini_section *section,
                                  const char *key, size_t key_length);

/*!
 * Lookup value by key name.
 *
 * \param section
 *     The section the key should be searched in.
 *
 * \param key, key_length
 *     Key name and length of the key name in number of characters, without the
 *     trailing zero-terminator. The function will call \c strlen() for \p key
 *     in case 0 is passed as \p key_length.
 *
 * \returns
 *     A structure holding the key and its corresponding value, or \c NULL in
 *     case the key was not found in the section.
 */
struct ini_key_value_pair *
inifile_section_lookup_kv_pair(const struct ini_section *section,
                               const char *key, size_t key_length);

#ifdef __cplusplus
}
#endif

/*!@}*/

#endif /* !INIFILE_H */
