#ifndef LIBDNF_UTILS_HPP
#define LIBDNF_UTILS_HPP

#define ASCII_LOWERCASE "abcdefghijklmnopqrstuvwxyz"
#define ASCII_UPPERCASE "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
#define ASCII_LETTERS ASCII_LOWERCASE ASCII_UPPERCASE
#define DIGITS "0123456789"
#define REPOID_CHARS ASCII_LETTERS DIGITS "-_.:"

#include "libdnf/sack/advisory.hpp"
#include "libdnf/dnf-utils.h"

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

DEPRECATED("advisory now provides getApplicablePackages and each AdvisoryModule provides isApplicable.")
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
std::string getRealpath(const std::string & path);
bool isSubdirectory(const std::string & parent, const std::string & child);
std::string pathJoin(const std::string & p1, const std::string & p2);
std::vector<std::string> getDirContent(const std::string &dirPath);

/**
*  Creates an alphabetically sorted list of all files in `directories` which names match the `pattern_file_name`.
*  If a file with the same name is in multiple directories, only the first file found is added to the list.
*  Directories are traversed in the same order as they are in the input vector.
*/
std::vector<std::string> createSortedFileList(
    const std::vector<std::string> & directories, const std::string & file_name_pattern);

/**
* @brief Decompress file.
*
* @param inPath Path to input (compressed) file
* @param outPath Path to output (decompressed) file
* @param outMode Mode of created (output) file
* @param compressType Type of compression (".bz2", ".gz", ...), nullptr - detect from inPath filename. Defaults to nullptr.
*/
void decompress(const char * inPath, const char * outPath, mode_t outMode, const char * compressType = nullptr);

/**
* @brief checksum file and return if matching.
*
* @param type Checksum type ("sha", "sha1", "sha256" etc). Raises libdnf::Error if invalid.
* @param inPath Path to input file
* @param valid_checksum hexadecimal encoded checksum string.
*/
bool checksum_check(const char * type, const char * inPath, const char * valid_checksum);
/**
* @brief checksum file and return checksum.
*
* @param type Checksum type ("sha", "sha1", "sha256" etc). Raises libdnf::Error if invalid.
* @param inPath Path to input file
*/
std::string checksum_value(const char * type, const char * inPath);
}

namespace numeric {
/**
* @brief Return a random number in the given range.
*
* Each possible value has an equal likelihood of being produced.
*
* @param min  lower bound of the range
* @param max  upper bound of the range
*/
int random(const int min, const int max);
}

}

#endif //LIBDNF_UTILS_HPP
