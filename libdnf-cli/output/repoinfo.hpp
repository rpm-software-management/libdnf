/*
Copyright (C) 2020 Red Hat, Inc.

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


#ifndef LIBDNF_CLI_OUTPUT_REPOINFO_HPP
#define LIBDNF_CLI_OUTPUT_REPOINFO_HPP

#include "libdnf-cli/utils/tty.hpp"

#include <libsmartcols/libsmartcols.h>

#include <iostream>
#include <string>

namespace libdnf::cli::output {

enum { COL_ARG_NAMES, COL_DESCR };

static struct libscols_table * create_repoinfo_table() {
    struct libscols_table * table = scols_new_table();
    scols_table_enable_noheadings(table, 1);
    scols_table_set_column_separator(table, ": ");
    if (libdnf::cli::utils::tty::is_interactive()) {
        scols_table_enable_colors(table, 1);
        //scols_table_enable_maxout(table, 1);
    }
    struct libscols_column *cl = scols_table_new_column(table, "Key", 2, SCOLS_FL_WRAP);
    scols_column_set_cmpfunc(cl, scols_cmpstr_cells, NULL);
    scols_table_new_column(table, "Value", 2, SCOLS_FL_WRAP);
    return table;
}

static void add_line_into_repoinfo_table(struct libscols_table * table, const char * id, const char * descr) {
    struct libscols_line * ln = scols_table_new_line(table, NULL);
    scols_line_set_data(ln, COL_ARG_NAMES, id);
    scols_line_set_data(ln, COL_DESCR, descr);
}

template <class Repo>
static void make_single_repoinfo_table(Repo repo) {

}

template <class Query>
static void print_repoinfo_table(Query query) {
    for ( auto & repo : query.get_data() ) {
        auto table = create_repoinfo_table();
        add_line_into_repoinfo_table(table, "Repo-id", repo->get_id().c_str());
        add_line_into_repoinfo_table(table, "Repo-name", repo->get_name().c_str());
        add_line_into_repoinfo_table(table, "Repo-revision", "");
        add_line_into_repoinfo_table(table, "Repo-updated", "");
        add_line_into_repoinfo_table(table, "Repo-pkgs", "");
        add_line_into_repoinfo_table(table, "Repo-available-pkgs", "");
        add_line_into_repoinfo_table(table, "Repo-size", "");
        add_line_into_repoinfo_table(table, "Repo-baseurl", "");
        add_line_into_repoinfo_table(table, "Repo-expire", "");
        add_line_into_repoinfo_table(table, "Repo-filename", "");
        scols_print_table(table);
        scols_unref_table(table);
        std::cout << std::endl;
    }
    /// TODO add total packages
}

} // namespace libdnf::cli::output

#endif  // LIBDNF_CLI_OUTPUT_REPOINFO_HPP
