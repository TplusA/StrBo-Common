/*
 * Copyright (C) 2017, 2019, 2020, 2023  T+A elektroakustik GmbH & Co. KG
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

#include <sstream>
#include <limits>
#include <climits>
#include <cstring>
#include <glib.h>

#include "configuration.hh"

void Configuration::default_serialize(char *dest, size_t dest_size,
                                      const char *src, size_t src_length)
{
    if(dest == nullptr || dest_size == 0)
        return;

    if(src_length == 0 && src != nullptr)
        src_length = strlen(src);

    if(src == nullptr || src_length == 0)
    {
        *dest = '\0';
        return;
    }

    const size_t len = std::min(dest_size - 1, src_length);
    std::copy(src, src + len, dest);
    dest[len] = '\0';
}

void Configuration::default_serialize(char *dest, size_t dest_size,
                                      const std::string &src)
{
    if(dest == nullptr || dest_size == 0)
        return;

    const size_t len = std::min(dest_size - 1, src.length());
    std::copy(src.begin(), src.begin() + len, dest);
    dest[len] = '\0';
}

bool Configuration::default_deserialize(std::string &dest, const char *src)
{
    dest = src;
    return true;
}

GVariantWrapper Configuration::default_box(const char *src)
{
    return GVariantWrapper(g_variant_new_string(src));
}

GVariantWrapper Configuration::default_box(const std::string &src)
{
    return GVariantWrapper(g_variant_new_string(src.c_str()));
}

GVariantWrapper Configuration::default_box(uint16_t value)
{
    return GVariantWrapper(g_variant_new_uint16(value));
}

GVariantWrapper Configuration::default_box(uint32_t value)
{
    return GVariantWrapper(g_variant_new_uint32(value));
}

GVariantWrapper Configuration::default_box(uint64_t value)
{
    return GVariantWrapper(g_variant_new_uint64(value));
}

GVariantWrapper Configuration::default_box(bool value)
{
    return GVariantWrapper(g_variant_new_boolean(value));
}

bool Configuration::default_unbox(std::string &dest, GVariantWrapper &&src)
{
    if(!g_variant_is_of_type(GVariantWrapper::get(src), G_VARIANT_TYPE_STRING))
        return false;

    dest = g_variant_get_string(GVariantWrapper::get(src), nullptr);
    return true;
}

bool Configuration::default_unbox(uint16_t &value, GVariantWrapper &&src)
{
    if(!g_variant_is_of_type(GVariantWrapper::get(src), G_VARIANT_TYPE_UINT16))
        return false;

    value = g_variant_get_uint16(GVariantWrapper::get(src));
    return true;
}

bool Configuration::default_unbox(uint32_t &value, GVariantWrapper &&src)
{
    if(!g_variant_is_of_type(GVariantWrapper::get(src), G_VARIANT_TYPE_UINT32))
        return false;

    value = g_variant_get_uint32(GVariantWrapper::get(src));
    return true;
}

bool Configuration::default_unbox(uint64_t &value, GVariantWrapper &&src)
{
    if(!g_variant_is_of_type(GVariantWrapper::get(src), G_VARIANT_TYPE_UINT64))
        return false;

    value = g_variant_get_uint64(GVariantWrapper::get(src));
    return true;
}

bool Configuration::default_unbox(bool &value, GVariantWrapper &&src)
{
    if(!g_variant_is_of_type(GVariantWrapper::get(src), G_VARIANT_TYPE_BOOLEAN))
        return false;

    value = g_variant_get_boolean(GVariantWrapper::get(src));
    return true;
}

template <typename T>
static inline void serialize_uint(char *dest, size_t dest_size, const T value)
{
    std::ostringstream ss;

    ss << value;
    Configuration::default_serialize(dest, dest_size, ss.str());
}

template <typename T>
static inline bool deserialize_uint(T &value, const char *src)
{
    char *endptr = nullptr;
    unsigned long long temp = strtoull(src, &endptr, 10);

    if(*endptr != '\0' || (temp == ULLONG_MAX && errno == ERANGE))
        return false;

    if(temp > std::numeric_limits<T>::max())
        return false;

    value = temp;

    return true;
}

void Configuration::default_serialize(char *dest, size_t dest_size, uint16_t value)
{
    serialize_uint(dest, dest_size, value);
}

bool Configuration::default_deserialize(uint16_t &value, const char *src)
{
    return deserialize_uint(value, src);
}

void Configuration::default_serialize(char *dest, size_t dest_size, uint32_t value)
{
    serialize_uint(dest, dest_size, value);
}

bool Configuration::default_deserialize(uint32_t &value, const char *src)
{
    return deserialize_uint(value, src);
}

void Configuration::default_serialize(char *dest, size_t dest_size, uint64_t value)
{
    serialize_uint(dest, dest_size, value);
}

bool Configuration::default_deserialize(uint64_t &value, const char *src)
{
    return deserialize_uint(value, src);
}

void Configuration::default_serialize(char *dest, size_t dest_size, bool value)
{
    if(value)
        default_serialize(dest, dest_size, "true");
    else
        default_serialize(dest, dest_size, "false");
}

bool Configuration::default_deserialize(bool &value, const char *src)
{
    std::string dest;

    default_deserialize(dest, src);
    value = dest == "true";
    return true;
}

size_t Configuration::find_varname_offset_in_keyname(const char *key)
{
    const char *found = strrchr(key, ':');
    return (found == nullptr) ? 0 : found - key + 1;
}
