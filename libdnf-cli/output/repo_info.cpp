#include "libdnf-cli/output/output.hpp"

#include <libsmartcols/libsmartcols.h>

namespace libdnf::cli::output {


enum { COL_KEY, COL_VAL };

struct libscols_table * make_dnf_repoinfo_tb(const s_vec_map & vec_map) {
    /// Build and return repoinfo table
    // definitions
    struct libscols_table * tb;

    setlocale(LC_ALL, "");

    // init table
    tb = scols_new_table();
    scols_table_enable_noheadings(tb, 1);
    scols_table_set_column_separator(tb, " : ");

    // init cols
    scols_table_new_column(tb, "KEY", 2, SCOLS_FL_WRAP);
    scols_table_new_column(tb, "VAL", 2, SCOLS_FL_WRAP);

    for (auto it = vec_map.begin(); it != vec_map.end(); it++) {
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
        scols_line_set_data(l1, COL_VAL, std::get<std::string>(pkg["id"]).c_str());

        scols_line_set_data(l2, COL_KEY, "name");
        scols_line_set_data(l2, COL_VAL, std::get<std::string>(pkg["name"]).c_str());

        scols_line_set_data(l3, COL_KEY, "revision");
        scols_line_set_data(l3, COL_VAL, std::get<std::string>(pkg["revision"]).c_str());

        scols_line_set_data(l4, COL_KEY, "updated");
        scols_line_set_data(l4, COL_VAL, std::get<std::string>(pkg["updated"]).c_str());

        scols_line_set_data(l5, COL_KEY, "pkgs");
        scols_line_set_data(l5, COL_VAL, std::get<std::string>(pkg["pkgs"]).c_str());

        scols_line_set_data(l6, COL_KEY, "available-pkgs");
        scols_line_set_data(l6, COL_VAL, std::get<std::string>(pkg["available-pkgs"]).c_str());

        scols_line_set_data(l7, COL_KEY, "size");
        scols_line_set_data(l7, COL_VAL, std::get<std::string>(pkg["size"]).c_str());

        scols_line_set_data(l8, COL_KEY, "baseurl");
        scols_line_set_data(l8, COL_VAL, std::get<std::string>(pkg["baseurl"]).c_str());

        scols_line_set_data(l9, COL_KEY, "expire");
        scols_line_set_data(l9, COL_VAL, std::get<std::string>(pkg["expire"]).c_str());

        scols_line_set_data(l10, COL_KEY, "filename");
        scols_line_set_data(l10, COL_VAL, std::get<std::string>(pkg["filename"]).c_str());

        if (std::next(it) != vec_map.end()) {
            auto le = scols_table_new_line(tb, NULL);
            scols_line_set_data(le, COL_KEY, "");
            scols_line_set_data(le, COL_VAL, "");
        }
    }
    return tb;
}

void print_repo_info(const s_vec_map & vec_map) {
    auto tb = make_dnf_repoinfo_tb(vec_map);
    scols_print_table(tb);
    scols_unref_table(tb);
}

char ** print_repo_info_to_string(const s_vec_map & vec_map) {
    char ** data = {};
    auto tb = make_dnf_repoinfo_tb(vec_map);
    scols_print_table_to_string(tb, data);
    scols_unref_table(tb);
    return data;
}

}  // namespace libdnf::cli::output
