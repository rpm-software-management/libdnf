#include "libdnf-cli/output/output.hpp"

#include <libsmartcols/libsmartcols.h>

#include <iostream>

enum { COL_KEY, COL_VAL };

namespace libdnf::cli::output {
struct libscols_table * make_dnf_info_tb(const s_vec_map & vec_map) {
    /// Build and return info table
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
        auto ln = scols_table_new_line(tb, NULL);
        auto lv = scols_table_new_line(tb, NULL);

        scols_line_set_data(ln, COL_KEY, "name");
        scols_line_set_data(ln, COL_VAL, std::get<std::string>(pkg["name"]).c_str());

        scols_line_set_data(lv, COL_KEY, "version");
        scols_line_set_data(lv, COL_VAL, std::get<std::string>(pkg["version"]).c_str());

        if (std::next(it) != vec_map.end()) {
            std::cout << std::endl;
        }
    }
    return tb;
}
void print_rpm_info(const s_vec_map & vec_map) {
    auto tb = make_dnf_info_tb(vec_map);
    scols_print_table(tb);
    scols_unref_table(tb);
}

char ** print_rpm_info_to_string(const s_vec_map & vec_map) {
    char ** data = {};
    auto tb = make_dnf_info_tb(vec_map);
    scols_print_table_to_string(tb, data);
    scols_unref_table(tb);
    return data;
}


}  // namespace libdnf::cli::output
