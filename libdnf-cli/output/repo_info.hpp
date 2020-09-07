#ifndef LIBDNF_CLI_OUTPUT_REPO_INFO_HPP
#define LIBDNF_CLI_OUTPUT_REPO_INFO_HPP

#include <string>

#include <libsmartcols/libsmartcols.h>

#include "libdnf/rpm/solv_sack.hpp"

namespace libdnf::cli::output {

std::string i_to_str(const int & val) {
    return std::to_string(val);
}

std::string i_to_str(const libdnf::rpm::PackageId & val) {
    return std::to_string(val.id);
}

template <class PkgSet>
struct libscols_table * make_dnf_repoinfo_tb(PkgSet & pset) {
    /// Build and return repoinfo table
    // definitions
    struct libscols_table * tb;

    enum { COL_KEY, COL_VAL };
    //setlocale(LC_ALL, "");

    // init table
    tb = scols_new_table();
    scols_table_enable_noheadings(tb, 1);
    scols_table_set_column_separator(tb, " : ");

    // init cols
    scols_table_new_column(tb, "KEY", 2, SCOLS_FL_WRAP);
    scols_table_new_column(tb, "VAL", 2, SCOLS_FL_WRAP);

    for (auto it = pset.begin(); it != pset.end(); it++) {
        auto pkg = *it;

        auto l1 = scols_table_new_line(tb, NULL);
        auto l2 = scols_table_new_line(tb, NULL);
        auto l3 = scols_table_new_line(tb, NULL);
        auto l4 = scols_table_new_line(tb, NULL);
        auto l5 = scols_table_new_line(tb, NULL);
        auto l6 = scols_table_new_line(tb, NULL);
        auto l7 = scols_table_new_line(tb, NULL);
        auto l8 = scols_table_new_line(tb, NULL);
        auto l9 = scols_table_new_line(tb, NULL);
        auto l10 = scols_table_new_line(tb, NULL);

        scols_line_set_data(l1, COL_KEY, "id");
        scols_line_set_data(l1, COL_VAL, i_to_str(pkg.get_id()).c_str());

        scols_line_set_data(l2, COL_KEY, "name");
        scols_line_set_data(l2, COL_VAL, pkg.get_name().c_str());

        scols_line_set_data(l3, COL_KEY, "revision");
        scols_line_set_data(l3, COL_VAL, "");

        scols_line_set_data(l4, COL_KEY, "updated");
        scols_line_set_data(l4, COL_VAL, "");

        scols_line_set_data(l5, COL_KEY, "pkgs");
        scols_line_set_data(l5, COL_VAL, "");

        scols_line_set_data(l6, COL_KEY, "available-pkgs");
        scols_line_set_data(l6, COL_VAL, "");

        scols_line_set_data(l7, COL_KEY, "size");
        scols_line_set_data(l7, COL_VAL, "");

        scols_line_set_data(l8, COL_KEY, "baseurl");
        scols_line_set_data(l8, COL_VAL, pkg.get_baseurl().c_str());

        scols_line_set_data(l9, COL_KEY, "expire");
        scols_line_set_data(l9, COL_VAL, "");

        scols_line_set_data(l10, COL_KEY, "filename");
        scols_line_set_data(l10, COL_VAL, "");

        if (it != pset.end()) {
            auto le = scols_table_new_line(tb, NULL);
            scols_line_set_data(le, COL_KEY, "");
            scols_line_set_data(le, COL_VAL, "");
        }
    }
    return tb;
}

template <class PkgSet>
void print_repo_info(const PkgSet & pset)
{
    auto tb = make_dnf_repoinfo_tb(pset);
    scols_print_table(tb);
    scols_unref_table(tb);
}

template <class PkgSet>
char ** print_repo_info_to_string(const PkgSet & pset)
{
    char ** data = {};
    auto tb = make_dnf_repoinfo_tb(pset);
    scols_print_table_to_string(tb, data);
    scols_unref_table(tb);
    return data;
}

}  // namespace libdnf::cli::output

#endif
