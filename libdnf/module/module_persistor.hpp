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

#ifndef LIBDNF_MODULE_MODULE_PERSISTOR_HPP
#define LIBDNF_MODULE_MODULE_PERSISTOR_HPP

#include "libdnf/module/module_item_container.hpp"
#include "libdnf/system/state.hpp"

#include <string>
#include <utility>
#include <vector>

namespace libdnf::module {


class ModulePersistor {
public:
    ModulePersistor(libdnf::system::State & system_state);

    bool add_module();
    void save_everything_to_system_state();

    const ModuleState & get_state(const std::string & module_name);
    const std::string & get_enabled_stream(const std::string & module_name);
    const std::vector<std::string> & get_installed_profiles(const std::string & module_name);

    std::vector<std::string> get_all_module_names();

    std::vector<std::string> get_all_newly_disabled_modules();
    std::vector<std::string> get_all_newly_reset_modules();
    std::map<std::string, std::string> get_all_newly_enabled_streams();
    std::map<std::string, std::string> get_all_newly_disabled_streams();
    std::map<std::string, std::string> get_all_newly_reset_streams();
    std::map<std::string, std::pair<std::string, std::string>> get_all_newly_switched_streams();
    std::map<std::string, std::vector<std::string>> get_all_newly_installed_profiles();
    std::map<std::string, std::vector<std::string>> get_all_newly_removed_profiles();

    bool change_state(const std::string & module_name, ModuleState state);
    bool change_stream(const std::string & module_name, const std::string & stream);
    bool add_profile(const std::string & module_name, const std::string & profile);
    bool remove_profile(const std::string & module_name, const std::string & profile);

private:
    friend ModuleItemContainer;

    struct RuntimeModuleState {
        ModuleState state;
        std::string enabled_stream;
        std::vector<std::string> installed_profiles;
        // TODO(pkratoch): What is this for?
        bool locked;
        int stream_changes_num;
    };

    libdnf::system::State & system_state;

    // Map of module names and their runtime states.
    std::map<std::string, struct RuntimeModuleState> runtime_module_states;

    struct RuntimeModuleState & get_runtime_module_state(const std::string & module_name);

    // Set ModuleState in system::State according to the RuntimeModuleState. Return `true` if it changed.
    bool set_config_from_runtime(const std::string & module_name);
    // Set RuntimeModuleState according to the ModuleState in system::State.
    void set_runtime_from_config(const std::string & module_name);
};


}  // namespace libdnf::module


#endif  // LIBDNF_MODULE_MODULE_PERSISTOR_HPP
