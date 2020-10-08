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

#include <libsmartcols/libsmartcols.h>

namespace libdnf::cli::output {i

enum { COL_ARG_NAMES, COL_DESCR };

static struct libscols_table * create_repoinfo_table() {
}

static void add_line_into_repoinfo_table(
    struct libscols_table * table,
    bool with_status,
    const char * id,
    const char * name){}

template <class Query>
static void print_repoinfo_table(Query query, bool with_status, int c) {
    auto table = create_repoinfo_table(with_status);
    for ( auto & repo : query.get_data() ) {
        add_line_into_repoinfo_table(
            table,
            with_status,
            repo->get_id(),
            repo->get_name());
    }
    auto cl = scols_table_get_column(table, c);
    scols_sort_table(table, cl);

    scols_print_table(table);
    scols_unref_table(table);
}
/*
 *
Repo-id            : virtualbox
Repo-name          : Fedora 31 - x86_64 - VirtualBox
Repo-revision      : 1599222917
Repo-updated       : Fri 04 Sep 2020 02:35:18 PM CEST
Repo-pkgs          : 45
Repo-available-pkgs: 45
Repo-size          : 4.2 G
Repo-baseurl       : http://download.virtualbox.org/virtualbox/rpm/fedora/31/x86_64
Repo-expire        : 172,800 second(s) (last: Thu 08 Oct 2020 02:22:53 PM CEST)
Repo-filename      : /etc/yum.repos.d/virtualbox.repo
Total packages: 88,800

 * */
} // namespace libdnf::cli::output

#endif  // LIBDNF_CLI_OUTPUT_REPOINFO_HPP
