/*
Copyright (C) 2020 Red Hat, Inc.

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

#include "repoquery.hpp"

#include "../../context.hpp"
#include "../../wrappers/dbus_package_wrapper.hpp"

#include "libdnf-cli/output/repoquery.hpp"

#include "libdnf-cli/output/repoquery.hpp"

#include <dnfdaemon-server/dbus.hpp>
#include <fmt/format.h>
#include <libdnf-cli/argument_parser_config/repoquery.hpp>
#include <libdnf/conf/option_string.hpp>
#include <libdnf/rpm/package.hpp>
#include <libdnf/rpm/package_query.hpp>
#include <libdnf/rpm/package_set.hpp>

#include <iostream>

namespace dnfdaemon::client {

using namespace libdnf::cli;

void CmdRepoquery::set_argument_parser(Context & ctx) {
    auto repoquery = ctx.argparser_config->register_subcommand(libdnf::cli::arguments::repoquery);
    ctx.argparser_config->register_named_arg(libdnf::cli::arguments::available, repoquery);
    ctx.argparser_config->register_named_arg(libdnf::cli::arguments::installed, repoquery);
    ctx.argparser_config->register_named_arg(libdnf::cli::arguments::info, repoquery);
    ctx.argparser_config->register_positional_arg(libdnf::cli::arguments::repoquery_patterns, repoquery);
}

dnfdaemon::KeyValueMap CmdRepoquery::session_config([[maybe_unused]] Context & ctx) {
    auto selected_command = ctx.argparser_config->get_selected_command();
    dnfdaemon::KeyValueMap cfg = {};
    auto installed_option =
        static_cast<libdnf::OptionBool *>(ctx.argparser_config->get_arg_value("installed", selected_command));
    auto available_option =
        static_cast<libdnf::OptionBool *>(ctx.argparser_config->get_arg_value("available", selected_command));
    cfg["load_system_repo"] = installed_option->get_value();
    cfg["load_available_repos"] =
        (available_option->get_priority() >= libdnf::Option::Priority::COMMANDLINE || !installed_option->get_value());
    return cfg;
}

void CmdRepoquery::run(Context & ctx) {
    // query packages
    auto selected_command = ctx.argparser_config->get_selected_command();
    dnfdaemon::KeyValueMap options = {};

    std::vector<std::string> patterns;
    auto patterns_options = selected_command->get_positional_arg("repoquery_patterns").get_linked_values();
    if (!patterns_options->empty()) {
        patterns.reserve(patterns_options->size());
        for (auto & pattern : *patterns_options) {
            auto option = dynamic_cast<libdnf::OptionString *>(pattern.get());
            patterns.emplace_back(option->get_value());
        }
    }
    options["patterns"] = patterns;
    auto info_option =
        static_cast<libdnf::OptionBool *>(ctx.argparser_config->get_arg_value("info", selected_command));
    if (info_option->get_value()) {
        options.insert(std::pair<std::string, std::vector<std::string>>(
            "package_attrs", {"name", "epoch", "version", "release", "arch", "repo"}));
    } else {
        options.insert(std::pair<std::string, std::vector<std::string>>("package_attrs", {"full_nevra"}));
    }

    dnfdaemon::KeyValueMapList packages;
    ctx.session_proxy->callMethod("list")
        .onInterface(dnfdaemon::INTERFACE_RPM)
        .withTimeout(static_cast<uint64_t>(-1))
        .withArguments(options)
        .storeResultsTo(packages);

    auto num_packages = packages.size();
    for (auto & raw_package : packages) {
        --num_packages;
        DbusPackageWrapper package(raw_package);
        if (info_option->get_value()) {
            // TODO(mblaha) use smartcols for this output
            libdnf::cli::output::print_package_info_table(package);
            if (num_packages) {
                std::cout << std::endl;
            }
        } else {
            std::cout << package.get_full_nevra() << std::endl;
        }
    }
}

}  // namespace dnfdaemon::client
