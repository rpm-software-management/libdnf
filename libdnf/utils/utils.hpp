#ifndef LIBDNF_UTILS_HPP
#define LIBDNF_UTILS_HPP

#include <string>
#include <vector>

namespace string {
std::vector<std::string> split(const std::string &source, const char *delimiter, int maxSplit = -1);
std::vector<std::string> rsplit(const std::string &source, const char *delimiter, int maxSplit = -1);
std::string trimSuffix(const std::string &source, const std::string &suffix);
std::string trimPrefix(const std::string &source, const std::string &prefix);
bool startsWith(const std::string &source, const std::string &toMatch);
bool endsWith(const std::string &source, const std::string &toMatch);
}

namespace filesystem {
bool exists (const std::string &name);
std::vector<std::string> getDirContent(const std::string &dirPath);
}

#endif //LIBDNF_UTILS_HPP