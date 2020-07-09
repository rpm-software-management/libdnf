#include "libdnf-cli/output/output.hpp"

#include <libsmartcols/libsmartcols.h>

namespace libdnf::cli::output {

enum { COL_KEY, COL_VAL };

struct libscols_table * make_dnf_repolist_tb(const s_vec_map & vec_map) {
    /*
     * Build and return repolist table
     */
    /// definitions
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

    for (auto it = vec_map.begin(); it != vec_map.end(); it++) {
        auto map = *it;
        ln = scols_table_new_line(tb, NULL);

        scols_line_set_data(ln, COL_KEY, std::get<std::string>(map["id"]).c_str());
        scols_line_set_data(ln, COL_VAL, std::get<std::string>(map["name"]).c_str());
    }
    return tb;
}


void print_repo_list(const s_vec_map & vec_map) {
    auto tb = make_dnf_repolist_tb(vec_map);
    scols_print_table(tb);
    scols_unref_table(tb);
}

char ** print_irepo_list_to_string(const s_vec_map & vec_map) {
    char ** data = {};
    auto tb = make_dnf_repolist_tb(vec_map);
    scols_print_table_to_string(tb, data);
    scols_unref_table(tb);
    return data;
}


}  // namespace libdnf::cli::output
