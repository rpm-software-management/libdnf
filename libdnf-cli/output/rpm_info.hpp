#ifndef LIBDNF_CLI_OUTPUT_RPM_INFO_HPP
#define LIBDNF_CLI_OUTPUT_RPM_INFO_HPP

#include <libsmartcols/libsmartcols.h>

namespace libdnf::cli::output {

template <class PkgSet>
struct libscols_table * make_rpm_info_tb(const PkgSet & pset) {
    /// Build and return info table
    enum { COL_KEY, COL_VAL };
    struct libscols_table * tb;
    // setlocale(LC_ALL, "");

    // init table
    tb = scols_new_table();
    scols_table_enable_noheadings(tb, 1);
    scols_table_set_column_separator(tb, " : ");

    // init cols
    scols_table_new_column(tb, "KEY", 2, SCOLS_FL_WRAP);
    scols_table_new_column(tb, "VAL", 2, SCOLS_FL_WRAP);

    for (auto it = pset.begin(); it != pset.end(); it++) {
        auto pkg = *it;
        auto ln = scols_table_new_line(tb, NULL);
        auto lv = scols_table_new_line(tb, NULL);

        scols_line_set_data(ln, COL_KEY, "name");
        scols_line_set_data(ln, COL_VAL, pkg.get_name().c_str());

        scols_line_set_data(lv, COL_KEY, "version");
        scols_line_set_data(lv, COL_VAL, pkg.get_version().c_str());

        //if (it != pset.end()) {
        //    std::cout << std::endl;
        //}
    }
    return tb;
}

template <class PkgSet>
void print_rpm_info(const PkgSet & pset) {
    auto tb = make_rpm_info_tb(pset);
    scols_print_table(tb);
    scols_unref_table(tb);
}

template <class PkgSet>
char ** print_rpm_info_to_string(const PkgSet & pset) {
    char ** data = {};
    auto tb = make_rpm_info_tb(pset);
    scols_print_table_to_string(tb, data);
    scols_unref_table(tb);
    return data;
}

}  // namespace libdnf::cli::output

#endif  // LIDNF_CLI_OUTPUT_RPM_INFO_HPP
