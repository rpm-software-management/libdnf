#ifndef LIBDNF_CLI_OUTPUT_REPO_LIST_HPP
#define LIBDNF_CLI_OUTPUT_REPO_LIST_HPP

#include <libsmartcols/libsmartcols.h>

namespace libdnf::cli::output {


template <class RepoSet>
struct libscols_table * make_dnf_repolist_tb(const RepoSet & rset) {
    /*
     * Build and return repolist table
     */
    /// definitions
    enum { COL_KEY, COL_VAL };

    struct libscols_table * tb;
    struct libscols_line * ln;
    struct libscols_symbols * sy = scols_new_symbols();

    setlocale(LC_ALL, "");

    // init table
    tb = scols_new_table();
    scols_table_enable_maxout(tb, 1);

    /// init cols
    scols_table_new_column(tb, "Repo ID", 1, SCOLS_FL_TREE);
    scols_table_new_column(tb, "Repo Name", 1, SCOLS_FL_WRAP);

    // init tree symbols
    scols_symbols_set_branch(sy, "");
    scols_symbols_set_right(sy, "");
    scols_symbols_set_vertical(sy, "");
    scols_table_set_symbols(tb, sy);

    for (auto it = rset.begin(); it != rset.end(); it++) {
        auto map = *it;
        ln = scols_table_new_line(tb, NULL);

        scols_line_set_data(ln, COL_KEY, rset.get_id().c_str());
        scols_line_set_data(ln, COL_VAL, rset.get_name.c_str());
    }
    return tb;
}

template <class RepoSet>
void print_repo_list(const RepoSet & rset)
{
    auto tb = make_dnf_repolist_tb(rset);
    scols_print_table(tb);
    scols_unref_table(tb);
}

template <class RepoSet>
char ** print_repo_list_to_string(const RepoSet & rset)
{
    char ** data = {};
    auto tb = make_dnf_repoinfo_tb(rset);
    scols_print_table_to_string(tb, data);
    scols_unref_table(tb);
    return data;
}

}  // namespace libdnf::cli::output

#endif  // LIBDNF_CLI_OUTPUT_REPO_LIST_HPP
