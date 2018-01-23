#ifndef LIBDNF_UTILS_HPP
#define LIBDNF_UTILS_HPP

#include <string>
#include <vector>

namespace string {
std::vector<std::string> split(const std::string &source, const char *delimiter);
}

namespace filesystem {
bool exists (const std::string &name);
}

#endif //LIBDNF_UTILS_HPP
