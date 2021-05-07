/*
Copyright (C) 2021 Red Hat, Inc.

This file is part of dnfdaemon-client: https://github.com/rpm-software-management/libdnf/

Dnfdaemon-client is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

Dnfdaemon-client is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with dnfdaemon-client.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "install.hpp"

#include "../../context.hpp"
#include "../../utils.hpp"

#include "libdnf-cli/output/transaction_table.hpp"

#include <dnfdaemon-server/dbus.hpp>
#include <libdnf-cli/argument_parser.hpp>
#include <libdnf-cli/argument_parser_config/install.hpp>
#include <libdnf/conf/option_bool.hpp>
#include <libdnf/conf/option_string.hpp>

#include <iostream>
#include <memory>

namespace dnfdaemon::client {

void CmdInstall::set_argument_parser(Context & ctx) {
    auto install = ctx.argparser_config->register_subcommand(libdnf::cli::arguments::install);
    ctx.argparser_config->register_named_arg(libdnf::cli::arguments::strict, install);
    ctx.argparser_config->register_positional_arg(libdnf::cli::arguments::install_patterns, install);
}


void CmdInstall::run(Context & ctx) {
    if (!am_i_root()) {
        std::cout << "This command has to be run with superuser privileges (under the root user on most systems)."
                  << std::endl;
        return;
    }

    // get package specs from command line and add them to the goal
    auto selected_command = ctx.argparser_config->get_selected_command();
    std::vector<std::string> patterns;
    auto patterns_options = selected_command->get_positional_arg("install_patterns").get_linked_values();
    if (patterns_options->size() > 0) {
        patterns.reserve(patterns_options->size());
        for (auto & pattern : *patterns_options) {
            auto option = dynamic_cast<libdnf::OptionString *>(pattern.get());
            patterns.emplace_back(option->get_value());
        }
    }

    dnfdaemon::KeyValueMap options = {};

    // pass the `strict` value to the server only when explicitly set by command line option
    auto strict_option =
        static_cast<libdnf::OptionBool *>(ctx.argparser_config->get_arg_value("strict", selected_command));
    if (strict_option->get_priority() >= libdnf::Option::Priority::COMMANDLINE) {
        options["strict"] = strict_option->get_value();
    }

    ctx.session_proxy->callMethod("install")
        .onInterface(dnfdaemon::INTERFACE_RPM)
        .withTimeout(static_cast<uint64_t>(-1))
        .withArguments(patterns, options);

    run_transaction(ctx);
}

}  // namespace dnfdaemon::client
