#include "File.hpp"
#include "CompressedFile.hpp"
#include "utils.hpp"

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sstream>
#include <utility>
#include <iostream>

#include <cerrno>
#include <system_error>

extern "C" {
#   include <solv/solv_xfopen.h>
};

namespace libdnf {

std::unique_ptr<File> File::newFile(const std::string &filePath)
{
    if (solv_xfopen_iscompressed(filePath.c_str()) == 1) {
        return std::unique_ptr<CompressedFile>(new CompressedFile(filePath));
    } else {
        return std::unique_ptr<File>(new File(filePath));
    }
}

File::File(const std::string &filePath)
        : filePath(filePath)
        , file(nullptr)
{}

File::~File()
{
    try {
        close();
    } catch (IOError &) {}
}

void File::open(const char *mode)
{
    file = fopen(filePath.c_str(), mode);
    if (!file) {
        throw OpenError(filePath, std::system_category().message(errno));
    }
}

void File::close()
{
    if (file == nullptr)
        return;

    if (fclose(file) != 0) {
        file = nullptr;
        throw CloseError(filePath);
    }

    file = nullptr;
}

size_t File::read(char *buffer, size_t count)
{
    size_t ret = fread(buffer, sizeof(char), count, file);

    if (ret != count && ferror(file) != 0) {
        throw ReadError("Error while reading file \"" + filePath + "\".");
    }

    return ret;
}

bool File::readLine(std::string &line)
{
    char *buffer = nullptr;
    size_t size = 0;
    if (getline(&buffer, &size, file) == -1) {
        free(buffer);
        return false;
    }

    line = buffer;
    free(buffer);

    return true;
}

std::string File::getContent()
{
    if (!file) {
        throw NotOpenedException(filePath);
    }

    fseek(file, 0, SEEK_END);
    auto fileSize = ftell(file);
    if (fileSize == -1)
        throw IOError(filePath);
    rewind(file);
    std::string content(fileSize, '\0');
    read(&content.front(), fileSize);

    return content;
}

}
