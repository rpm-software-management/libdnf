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

#ifndef LIBDNF_CLI_ARGUMENT_PARSER_CONFIG_ARGUMENT_PARSER_CONFIG_HPP
#define LIBDNF_CLI_ARGUMENT_PARSER_CONFIG_ARGUMENT_PARSER_CONFIG_HPP

#include <libdnf-cli/argument_parser.hpp>
#include <libdnf/conf/config_main.hpp>

#include <map>
#include <string>

namespace libdnf::cli {

class ArgumentParserConfig {
public:
    explicit ArgumentParserConfig(libdnf::ConfigMain & config);

    ArgumentParser & get_arg_parser() noexcept { return arg_parser; }
    libdnf::ConfigMain & get_config() const noexcept { return config; };
    ArgumentParser::Command * get_root_command() { return arg_parser.get_root_command(); };

    ArgumentParser::Command * get_selected_command() { return selected_command; };
    void set_selected_command(ArgumentParser::Command * command) { selected_command = command; };

    ArgumentParser::Command * register_subcommand(
        ArgumentParser::Command * (*command_factory)(ArgumentParserConfig & arg), ArgumentParser::Command * parent);
    ArgumentParser::Command * register_subcommand(
        ArgumentParser::Command * (*command_factory)(ArgumentParserConfig & arg)) {
        return register_subcommand(command_factory, arg_parser.get_root_command());
    }

    ArgumentParser::PositionalArg * register_positional_arg(
        ArgumentParser::PositionalArg * (*arg_factory)(ArgumentParserConfig & arg), ArgumentParser::Command * command);
    ArgumentParser::PositionalArg * register_positional_arg(
        ArgumentParser::PositionalArg * (*arg_factory)(ArgumentParserConfig & arg)) {
        return register_positional_arg(arg_factory, arg_parser.get_root_command());
    }

    ArgumentParser::NamedArg * register_named_arg(
        ArgumentParser::NamedArg * (*arg_factory)(ArgumentParserConfig & arg), ArgumentParser::Command * command);
    ArgumentParser::NamedArg * register_named_arg(
        ArgumentParser::NamedArg * (*arg_factory)(ArgumentParserConfig & arg)) {
        return register_named_arg(arg_factory, arg_parser.get_root_command());
    };

    ArgumentParser::NamedArg & get_arg(const std::string & id, ArgumentParser::Command * command);
    ArgumentParser::NamedArg & get_arg(const std::string & id) {
        return get_arg(id, arg_parser.get_root_command());
    };

    libdnf::Option * get_arg_value(const std::string & id, ArgumentParser::Command * command);
    libdnf::Option * get_arg_value(const std::string & id) {
        return get_arg_value(id, arg_parser.get_root_command());
    };

    void parse(int argc, const char * const argv[]) { arg_parser.parse(argc, argv); };

    // TODO(mblaha) handle setopts properly
    std::vector<std::pair<std::string, std::string>> setopts;

private:
    libdnf::ConfigMain & config;
    ArgumentParser arg_parser;
    ArgumentParser::Command * selected_command{nullptr};
};

}  // namespace libdnf::cli

#endif
