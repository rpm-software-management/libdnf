/*
Copyright Contributors to the libdnf project.

This file is part of libdnf: https://github.com/rpm-software-management/libdnf/

Libdnf is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

Libdnf is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with libdnf.  If not, see <https://www.gnu.org/licenses/>.
*/


#include "search.hpp"

#include <libdnf/conf/option_string.hpp>
#include <libdnf/rpm/package.hpp>
#include <libdnf/rpm/package_query.hpp>
#include <libdnf/rpm/package_set.hpp>
#include <libdnf/transaction/transaction_item_reason.hpp>

#include <iostream>

#include "libdnf-cli/tty.hpp"

#include <libsmartcols/libsmartcols.h>

namespace fs = std::filesystem;


namespace dnf5 {


using namespace libdnf::cli;


class SearchResult {
public:
    std::string type;
    std::string name;
    std::string summary;
    bool installed = false;
    bool userinstalled = false;

    std::string get_flags() const {
        std::string result;
        if (installed) {
            result += "i";
            if (userinstalled) {
                result += "+";
            }
        }
        return result;
    }

};


void print_search_results(const std::map<std::string, SearchResult> & search_results) {
    std::unique_ptr<libscols_table, decltype(&scols_unref_table)> table(scols_new_table(), &scols_unref_table);

    auto cl_flags = scols_table_new_column(table.get(), "F", 0, 0);
    auto cl_name = scols_table_new_column(table.get(), "NAME", 0.3, 0);
    auto cl_summary = scols_table_new_column(table.get(), "SUMMARY", 0.6, 0);
    auto cl_type = scols_table_new_column(table.get(), "TYPE", 0.1, 0);

    for (auto cl : {cl_flags, cl_name, cl_summary, cl_type}) {
        auto header = scols_column_get_header(cl);
        scols_cell_set_color(header, "bold");
    }

    if (libdnf::cli::tty::is_interactive()) {
        scols_table_enable_colors(table.get(), 1);
    }

    for (auto it : search_results) {
        auto & sr = it.second;
        struct libscols_line * ln = scols_table_new_line(table.get(), NULL);
        scols_line_set_data(ln, 0, sr.get_flags().c_str());
        scols_line_set_data(ln, 1, sr.name.c_str());
        scols_line_set_data(ln, 2, sr.summary.c_str());
        scols_line_set_data(ln, 3, sr.type.c_str());
    }

    scols_print_table(table.get());
}


void SearchCommand::set_argument_parser() {
    auto & ctx = get_context();
    auto & parser = ctx.get_argument_parser();

    auto & cmd = *get_argument_parser_command();
    cmd.set_short_description("Search for software matching all specified strings");

    patterns = parser.add_new_values();
    auto patterns_arg = parser.add_new_positional_arg(
        "patterns",
        ArgumentParser::PositionalArg::UNLIMITED,
        parser.add_init_value(std::unique_ptr<libdnf::Option>(new libdnf::OptionString(nullptr))),
        patterns);
    patterns_arg->set_short_description("Patterns");
    cmd.register_positional_arg(patterns_arg);
}


void SearchCommand::configure() {
    auto & context = get_context();
    context.set_load_system_repo(true);
    context.set_load_available_repos(Context::LoadAvailableRepos::ENABLED);
}


void SearchCommand::run() {
    auto & ctx = get_context();

    libdnf::rpm::PackageSet pset(ctx.base);

    for (auto & pattern : *patterns) {
        libdnf::rpm::PackageQuery q(ctx.base);
        auto pat = pattern.get()->get_value_string();
        if (pat.find("*") == std::string::npos && pat.find("?") == std::string::npos) {
            // no glob characters found in the string, wrap it in globs
            pat = "*" + pat + "*";
        }
        q.filter_name({pat}, libdnf::sack::QueryCmp::IGLOB);
        pset |= q;
    }

    std::map<std::string, SearchResult> result;

    for (auto package : pset) {
        auto it = result.find(package.get_name());
        if (it == result.end()) {
            SearchResult sr;
            sr.type = "package";
            sr.name = package.get_name();
            sr.summary = package.get_summary();
            sr.installed = package.is_installed();
            sr.userinstalled = package.get_reason() == libdnf::transaction::TransactionItemReason::USER;
            result.emplace(package.get_name(), sr);
        } else {
            auto & sr = it->second;
            if (package.is_installed()) {
                sr.installed = true;
                sr.userinstalled |= package.get_reason() == libdnf::transaction::TransactionItemReason::USER;
            }
        }
    }

    print_search_results(result);
    std::cout << '\n';
}


/*
from `man zypper`:

           The Status column can contain the following values:

               i+
                   installed by user request

               i
                   installed automatically (by the resolver, see section Automatically installed packages)

               v
                   a different version is installed

               empty
                   neither of the above cases

               !
                   a patch in needed state

               .l
                   is shown in the 2nd column if the item is locked (see section Package Locks Management)

               .P
                   is shown in the 2nd column if the item is part of a PTF (A program temporary fix which must be explicitly selected and will otherwise not be considered in dependency
                   resolution).

               .R
                   is shown in the 2nd column if the item has been retracted (see patch in section Package Types)

           The v status is only shown if the version or the repository matters (see --details or --repo), and the installed instance differs from the one listed in version or
           repository.
*/


}  // namespace dnf5
