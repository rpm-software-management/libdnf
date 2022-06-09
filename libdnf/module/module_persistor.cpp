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

#include "module/module_persistor.hpp"

#include "utils/bgettext/bgettext-mark-domain.h"

#include "libdnf/module/module_errors.hpp"

#include <modulemd-2.0/modulemd.h>

#include <algorithm>
#include <iterator>
#include <memory>
#include <set>
#include <string>

namespace libdnf::module {


ModulePersistor::ModulePersistor(libdnf::system::State & system_state) : system_state(system_state) {}


struct ModulePersistor::RuntimeModuleState & ModulePersistor::get_runtime_module_state(
    const std::string & module_name) {
    try {
        return runtime_module_states.at(module_name);
    } catch (std::out_of_range &) {
        throw NoModuleError(M_("No such module: {}"), module_name);
    }
}


const ModuleState & ModulePersistor::get_state(const std::string & module_name) {
    return get_runtime_module_state(module_name).state;
}


const std::string & ModulePersistor::get_enabled_stream(const std::string & module_name) {
    return get_runtime_module_state(module_name).enabled_stream;
}


const std::vector<std::string> & ModulePersistor::get_installed_profiles(const std::string & module_name) {
    return get_runtime_module_state(module_name).installed_profiles;
}


std::vector<std::string> ModulePersistor::get_all_module_names() {
    std::vector<std::string> result;
    result.reserve(runtime_module_states.size());
    for (auto & item : runtime_module_states) {
        result.push_back(item.first);
    }
    return result;
}


bool ModulePersistor::change_state(const std::string & module_name, ModuleState state) {
    if (get_runtime_module_state(module_name).state == state) {
        return false;
    }

    get_runtime_module_state(module_name).state = state;
    return true;
}


bool ModulePersistor::add_profile(const std::string & module_name, const std::string & profile) {
    auto & profiles = get_runtime_module_state(module_name).installed_profiles;

    // Return false if the profile is already there.
    if (std::find(std::begin(profiles), std::end(profiles), profile) != std::end(profiles)) {
        return false;
    }

    profiles.push_back(profile);
    return true;
}


bool ModulePersistor::remove_profile(const std::string & module_name, const std::string & profile) {
    auto & profiles = get_runtime_module_state(module_name).installed_profiles;

    for (auto it = profiles.begin(); it != profiles.end(); it++) {
        if (*it == profile) {
            profiles.erase(it);
            return true;
        }
    }

    return false;
}


}  // namespace libdnf::module
