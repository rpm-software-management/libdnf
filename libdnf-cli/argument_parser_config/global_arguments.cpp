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
#include <libdnf-cli/argument_parser_config/global_arguments.hpp>
#include <string.h>

namespace libdnf::cli::arguments {

ArgumentParser::NamedArg * assumeyes(ArgumentParserConfig & parser_config) {
    auto & arg_parser = parser_config.get_arg_parser();
    auto assumeyes = arg_parser.add_new_named_arg("assumeyes");
    assumeyes->set_long_name("assumeyes");
    assumeyes->set_short_name('y');
    assumeyes->set_short_description("automatically answer yes for all questions");
    assumeyes->set_const_value("true");
    assumeyes->link_value(&parser_config.get_config().assumeyes());
    return assumeyes;
}

ArgumentParser::NamedArg * comment(ArgumentParserConfig & parser_config) {
    auto & arg_parser = parser_config.get_arg_parser();
    auto value =
        static_cast<libdnf::OptionString *>(arg_parser.add_init_value(std::make_unique<libdnf::OptionString>(nullptr)));
    auto comment = arg_parser.add_new_named_arg("comment");
    comment->set_long_name("comment");
    comment->set_has_value(true);
    comment->set_arg_value_help("COMMENT");
    comment->set_short_description("add a comment to transaction");
    comment->set_description(
        "Adds a comment to the action. If a transaction takes place, the comment is stored in it.");
    comment->link_value(value);
    return comment;
}


ArgumentParser::NamedArg * help(ArgumentParserConfig & parser_config) {
    auto & arg_parser = parser_config.get_arg_parser();
    auto value =
        static_cast<libdnf::OptionBool *>(arg_parser.add_init_value(std::make_unique<libdnf::OptionBool>(false)));
    auto help = arg_parser.add_new_named_arg("help");
    help->set_long_name("help");
    help->set_short_name('h');
    help->set_short_description("Print help");
    help->link_value(value);
    return help;
}


ArgumentParser::NamedArg * verbose(ArgumentParserConfig & parser_config) {
    auto & arg_parser = parser_config.get_arg_parser();
    auto value =
        static_cast<libdnf::OptionBool *>(arg_parser.add_init_value(std::make_unique<libdnf::OptionBool>(false)));
    auto verbose = arg_parser.add_new_named_arg("verbose");
    verbose->set_short_name('v');
    verbose->set_long_name("verbose");
    verbose->set_short_description("increase output verbosity");
    verbose->set_const_value("true");
    verbose->link_value(value);
    return verbose;
}


ArgumentParser::NamedArg * assumeno(ArgumentParserConfig & parser_config) {
    auto & arg_parser = parser_config.get_arg_parser();
    auto assumeno = arg_parser.add_new_named_arg("assumeno");
    assumeno->set_long_name("assumeno");
    assumeno->set_short_description("automatically answer no for all questions");
    assumeno->set_const_value("true");
    assumeno->link_value(&parser_config.get_config().assumeno());
    return assumeno;
}


ArgumentParser::NamedArg * allowerasing(ArgumentParserConfig & parser_config) {
    auto & arg_parser = parser_config.get_arg_parser();
    auto value =
        static_cast<libdnf::OptionBool *>(arg_parser.add_init_value(std::make_unique<libdnf::OptionBool>(false)));
    auto allowerasing = arg_parser.add_new_named_arg("allowerasing");
    allowerasing->set_long_name("allowerasing");
    allowerasing->set_short_description("installed package can be removed to resolve the transaction");
    allowerasing->set_const_value("true");
    allowerasing->link_value(value);
    return allowerasing;
}


ArgumentParser::NamedArg * setopt(ArgumentParserConfig & parser_config) {
    auto & arg_parser = parser_config.get_arg_parser();
    auto setopt = arg_parser.add_new_named_arg("setopt");
    setopt->set_long_name("setopt");
    setopt->set_has_value(true);
    setopt->set_arg_value_help("KEY=VALUE");
    setopt->set_short_description("set arbitrary config and repo options");
    setopt->set_description(
        R"**(Override a configuration option from the configuration file. To override configuration options for repositories, use repoid.option for  the
              <option>.  Values  for configuration options like excludepkgs, includepkgs, installonlypkgs and tsflags are appended to the original value,
              they do not override it. However, specifying an empty value (e.g. --setopt=tsflags=) will clear the option.)**");

    // --setopt option support
    setopt->set_parse_hook_func([&parser_config](
                                    [[maybe_unused]] libdnf::cli::ArgumentParser::NamedArg * arg,
                                    [[maybe_unused]] const char * option,
                                    const char * value) {
        auto val = strchr(value + 1, '=');
        if (!val) {
            throw std::runtime_error(std::string("setopt: Invalid argument value format: Missing '=': ") + value);
        }
        auto key = std::string(value, val);
        auto dot_pos = key.rfind('.');
        if (dot_pos != std::string::npos) {
            if (dot_pos == key.size() - 1) {
                throw std::runtime_error(
                    std::string("setopt: Invalid argument value format: Last key character cannot be '.': ") + value);
            }
        }
        // Store option to vector for later use
        parser_config.setopts.emplace_back(key, val + 1);
        return true;
    });
    return setopt;
}

}  // namespace libdnf::cli::arguments
