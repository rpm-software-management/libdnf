#ifndef LIBDNF_UTILS_HPP
#define LIBDNF_UTILS_HPP

#define ASCII_LOWERCASE "abcdefghijklmnopqrstuvwxyz"
#define ASCII_UPPERCASE "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
#define ASCII_LETTERS ASCII_LOWERCASE ASCII_UPPERCASE
#define DIGITS "0123456789"
#define REPOID_CHARS ASCII_LETTERS DIGITS "-_.:"

#include "libdnf/sack/advisory.hpp"

#include <functional>
#include <string>
#include <vector>

#include <sys/types.h>

namespace libdnf {

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

bool isAdvisoryApplicable(Advisory & advisory, DnfSack * sack);

namespace string {
inline std::string fromCstring(const char * cstring) { return cstring ? cstring : ""; }
std::vector<std::string> split(const std::string &source, const char *delimiter, int maxSplit = -1);
std::vector<std::string> rsplit(const std::string &source, const char *delimiter, int maxSplit = -1);
std::string trimSuffix(const std::string &source, const std::string &suffix);
std::string trimPrefix(const std::string &source, const std::string &prefix);
std::string trim(const std::string &source);
bool startsWith(const std::string &source, const std::string &toMatch);
bool endsWith(const std::string &source, const std::string &toMatch);
}

bool haveFilesSameContent(const char * filePath1, const char * filePath2);
bool updateFile(const char * filePath, const char * newFileContent);

namespace filesystem {
bool exists (const std::string &name);
bool isDIR(const std::string& dirPath);
std::vector<std::string> getDirContent(const std::string &dirPath);

/**
* @brief Decompress file.
*
* @param inPath Path to input (compressed) file
* @param outPath Path to output (decompressed) file
* @param outMode Mode of created (output) file
* @param compressType Type of compression (".bz2", ".gz", ...), nullptr - detect from inPath filename. Defaults to nullptr.
*/
void decompress(const char * inPath, const char * outPath, mode_t outMode, const char * compressType = nullptr);
}

}

#endif //LIBDNF_UTILS_HPP
