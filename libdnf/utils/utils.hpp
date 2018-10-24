#ifndef LIBDNF_UTILS_HPP
#define LIBDNF_UTILS_HPP

#include <functional>
#include <string>
#include <vector>

/**
* @brief Object calls user defined function during its destruction.
*
* User function is passed as parameter of object constructor.
* Example with user function defined as lambda:
*
*    Finalizer finalizerObject([&tmpDirectory](){
*        remove(tmpDirectory);
*    });
*/
class Finalizer {
public:
    Finalizer(const std::function<void()> & func) : func(func) {}
    Finalizer(std::function<void()> && func) : func(std::move(func)) {}
    ~Finalizer() { func(); }
private:
    std::function<void()> func;
};

namespace string {
std::vector<std::string> split(const std::string &source, const char *delimiter, int maxSplit = -1);
std::vector<std::string> rsplit(const std::string &source, const char *delimiter, int maxSplit = -1);
std::string trimSuffix(const std::string &source, const std::string &suffix);
std::string trimPrefix(const std::string &source, const std::string &prefix);
bool startsWith(const std::string &source, const std::string &toMatch);
bool endsWith(const std::string &source, const std::string &toMatch);
}

bool haveFilesSameContent(const char * filePath1, const char * filePath2);

namespace filesystem {
bool exists (const std::string &name);
bool isDIR(const std::string& dirPath);
std::vector<std::string> getDirContent(const std::string &dirPath);
}

#endif //LIBDNF_UTILS_HPP
