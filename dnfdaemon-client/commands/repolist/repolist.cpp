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

#include "repolist.hpp"

#include "../../context.hpp"
#include "../../wrappers/dbus_repo_wrapper.hpp"
#include "../../wrappers/dbus_query_repo_wrapper.hpp"
#include "libdnf-cli/output/repolist.hpp"

#include <dnfdaemon-server/dbus.hpp>
#include <libdnf-cli/argument_parser_config/repolist.hpp>
#include <libdnf-cli/output/repo_info.hpp>
#include <libdnf/conf/option_string.hpp>

#include <iostream>
#include <numeric>

namespace dnfdaemon::client {

void CmdRepolist::set_argument_parser(Context & ctx) {
    libdnf::cli::ArgumentParser::Command * ap_command;
    if (command == "repolist") {
        ap_command = ctx.argparser_config->register_subcommand(libdnf::cli::arguments::repolist);
    } else {
        ap_command = ctx.argparser_config->register_subcommand(libdnf::cli::arguments::repoinfo);
    }

    auto all = ctx.argparser_config->register_named_arg(libdnf::cli::arguments::all, ap_command);
    auto enabled = ctx.argparser_config->register_named_arg(libdnf::cli::arguments::enabled, ap_command);
    auto disabled = ctx.argparser_config->register_named_arg(libdnf::cli::arguments::disabled, ap_command);

    ctx.argparser_config->register_positional_arg(libdnf::cli::arguments::repolist_patterns, ap_command);

    auto & arg_parser = ctx.argparser_config->get_arg_parser();
    auto conflict_args =
        arg_parser.add_conflict_args_group(std::unique_ptr<std::vector<libdnf::cli::ArgumentParser::Argument *>>(
            new std::vector<libdnf::cli::ArgumentParser::Argument *>{all, enabled, disabled}));

    all->set_conflict_arguments(conflict_args);
    enabled->set_conflict_arguments(conflict_args);
    disabled->set_conflict_arguments(conflict_args);
}

/// Joins vector of strings to a single string using given separator
/// ["a", "b", "c"] -> "a b c"
std::string join(const std::vector<std::string> & str_list, const std::string & separator) {
    return std::accumulate(
        str_list.begin(),
        str_list.end(),
        std::string(),
        [&](const std::string & a, const std::string & b) -> std::string {
            return a + (a.length() > 0 ? separator : "") + b;
        });
}

/// Joins vector of string pairs to a single string. Pairs are joined using
/// field_separator, records using record_separator
/// [("a", "1"), ("b", "2")] -> "a: 1, b: 2"
std::string join_pairs(
    const std::vector<std::pair<std::string, std::string>> & pair_list,
    const std::string & field_separator,
    const std::string & record_separator) {
    std::vector<std::string> records{};
    for (auto & pair : pair_list) {
        records.emplace_back(pair.first + field_separator + pair.second);
    }
    return join(records, record_separator);
}

std::string enable_disable(Context & ctx) {
    std::string retval{"enabled"};
    auto sc = ctx.argparser_config->get_selected_command();
    auto all = static_cast<libdnf::OptionBool *>(ctx.argparser_config->get_arg_value("all", sc));
    auto disabled = static_cast<libdnf::OptionBool *>(ctx.argparser_config->get_arg_value("disabled", sc));
    if (all->get_value()) {
        retval = "all";
    } else if (disabled->get_value()) {
        retval = "disabled";
    }
    return retval;
}

void CmdRepolist::run(Context & ctx) {
    // prepare options from command line arguments
    dnfdaemon::KeyValueMap options = {};
    auto enable_disable_option = enable_disable(ctx);
    options["enable_disable"] = enable_disable_option;

    auto selected_command = ctx.argparser_config->get_selected_command();
    std::vector<std::string> patterns;
    auto patterns_options = selected_command->get_positional_arg("repolist_patterns").get_linked_values();
    if (!patterns_options->empty()) {
        options["enable_disable"] = "all";
        patterns.reserve(patterns_options->size());
        for (auto & pattern : *patterns_options) {
            auto option = static_cast<libdnf::OptionString *>(pattern.get());
            patterns.emplace_back(option->get_value());
        }
    }
    options["patterns"] = patterns;

    std::vector<std::string> attrs{"id", "name", "enabled"};
    if (command == "repoinfo") {
        std::vector<std::string> repoinfo_attrs{
            "size",
            "revision",
            "content_tags",
            "distro_tags",
            "updated",
            "pkgs",
            "available_pkgs",
            "metalink",
            "mirrorlist",
            "baseurl",
            "metadata_expire",
            "excludepkgs",
            "includepkgs",
            "repofile"};
        attrs.insert(attrs.end(), repoinfo_attrs.begin(), repoinfo_attrs.end());
    }
    options["repo_attrs"] = attrs;

    // call list() method on repo interface via dbus
    dnfdaemon::KeyValueMapList repositories;
    ctx.session_proxy->callMethod("list")
        .onInterface(dnfdaemon::INTERFACE_REPO)
        .withArguments(options)
        .storeResultsTo(repositories);

    if (command == "repolist") {
        // print the output table
        bool with_status = enable_disable_option == "all";
        libdnf::cli::output::print_repolist_table(
            DbusQueryRepoWrapper(repositories), with_status,
            libdnf::cli::output::COL_REPO_ID);
    } else {
        // repoinfo command
        auto verbose = static_cast<libdnf::OptionBool *>(ctx.argparser_config->get_arg_value("verbose"));

        for (auto & raw_repo : repositories) {
            DbusRepoWrapper repo(raw_repo);
            auto repo_info = libdnf::cli::output::RepoInfo();
            repo_info.add_repo(repo, verbose->get_value(), verbose->get_value());
            repo_info.print();
            std::cout << std::endl;
        }
    }
}

}  // namespace dnfdaemon::client
