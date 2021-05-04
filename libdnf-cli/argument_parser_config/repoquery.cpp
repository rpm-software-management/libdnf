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

ArgumentParser::Command * repoquery(ArgumentParserConfig & parser_config) {
    auto & arg_parser = parser_config.get_arg_parser();
    auto repoquery = arg_parser.add_new_command("repoquery");
    repoquery->set_short_description("list packages matching keywords");
    repoquery->set_description("");
    repoquery->set_named_args_help_header("Optional arguments:");
    repoquery->set_positional_args_help_header("Positional arguments:");
    repoquery->set_parse_hook_func([repoquery, &parser_config](
                                       [[maybe_unused]] libdnf::cli::ArgumentParser::Argument * arg,
                                       [[maybe_unused]] const char * option,
                                       [[maybe_unused]] int argc,
                                       [[maybe_unused]] const char * const argv[]) {
        parser_config.set_selected_command(repoquery);
        return true;
    });
    return repoquery;
}


ArgumentParser::NamedArg * available(ArgumentParserConfig & parser_config) {
    auto & arg_parser = parser_config.get_arg_parser();
    auto value =
        static_cast<libdnf::OptionBool *>(arg_parser.add_init_value(std::make_unique<libdnf::OptionBool>(false)));
    auto available = arg_parser.add_new_named_arg("available");
    available->set_long_name("available");
    available->set_short_description("search in available packages (default)");
    available->set_const_value("true");
    available->link_value(value);
    return available;
}


ArgumentParser::NamedArg * installed(ArgumentParserConfig & parser_config) {
    auto & arg_parser = parser_config.get_arg_parser();
    auto value =
        static_cast<libdnf::OptionBool *>(arg_parser.add_init_value(std::make_unique<libdnf::OptionBool>(false)));
    auto installed = arg_parser.add_new_named_arg("installed");
    installed->set_long_name("installed");
    installed->set_short_description("search in installed packages");
    installed->set_const_value("true");
    installed->link_value(value);
    return installed;
}


ArgumentParser::NamedArg * info(ArgumentParserConfig & parser_config) {
    auto & arg_parser = parser_config.get_arg_parser();
    auto value =
        static_cast<libdnf::OptionBool *>(arg_parser.add_init_value(std::make_unique<libdnf::OptionBool>(false)));
    auto info = arg_parser.add_new_named_arg("info");
    info->set_long_name("info");
    info->set_short_description("show detailed information about the packages");
    info->set_const_value("true");
    info->link_value(value);
    return info;
}


ArgumentParser::PositionalArg * repoquery_patterns(ArgumentParserConfig & parser_config) {
    auto & arg_parser = parser_config.get_arg_parser();
    auto patterns_options = arg_parser.add_new_values();
    auto keys = arg_parser.add_new_positional_arg(
        "repoquery_patterns",
        libdnf::cli::ArgumentParser::PositionalArg::UNLIMITED,
        arg_parser.add_init_value(std::unique_ptr<libdnf::Option>(new libdnf::OptionString(nullptr))),
        patterns_options);
    keys->set_short_description("List of packages to search for");
    return keys;
}

}  // namespace libdnf::cli::arguments
