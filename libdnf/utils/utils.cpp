#include "utils.hpp"

#include <sys/stat.h>
#include <vector>

std::vector<std::string> string::split(const std::string &source, const char *delimiter)
{
    if (source.empty())
        throw "Source cannot be empty";

    std::string::size_type tokenBeginIndex = 0;
    std::vector<std::string> container;

    while ((tokenBeginIndex = source.find_first_not_of(delimiter, tokenBeginIndex)) != std::string::npos) {
        auto tokenEndIndex = source.find_first_of(delimiter, tokenBeginIndex);
        container.emplace_back(source.substr(tokenBeginIndex, tokenEndIndex - tokenBeginIndex));
        tokenBeginIndex = tokenEndIndex;
    }

    if (container.empty()) {
        throw "No delimiter found in source: " + source;
    }

    return container;
}

bool filesystem::exists(const std::string &name)
{
    struct stat buffer;
    return stat(name.c_str(), &buffer) == 0;
}