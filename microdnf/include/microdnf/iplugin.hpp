/*
Copyright Contributors to the libdnf project.

This file is part of libdnf: https://github.com/rpm-software-management/libdnf/

Libdnf is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 2.1 of the License, or
(at your option) any later version.

Libdnf is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with libdnf.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef MICRODNF_PLUGIN_IPLUGIN_HPP
#define MICRODNF_PLUGIN_IPLUGIN_HPP

#include "context.hpp"

#include <cstdint>


namespace microdnf {

struct Version {
    std::uint16_t major;
    std::uint16_t minor;
    std::uint16_t micro;
};

static constexpr Version PLUGIN_API_VERSION{.major = 0, .minor = 1, .micro = 0};

class IPlugin {
public:
    IPlugin() = default;
    virtual ~IPlugin() = default;

    IPlugin(const IPlugin &) = delete;
    IPlugin(IPlugin &&) = delete;
    IPlugin & operator=(const IPlugin &) = delete;
    IPlugin & operator=(IPlugin &&) = delete;

    /// Returns the version of the API supported by the plugin. It can be called at any time.
    virtual Version get_api_version() const noexcept = 0;

    /// Returns the name of the plugin. It can be called at any time.
    virtual const char * get_name() const noexcept = 0;

    /// Gets the plugin version. It can be called at any time.
    virtual Version get_version() const noexcept = 0;

    /// Gets the value of the attribute from the plugin. Returns nullptr if the attribute does not exist.
    /// It can be called at any time.
    virtual const char * get_attribute(const char * name) const noexcept = 0;

    /// Plugin initialization.
    virtual void init(microdnf::Context * context) = 0;

    virtual std::vector<std::unique_ptr<libdnf::cli::session::Command>> create_commands(libdnf::cli::session::Command & parent) = 0;

    /// It is called when a hook is reached. The argument describes what happened.
    //virtual bool hook(HookId hook_id) = 0;

    /// Finish the plugin and release all resources obtained by the init method and in hooks.
    virtual void finish() noexcept = 0;
};

}  // namespace dnf5::plugin


extern "C" {

/// Returns the version of the API supported by the plugin.
/// Same result as IPlugin::get_api_version(), but can be called without creating an IPlugin instance.
microdnf::Version microdnf_plugin_get_api_version(void);

/// Returns the name of the plugin. It can be called at any time.
/// Same result as IPlugin::get_name(), but can be called without creating an IPlugin instance.
const char * microdnf_plugin_get_name(void);

/// Returns the version of the plugin. It can be called at any time.
/// Same result as IPlugin::get_version(), but can be called without creating an IPlugin instance.
microdnf::Version microdnf_plugin_get_version(void);

/// Creates a new plugin instance. Passes the API version to the plugin.
microdnf::IPlugin * microdnf_plugin_new_instance(microdnf::Version api_version);

/// Deletes plugin instance.
void microdnf_plugin_delete_instance(microdnf::IPlugin * plugin_instance);
}

#endif
