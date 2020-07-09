#ifndef LIBDNF_CLI_OUTPUT_OUTPUT_HPP
#define LIBDNF_CLI_OUTPUT_OUTPUT_HPP

#include <map>
#include <string>
#include <variant>
#include <vector>

namespace libdnf::cli::output {

typedef std::map<std::string, std::variant<std::string>> s_map;
typedef std::vector<s_map> s_vec_map;
typedef std::map<std::string, s_vec_map> s_map_vec_map;

void print_transaction_table(const s_map_vec_map & map_vec_map);

void print_rpm_info(const s_vec_map & vec_map);

void print_repo_info(const s_vec_map & vec_map);

void print_repo_list(const s_vec_map & vec_map);

char ** print_transaction_table_to_string(const s_map_vec_map & map_vec_map);

char ** print_rpm_info_to_string(const s_vec_map & vec_map);

char ** print_repo_info_to_string(const s_vec_map & vec_map);

char ** print_repo_list_to_string(const s_vec_map & vec_map);

}  // namespace libdnf::cli::output

#endif  // LIBDNF_CLI_UTILS_SMARTCOLS
