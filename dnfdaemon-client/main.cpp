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

#include "commands/downgrade/downgrade.hpp"
#include "commands/install/install.hpp"
#include "commands/remove/remove.hpp"
#include "commands/repolist/repolist.hpp"
#include "commands/repoquery/repoquery.hpp"
#include "commands/upgrade/upgrade.hpp"
#include "context.hpp"

#include <dnfdaemon-server/dbus.hpp>
#include <fcntl.h>
#include <fmt/format.h>
#include <libdnf-cli/argument_parser.hpp>
#include <libdnf-cli/argument_parser_config/argument_parser_config.hpp>
#include <libdnf-cli/argument_parser_config/global_arguments.hpp>
#include <libdnf/base/base.hpp>
#include <libdnf/logger/memory_buffer_logger.hpp>
#include <libdnf/logger/stream_logger.hpp>
#include <string.h>

#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>

namespace fs = std::filesystem;

namespace dnfdaemon::client {

static bool parse_args(Context & ctx, int argc, char * argv[]) {
    // configure the root command
    auto rc = ctx.argparser_config->get_root_command();
    rc->set_short_description("Utility for packages maintaining");
    rc->set_description("Dnfdaemon-client is a program for maintaining packages.");
    rc->set_commands_help_header("List of commands:");
    rc->set_named_args_help_header("Global arguments:");

    // register global options
    ctx.argparser_config->register_named_arg(libdnf::cli::arguments::help);
    ctx.argparser_config->register_named_arg(libdnf::cli::arguments::verbose);
    ctx.argparser_config->register_named_arg(libdnf::cli::arguments::assumeyes);
    ctx.argparser_config->register_named_arg(libdnf::cli::arguments::assumeno);
    ctx.argparser_config->register_named_arg(libdnf::cli::arguments::allowerasing);
    ctx.argparser_config->register_named_arg(libdnf::cli::arguments::setopt);

    for (auto const & command : ctx.commands) {
        command.second->set_argument_parser(ctx);
    }

    try {
        ctx.argparser_config->parse(argc, argv);
    } catch (const std::exception & ex) {
        std::cout << ex.what() << std::endl;
    }

    auto & help = ctx.argparser_config->get_arg("help");
    return help.get_parse_count() > 0;
}

}  // namespace dnfdaemon::client


int main(int argc, char * argv[]) {
    auto connection = sdbus::createSystemBusConnection();
    connection->enterEventLoopAsync();

    dnfdaemon::client::Context context(*connection);

    // TODO(mblaha): logging

    //log_router.info("Dnfdaemon-client start");

    // Register commands
    context.commands.emplace("repoinfo", std::make_unique<dnfdaemon::client::CmdRepolist>("repoinfo"));
    context.commands.emplace("repolist", std::make_unique<dnfdaemon::client::CmdRepolist>("repolist"));
    context.commands.emplace("repoquery", std::make_unique<dnfdaemon::client::CmdRepoquery>());
    context.commands.emplace("downgrade", std::make_unique<dnfdaemon::client::CmdDowngrade>());
    context.commands.emplace("install", std::make_unique<dnfdaemon::client::CmdInstall>());
    context.commands.emplace("upgrade", std::make_unique<dnfdaemon::client::CmdUpgrade>());
    context.commands.emplace("remove", std::make_unique<dnfdaemon::client::CmdRemove>());

    // Parse command line arguments
    bool help_printed = dnfdaemon::client::parse_args(context, argc, argv);
    auto argparser_selected_command = context.argparser_config->get_selected_command();
    if (!argparser_selected_command) {
        if (help_printed) {
            return 0;
        } else {
            context.argparser_config->get_root_command()->help();
            return 1;
        }
    }
    context.selected_command = context.commands.at(argparser_selected_command->get_id()).get();

    // initialize server session using command line arguments
    context.init_session();

    // Preconfigure selected command
    context.selected_command->pre_configure(context);

    // Configure selected command
    context.selected_command->configure(context);

    // Run selected command
    try {
        context.selected_command->run(context);
    } catch (std::exception & ex) {
        std::cerr << fmt::format("Command returned error: {}", ex.what()) << std::endl;
    }

    //log_router.info("Dnfdaemon-client end");

    return 0;
}
