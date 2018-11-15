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

extern "C" {
#   include <solv/solv_xfopen.h>
};

std::shared_ptr<libdnf::File> libdnf::File::newFile(const std::string &filePath)
{
    if (solv_xfopen_iscompressed(filePath.c_str()) == 1) {
        return std::make_shared<libdnf::CompressedFile>(filePath);
    } else {
        return std::make_shared<libdnf::File>(filePath);
    }
}

libdnf::File::File(const std::string &filePath)
        : filePath(filePath)
        , file(nullptr)
{}

libdnf::File::~File()
{
    try {
        close();
    } catch (IOException &) {}
}

void libdnf::File::open(const char *mode)
{
    file = fopen(filePath.c_str(), mode);
    if (!file) {
        throw OpenException(filePath);
    }
}

void libdnf::File::close()
{
    if (file == nullptr)
        return;

    if (fclose(file) != 0) {
        throw CloseException(filePath);
    }

    file = nullptr;
}

size_t libdnf::File::read(char *buffer, size_t count)
{
    return fread(buffer, sizeof(char), count, file);
}

bool libdnf::File::readLine(std::string &line)
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

std::string libdnf::File::getContent()
{
    if (!file) {
        throw NotOpenedException(filePath);
    }

    fseek(file, 0, SEEK_END);
    auto fileSize = static_cast<size_t>(ftell(file));
    rewind(file);
    std::string content(fileSize, '\0');
    auto bytesRead = read(&content.front(), fileSize);

    if (bytesRead != fileSize) {
        throw ShortReadException(filePath);
    }

    return content;
}

