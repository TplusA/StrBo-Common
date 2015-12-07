/*
 * Copyright (C) 2017, 2018, 2019  T+A elektroakustik GmbH & Co. KG
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

#ifndef CONFIGURATION_BASE_HH
#define CONFIGURATION_BASE_HH

#include <string>
#include <functional>

#include "gvariantwrapper.hh"

namespace Configuration
{

/*!
 * \internal
 * \brief Interface for marking beginning and end of atomic changes.
 */
class ConfigChangedIface;

/*!
 * Base class template for RAII-style update scopes.
 *
 * Function member #Configuration::ConfigChanged::get_update_scope() must to be
 * called by client code to start updating managed values.
 *
 * Function member #Configuration::ConfigChanged::get_update_settings_iface()
 * must be implemented by classed deriving from this class template to enable
 * setting up the scope requested by client code.
 */
template <typename ValuesT>
class ConfigChanged;

/*!
 * Management of a structure of values.
 *
 * This class template takes care of mapping a structure of values to an INI
 * file, and vice versa. It makes use of several other helper class templates
 * which depend on the managed structure. These helpers take care of
 * serialization and deserialization of structure members, and help with
 * tracking value changes.
 *
 * \tparam ValuesT
 *     Type of the managed structure.
 *
 * There are some requirements to be fulfilled by \p ValuesT:
 * - It must define an \c enum \c class name \c KeyID that assigns an
 *   identifier to each value in the structure.
 * - It must define a \c constexpr member named \c NUMBER_OF_KEYS of type
 *   \c size_t that gives the number of values in the structure.
 * - It must define a non-nullptr \c constexpr C string member named
 *   \c CONFIGURATION_SECTION_NAME.
 * - It must define a \c static \c const member name \c all_keys of type
 *   \c std::array, storing exactly \c NUMBER_OF_KEYS objects derived from
 *   #Configuration::ConfigKeyBase.
 * - The structure must have a default constructor.
 */
template <typename ValuesT>
class ConfigManager;

/*!
 * Wrapper with well-defined API around a table (structure) of values.
 *
 * Values may be updated using the #Configuration::Settings::update() function
 * member. It will keep track of changed values which may be queried via
 * #Configuration::Settings::get_changed_ids().
 *
 * Objects of this type are not accessed directly by client code. There is,
 * however, one #Configuration::Settings object embedded into each
 * #Configuration::ConfigManager, and it may be accessed only through the
 * #Configuration::UpdateSettings class template. A specialization of the
 * #Configuration::UpdateSettings class template must be provided along the
 * definition of the \p ValuesT type.
 *
 * \tparam ValuesT
 *     Type of the managed structure.
 */
template <typename ValuesT>
class Settings;

/*!
 * Interface to managed table of values for the purpose of updating.
 *
 * Structure members are set through table-specific function members to enable
 * tracking of changes.
 */
template <typename ValuesT>
class UpdateSettings;

enum class InsertResult
{
    UPDATED,
    UNCHANGED,
    KEY_UNKNOWN,
    VALUE_TYPE_INVALID,  //<! Type of given value is invalid/not supported
    VALUE_INVALID,       //<! Value has correct type, but value is invalid
    PERMISSION_DENIED,

    LAST_CODE = PERMISSION_DENIED,
};

/*!
 * Base class template for configuration keys in a managed table.
 */
template <typename ValuesT>
class ConfigKeyBase
{
  public:
    using Serializer = std::function<void(char *, size_t, const ValuesT &)>;
    using Deserializer = std::function<bool(ValuesT &, const char *)>;
    using Boxer = std::function<GVariantWrapper(const ValuesT &)>;
    using Unboxer = std::function<InsertResult(UpdateSettings<ValuesT> &, GVariantWrapper &&)>;

    const typename ValuesT::KeyID id_;
    const std::string name_;
    const size_t varname_offset_;

    ConfigKeyBase(const ConfigKeyBase &) = delete;
    ConfigKeyBase(ConfigKeyBase &&) = default;
    ConfigKeyBase &operator=(const ConfigKeyBase &) = delete;

    explicit ConfigKeyBase(typename ValuesT::KeyID id, const char *name,
                           size_t varname_offset):
        id_(id),
        name_(name),
        varname_offset_(varname_offset)
    {}

    virtual ~ConfigKeyBase() {}

    virtual void read(char *dest, size_t dest_size, const ValuesT &src) const = 0;
    virtual bool write(ValuesT &dest, const char *src) const = 0;
    virtual GVariantWrapper box(const ValuesT &src) const = 0;
    virtual InsertResult unbox(UpdateSettings<ValuesT> &dest, GVariantWrapper &&src) const = 0;
};

size_t find_varname_offset_in_keyname(const char *key);

}

#endif /* !CONFIGURATION_BASE_HH */
