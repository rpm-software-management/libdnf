/*
Copyright (C) 2021 Red Hat, Inc.

This file is part of libdnf: https://github.com/rpm-software-management/libdnf/

Libdnf is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

Libdnf is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with libdnf.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <libdnf-cli/argument_parser.hpp>
#include <libdnf-cli/argument_parser_config/argument_parser_config.hpp>

namespace libdnf::cli::arguments {

ArgumentParser::Command * repolist(ArgumentParserConfig & parser_config) {
    auto & arg_parser = parser_config.get_arg_parser();
    auto repolist = arg_parser.add_new_command("repolist");
    repolist->set_short_description("display the configured software repositories");
    repolist->set_description("");
    repolist->set_named_args_help_header("Optional arguments:");
    repolist->set_positional_args_help_header("Positional arguments:");
    repolist->set_parse_hook_func([repolist, &parser_config](
                                      [[maybe_unused]] libdnf::cli::ArgumentParser::Argument * arg,
                                      [[maybe_unused]] const char * option,
                                      [[maybe_unused]] int argc,
                                      [[maybe_unused]] const char * const argv[]) {
        parser_config.set_selected_command(repolist);
        return true;
    });
    return repolist;
}


ArgumentParser::Command * repoinfo(ArgumentParserConfig & parser_config) {
    auto & arg_parser = parser_config.get_arg_parser();
    auto repoinfo = arg_parser.add_new_command("repoinfo");
    repoinfo->set_short_description("display the configured software repositories");
    repoinfo->set_description("");
    repoinfo->set_named_args_help_header("Optional arguments:");
    repoinfo->set_positional_args_help_header("Positional arguments:");
    repoinfo->set_parse_hook_func([repoinfo, &parser_config](
                                      [[maybe_unused]] libdnf::cli::ArgumentParser::Argument * arg,
                                      [[maybe_unused]] const char * option,
                                      [[maybe_unused]] int argc,
                                      [[maybe_unused]] const char * const argv[]) {
        parser_config.set_selected_command(repoinfo);
        return true;
    });
    return repoinfo;
}


ArgumentParser::NamedArg * all(ArgumentParserConfig & parser_config) {
    auto & arg_parser = parser_config.get_arg_parser();
    auto value =
        static_cast<libdnf::OptionBool *>(arg_parser.add_init_value(std::make_unique<libdnf::OptionBool>(false)));
    auto all = arg_parser.add_new_named_arg("all");
    all->set_long_name("all");
    all->set_short_description("show all repos");
    all->set_const_value("true");
    all->link_value(value);
    return all;
}


ArgumentParser::NamedArg * enabled(ArgumentParserConfig & parser_config) {
    auto & arg_parser = parser_config.get_arg_parser();
    auto value =
        static_cast<libdnf::OptionBool *>(arg_parser.add_init_value(std::make_unique<libdnf::OptionBool>(false)));
    auto enabled = arg_parser.add_new_named_arg("enabled");
    enabled->set_long_name("enabled");
    enabled->set_short_description("show enabled repos (default)");
    enabled->set_const_value("true");
    enabled->link_value(value);
    return enabled;
}


ArgumentParser::NamedArg * disabled(ArgumentParserConfig & parser_config) {
    auto & arg_parser = parser_config.get_arg_parser();
    auto value =
        static_cast<libdnf::OptionBool *>(arg_parser.add_init_value(std::make_unique<libdnf::OptionBool>(false)));
    auto disabled = arg_parser.add_new_named_arg("disabled");
    disabled->set_long_name("disabled");
    disabled->set_short_description("show disabled repos");
    disabled->set_const_value("true");
    disabled->link_value(value);
    return disabled;
}


ArgumentParser::PositionalArg * repolist_patterns(ArgumentParserConfig & parser_config) {
    auto & arg_parser = parser_config.get_arg_parser();
    auto patterns_options = arg_parser.add_new_values();
    auto keys = arg_parser.add_new_positional_arg(
        "repolist_patterns",
        libdnf::cli::ArgumentParser::PositionalArg::UNLIMITED,
        arg_parser.add_init_value(std::unique_ptr<libdnf::Option>(new libdnf::OptionString(nullptr))),
        patterns_options);
    keys->set_short_description("List of repositories to query");
    return keys;
}

}  // namespace libdnf::cli::arguments
