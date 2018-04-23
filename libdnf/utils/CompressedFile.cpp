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

    while (true) {
        constexpr size_t fileSize = 2048;
        auto content = new char[fileSize + 1];
        auto bytesRead = read(content, fileSize);

        ss << content;
        delete [] content;

        if (bytesRead != fileSize) {
            return ss.str();
        }
    }
}
