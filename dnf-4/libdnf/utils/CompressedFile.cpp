#include "CompressedFile.hpp"
#include <utility>
#include <sstream>

extern "C" {
#   include <solv/solv_xfopen.h>
};

#include <cerrno>
#include <system_error>

namespace libdnf {

CompressedFile::CompressedFile(const std::string &filePath)
: File(filePath) {}

CompressedFile::~CompressedFile() = default;

void CompressedFile::open(const char *mode)
{
    // There are situations when solv_xfopen returns NULL but it does not set errno.
    // In this case errno value is random, so we set a default value.
    errno = 0;
    file = solv_xfopen(filePath.c_str(), mode);
    if (!file) {
        if (errno) {
            throw OpenException(filePath, std::system_category().message(errno));
        }
        throw OpenException(filePath);
    }
}

std::string CompressedFile::getContent()
{
    if (!file) {
        throw NotOpenedException(filePath);
    }

    constexpr size_t bufferSize = 4096;
    char buffer[bufferSize];
    std::ostringstream ss;
    size_t bytesRead;

    do {
        bytesRead = read(buffer, bufferSize);
        ss.write(buffer, bytesRead);
    } while (bytesRead == bufferSize);

    return ss.str();
}

}
