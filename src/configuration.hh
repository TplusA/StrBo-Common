/*
 * Copyright (C) 2017, 2019, 2020, 2022  T+A elektroakustik GmbH & Co. KG
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

#ifndef CONFIGURATION_HH
#define CONFIGURATION_HH

#include <string>
#include <vector>
#include <functional>
#include <cstring>
#include <algorithm>

#include "configuration_changed.hh"
#include "gvariantwrapper.hh"
#include "inifile.h"
#include "messages.h"

/* for GVariant serialization/deserialization */
struct _GVariantType;

namespace Configuration
{

static inline bool key_to_local_key(const char *&key,
                                    const char *expected_owner,
                                    size_t expected_owner_length)
{
    if(key[0] != '@')
        return true;

    if(expected_owner_length == 0)
        expected_owner_length = strlen(expected_owner);

    if(strncmp(key + 1, expected_owner, expected_owner_length) == 0 &&
        key[expected_owner_length + 1] == ':')
    {
        key += expected_owner_length + 1;
        return true;
    }

    return false;
}

static inline bool key_extract_section_name(const char *const key,
                                            const char *expected_owner,
                                            size_t expected_owner_length,
                                            const char **out_local_key,
                                            std::string &section)
{
    const char *local_key = key;

    if(!key_to_local_key(local_key, expected_owner, expected_owner_length))
        return false;

    if(local_key[0] != ':')
        return false;

    if(out_local_key != nullptr)
        *out_local_key = local_key;

    section.clear();

    for(size_t i = 1; local_key[i] != '\0' && local_key[i] != ':'; ++i)
        section.push_back(local_key[i]);

    return true;
}

template <typename ValuesT>
class ConfigManager: public ConfigChanged<ValuesT>
{
  public:
    using UpdatedCallback = std::function<void(const char *,
                                               const std::array<bool, ValuesT::NUMBER_OF_KEYS> &)>;

  private:
    const char *const configuration_file_;
    const ValuesT &default_settings_;

    bool is_updating_;
    Settings<ValuesT> settings_;
    UpdateSettings<ValuesT> update_settings_;

    UpdatedCallback configuration_updated_callback_;

  public:
    ConfigManager(const ConfigManager &) = delete;
    ConfigManager &operator=(const ConfigManager &) = delete;

    explicit ConfigManager(const char *configuration_file,
                           const ValuesT &defaults):
        configuration_file_(configuration_file),
        default_settings_(defaults),
        is_updating_(false),
        update_settings_(settings_)
    {}

    void set_updated_notification_callback(UpdatedCallback &&callback)
    {
        configuration_updated_callback_ = callback;
    }

    bool load()
    {
        msg_log_assert(!is_updating_);

        ValuesT loaded(default_settings_);

        if(try_load(configuration_file_, loaded))
            settings_.put(loaded);
        else
            reset_to_defaults();

        return settings_.is_valid();
    }

    void reset_to_defaults()
    {
        msg_log_assert(!is_updating_);
        settings_.put(default_settings_);
    }

    static const char *get_database_name() { return ValuesT::DATABASE_NAME; }
    const ValuesT &values() const { return settings_.values(); }

    static std::vector<const char *> keys()
    {
        std::vector<const char *> result;
        std::transform(ValuesT::all_keys.begin(), ValuesT::all_keys.end(),
                       std::back_inserter(result),
                       [] (const auto &k) { return k.name_.c_str(); });
        return result;
    }

    GVariantWrapper lookup_boxed(const char *key) const
    {
        if(!to_local_key(key))
            return GVariantWrapper();

        const size_t requested_key_length(strlen(key));
        const auto &it(std::find_if(
            ValuesT::all_keys.begin(), ValuesT::all_keys.end(),
            [key, requested_key_length] (const auto &k)
            {
                return k.name_.length() == requested_key_length &&
                       strcmp(k.name_.c_str(), key) == 0;
            }));

        return it != ValuesT::all_keys.end()
            ? it->box(settings_.values())
            : GVariantWrapper();
    }

    static bool to_local_key(const char *&key)
    {
        return key_to_local_key(key, ValuesT::OWNER_NAME,
                                sizeof(ValuesT::OWNER_NAME) - 1);
    }

    static bool is_matching_key(const char *key)
    {
        std::string section;

        return key_extract_section_name(key, ValuesT::OWNER_NAME,
                                        sizeof(ValuesT::OWNER_NAME) - 1,
                                        nullptr, section) &&
            section == ValuesT::CONFIGURATION_SECTION_NAME;
    }

  protected:
    UpdateSettings<ValuesT> &get_update_settings_iface() final override
    {
        return update_settings_;
    }

    void update_begin() final override
    {
        msg_log_assert(!is_updating_);
        is_updating_ = true;
    }

