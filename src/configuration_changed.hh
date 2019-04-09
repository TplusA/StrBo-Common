/*
 * Copyright (C) 2017, 2019  T+A elektroakustik GmbH & Co. KG
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

#ifndef CONFIGURATION_CHANGED_HH
#define CONFIGURATION_CHANGED_HH

#include <cstdint>

#include "configuration_base.hh"

namespace Configuration
{

class ConfigChangedIface
{
  protected:
    explicit ConfigChangedIface() {}

  public:
    ConfigChangedIface(const ConfigChangedIface &) = delete;
    ConfigChangedIface &operator=(const ConfigChangedIface &) = delete;

    virtual ~ConfigChangedIface() {}

  protected:
    virtual void update_begin() = 0;
    virtual void update_done(const char *origin) = 0;
};

template <typename ValuesT>
class ConfigChanged: public ConfigChangedIface
{
  public:
    class UpdateScope
    {
      private:
        ConfigChanged &changed_iface_;
        const char *origin_;

      public:
        UpdateScope(const UpdateScope &) = delete;
        UpdateScope(UpdateScope &&) = default;
        UpdateScope &operator=(const UpdateScope &) = delete;

      private:
        explicit UpdateScope(ConfigChanged &changed_iface, const char *origin):
            changed_iface_(changed_iface),
            origin_(origin)
        {
            changed_iface_.update_begin();
        }

        friend class ConfigChanged;

      public:
        ~UpdateScope() { changed_iface_.update_done(origin_); }

        UpdateSettings<ValuesT> &operator()() { return changed_iface_.get_update_settings_iface(); }
    };

  protected:
    explicit ConfigChanged() {}

  public:
    ConfigChanged(const ConfigChanged &) = delete;
    ConfigChanged &operator=(const ConfigChanged &) = delete;

    virtual ~ConfigChanged() {}

    UpdateScope get_update_scope(const char *origin)
    {
        return UpdateScope(*this, origin);
    }

  protected:
    virtual UpdateSettings<ValuesT> &get_update_settings_iface() = 0;

};

template <typename ValuesT>
static inline const ConfigChanged<ValuesT> &downcast(const ConfigChangedIface &iface)
{
    return static_cast<const ConfigChanged<ValuesT> &>(iface);
}

template <typename ValuesT>
static inline ConfigChanged<ValuesT> &downcast(ConfigChangedIface &iface)
{
    return static_cast<ConfigChanged<ValuesT> &>(iface);
}

}

#endif /* !CONFIGURATION_CHANGED_HH */
