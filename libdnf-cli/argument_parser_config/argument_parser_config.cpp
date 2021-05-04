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
#include <libdnf/conf/config_main.hpp>

#include <map>
#include <string>

namespace libdnf::cli {

ArgumentParserConfig::ArgumentParserConfig(libdnf::ConfigMain & config) : config(config) {
    auto root_command = arg_parser.add_new_command("_root");
    arg_parser.set_root_command(root_command);
}


ArgumentParser::Command * ArgumentParserConfig::register_subcommand(
    ArgumentParser::Command * (*command_factory)(ArgumentParserConfig & arg), ArgumentParser::Command * parent) {
    auto cmd = command_factory(*this);
    parent->register_command(cmd);
    return cmd;
}


ArgumentParser::NamedArg * ArgumentParserConfig::register_named_arg(
    ArgumentParser::NamedArg * (*arg_factory)(ArgumentParserConfig & arg), ArgumentParser::Command * command) {
    auto arg = arg_factory(*this);
    command->register_named_arg(arg);
    return arg;
}


ArgumentParser::PositionalArg * ArgumentParserConfig::register_positional_arg(
    ArgumentParser::PositionalArg * (*arg_factory)(ArgumentParserConfig & arg), ArgumentParser::Command * command) {
    auto arg = arg_factory(*this);
    command->register_positional_arg(arg);
    return arg;
}


ArgumentParser::NamedArg & ArgumentParserConfig::get_arg(
    const std::string & id, ArgumentParser::Command * command) {
    return command->get_named_arg(id);
}


libdnf::Option * ArgumentParserConfig::get_arg_value(
    const std::string & id, ArgumentParser::Command * command) {
    return get_arg(id, command).get_linked_value();
}

}  // namespace libdnf::cli