    void update_done(const char *origin) final override
    {
        msg_log_assert(is_updating_);
        is_updating_ = false;

        if(settings_.is_changed())
        {
            store();

            if(configuration_updated_callback_ != nullptr)
                configuration_updated_callback_(origin, settings_.get_changed_ids());

            settings_.changes_processed_notification();
        }
    }

  private:
    static bool try_load(const char *file, ValuesT &values)
    {
        struct ini_file ini;

        if(inifile_parse_from_file(&ini, file) != 0)
            return false;

        auto *section =
            inifile_find_section(&ini, ValuesT::CONFIGURATION_SECTION_NAME,
                                 sizeof(ValuesT::CONFIGURATION_SECTION_NAME) - 1);

        if(section == nullptr)
        {
            inifile_free(&ini);
            return false;
        }

        for(const auto &k : ValuesT::all_keys)
        {
            auto *kv = inifile_section_lookup_kv_pair(section,
                                                      k.name_.c_str() + k.varname_offset_,
                                                      k.name_.length() - k.varname_offset_);

            if(kv != nullptr)
                k.write(values, kv->value);
        }

        inifile_free(&ini);

        return true;
    }

    static bool try_store(const char *file, const ValuesT &values)
    {
        struct ini_file ini;
        struct ini_section *section;

        if(inifile_parse_from_file(&ini, file) == 0)
            section =
                inifile_find_section(&ini, ValuesT::CONFIGURATION_SECTION_NAME,
                                     sizeof(ValuesT::CONFIGURATION_SECTION_NAME) - 1);
        else
        {
            inifile_new(&ini);
            section = nullptr;
        }

        if(section == nullptr)
        {
            section =
                inifile_new_section(&ini, ValuesT::CONFIGURATION_SECTION_NAME,
                                    sizeof(ValuesT::CONFIGURATION_SECTION_NAME) - 1);

            if(section == nullptr)
            {
                inifile_free(&ini);
                return false;
            }
        }

        char buffer[128];

        for(const auto &k : ValuesT::all_keys)
        {
            k.read(buffer, sizeof(buffer), values);
            if(buffer[0] != '\0')
                inifile_section_store_value(section,
                                            k.name_.c_str() + k.varname_offset_,
                                            k.name_.length() - k.varname_offset_,
                                            buffer, 0);
            else
                inifile_section_store_empty_value(section,
                                                  k.name_.c_str() + k.varname_offset_,
                                                  k.name_.length() - k.varname_offset_);
        }

        inifile_write_to_file(&ini, file);
        inifile_free(&ini);

        return true;
    }

    bool store()
    {
        msg_log_assert(!is_updating_);
        return try_store(configuration_file_, settings_.values());
    }
};

void default_serialize(char *dest, size_t dest_size, const char *src, size_t src_length = 0);
void default_serialize(char *dest, size_t dest_size, const std::string &src);
void default_serialize(char *dest, size_t dest_size, uint16_t value);
void default_serialize(char *dest, size_t dest_size, uint32_t value);
void default_serialize(char *dest, size_t dest_size, uint64_t value);
void default_serialize(char *dest, size_t dest_size, bool value);

GVariantWrapper default_box(const char *src);
GVariantWrapper default_box(const std::string &src);
GVariantWrapper default_box(uint16_t value);
GVariantWrapper default_box(uint32_t value);
GVariantWrapper default_box(uint64_t value);
GVariantWrapper default_box(bool value);

bool default_deserialize(std::string &dest, const char *src);
bool default_deserialize(uint16_t &value, const char *src);
bool default_deserialize(uint32_t &value, const char *src);
bool default_deserialize(uint64_t &value, const char *src);
bool default_deserialize(bool &value, const char *src);

bool default_unbox(std::string &dest, GVariantWrapper &&src);
bool default_unbox(uint16_t &value, GVariantWrapper &&src);
bool default_unbox(uint32_t &value, GVariantWrapper &&src);
bool default_unbox(uint64_t &value, GVariantWrapper &&src);
bool default_unbox(bool &value, GVariantWrapper &&src);

template <typename ValuesT, typename Traits>
static void serialize_value(char *dest, size_t dest_size, const ValuesT &v)
{
    ::Configuration::default_serialize(dest, dest_size, v.*Traits::field);
}

template <typename ValuesT, typename Traits>
static bool deserialize_value(ValuesT &v, const char *value)
{
    return ::Configuration::default_deserialize(v.*Traits::field, value);
}

template <typename ValuesT, typename Traits>
static GVariantWrapper box_value(const ValuesT &v)
{
    return ::Configuration::default_box(v.*Traits::field);
}

}

#endif /* !CONFIGURATION_HH */
