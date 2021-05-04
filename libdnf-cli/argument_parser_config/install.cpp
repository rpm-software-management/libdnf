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

ArgumentParser::Command * install(ArgumentParserConfig & parser_config) {
    auto & arg_parser = parser_config.get_arg_parser();
    auto install = arg_parser.add_new_command("install");
    install->set_short_description("install packages on the system");
    install->set_description("");
    install->set_named_args_help_header("Optional arguments:");
    install->set_positional_args_help_header("Positional arguments:");
    install->set_parse_hook_func([install, &parser_config](
                                     [[maybe_unused]] libdnf::cli::ArgumentParser::Argument * arg,
                                     [[maybe_unused]] const char * option,
                                     [[maybe_unused]] int argc,
                                     [[maybe_unused]] const char * const argv[]) {
        parser_config.set_selected_command(install);
        return true;
    });
    return install;
}


ArgumentParser::NamedArg * strict(ArgumentParserConfig & parser_config) {
    auto & arg_parser = parser_config.get_arg_parser();
    auto value =
        static_cast<libdnf::OptionBool *>(arg_parser.add_init_value(std::make_unique<libdnf::OptionBool>(false)));
    auto strict = arg_parser.add_new_named_arg("strict");
    strict->set_long_name("strict");
    strict->set_short_description(
        "Broken or unavailable packages cause the transaction to fail (yes) or will be skipped (no).");
    strict->set_has_value(true);
    strict->set_arg_value_help("<yes/no>");
    strict->link_value(value);
    return strict;
}


ArgumentParser::PositionalArg * install_patterns(ArgumentParserConfig & parser_config) {
    auto & arg_parser = parser_config.get_arg_parser();
    auto patterns_options = arg_parser.add_new_values();
    auto keys = arg_parser.add_new_positional_arg(
        "install_patterns",
        libdnf::cli::ArgumentParser::PositionalArg::UNLIMITED_BUT_ONE,
        arg_parser.add_init_value(std::unique_ptr<libdnf::Option>(new libdnf::OptionString(nullptr))),
        patterns_options);
    keys->set_short_description("List of packages to install");
    return keys;
}

}  // namespace libdnf::cli::arguments
