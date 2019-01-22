#include "CompressedFile.hpp"
#include <utility>
#include <sstream>

extern "C" {
#   include <solv/solv_xfopen.h>
};

namespace libdnf {

CompressedFile::CompressedFile(const std::string &filePath)
: File(filePath) {}

CompressedFile::~CompressedFile() = default;

void CompressedFile::open(const char *mode)
{
    file = solv_xfopen(filePath.c_str(), mode);
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
