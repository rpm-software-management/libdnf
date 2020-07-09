#include "libdnf-cli/output/output.hpp"

#include <libsmartcols/libsmartcols.h>

namespace libdnf::cli::output {

enum { COL_NAME, COL_VERS, COL_ARCH, COL_REPO, COL_SIZE };

struct libscols_line * make_transaction_ln(
    /// Build and return lines given parent line
    std::string line_name,
    struct libscols_table * tb,
    struct libscols_line * parent_ln,
    const s_vec_map & vec_map) {
    /// make header line
    auto ln = parent_ln;
    scols_line_set_data(ln, COL_NAME, line_name.c_str());
    scols_line_set_data(ln, COL_VERS, "");
    scols_line_set_data(ln, COL_ARCH, "");
    scols_line_set_data(ln, COL_REPO, "");
    scols_line_set_data(ln, COL_SIZE, "");

    /// make child lines
    for (auto it = vec_map.begin(); it != vec_map.end(); it++) {
        auto map = *it;
        ln = scols_table_new_line(tb, parent_ln);

        scols_line_set_data(ln, COL_NAME, std::get<std::string>(map["name"]).c_str());
        scols_line_set_data(ln, COL_VERS, std::get<std::string>(map["version"]).c_str());
        scols_line_set_data(ln, COL_ARCH, std::get<std::string>(map["arch"]).c_str());
        scols_line_set_data(ln, COL_REPO, std::get<std::string>(map["repo"]).c_str());
        scols_line_set_data(ln, COL_SIZE, std::get<std::string>(map["size"]).c_str());
    }
    return ln;
}

struct libscols_table * make_transaction_tb(s_map_vec_map map_vec_map) {
    /// Build transaction table and return it
    // definitions
    struct libscols_table * tb;
    struct libscols_line *ins_ln,  // installing
        *dep_ln,                   // installing dependencies
        *rem_ln;                   // removing
    struct libscols_symbols * sy;

    setlocale(LC_ALL, "");

    // init table
    tb = scols_new_table();
    scols_table_enable_maxout(tb, 1);

    // init cols
    scols_table_new_column(tb, "Package", 0.2, SCOLS_FL_TREE);
    scols_table_new_column(tb, "Version", 0.2, SCOLS_FL_WRAP);
    scols_table_new_column(tb, "Arch", 0.2, SCOLS_FL_WRAP);
    scols_table_new_column(tb, "Repository", 0.2, SCOLS_FL_WRAP);
    scols_table_new_column(tb, "Size", 0.2, SCOLS_FL_WRAP);

    // init rows
    ins_ln = scols_table_new_line(tb, NULL);
    dep_ln = scols_table_new_line(tb, NULL);
    rem_ln = scols_table_new_line(tb, NULL);

    // init symbols
    sy = scols_new_symbols();
    scols_symbols_set_right(sy, " ");
    scols_symbols_set_branch(sy, " ");
    scols_symbols_set_vertical(sy, " ");
    scols_table_set_symbols(tb, sy);

    // split map in separate vectors of maps
    auto ins_list = map_vec_map["install"];
    auto dep_list = map_vec_map["dependency"];
    auto rem_list = map_vec_map["remove"];

    make_transaction_ln("Installing", tb, ins_ln, ins_list);
    make_transaction_ln("Installing dependencies", tb, dep_ln, dep_list);
    make_transaction_ln("Removing", tb, rem_ln, rem_list);

    return tb;
}


void print_transaction_table(const s_map_vec_map & map_vec_map) {
    auto tb = make_transaction_tb(map_vec_map);
    scols_print_table(tb);
    scols_unref_table(tb);
}

char ** print_transaction_table_to_string(const s_map_vec_map & map_vec_map) {
    char ** data = {};
    auto tb = make_transaction_tb(map_vec_map);
    scols_print_table_to_string(tb, data);
    scols_unref_table(tb);
    return data;
}

}  // namespace libdnf::cli::output
