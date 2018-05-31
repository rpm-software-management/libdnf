#include "CompressedFile.hpp"
#include <utility>
#include <sstream>

extern "C" {
#   include <solv/solv_xfopen.h>
};

libdnf::CompressedFile::CompressedFile(const std::string &filePath)
        : File(filePath)
{}

libdnf::CompressedFile::~CompressedFile() = default;

void libdnf::CompressedFile::open(const char *mode)
{
    file = solv_xfopen(filePath.c_str(), mode);
}

std::string libdnf::CompressedFile::getContent()
{
    std::ostringstream ss;

    if (!file) {
        throw NotOpenedException(filePath);
    }

    constexpr size_t bufferSize = 4096;
    char buffer[bufferSize + 1];

    while (true) {
        auto bytesRead = read(buffer, bufferSize);
        buffer[bytesRead] = '\0';

        ss << buffer;

        if (bytesRead != bufferSize) {
            break;
        }
    }

    return ss.str();
}
