#include "utils.hpp"

#include <algorithm>
#include <sys/stat.h>
#include <dirent.h>
#include <cstring>
#include <stdexcept>

std::vector<std::string> string::split(const std::string &source, const char *delimiter, int maxSplit)
{
    if (source.empty())
        throw std::runtime_error{"Source cannot be empty"};

    std::string::size_type tokenBeginIndex = 0;
    std::vector<std::string> container;

    while ((tokenBeginIndex = source.find_first_not_of(delimiter, tokenBeginIndex)) != std::string::npos) {
        if (maxSplit != -1 && ((int) (container.size() + 1 )) == maxSplit) {
            container.emplace_back(source.substr(tokenBeginIndex));
            break;
        }

        auto tokenEndIndex = source.find_first_of(delimiter, tokenBeginIndex);
        container.emplace_back(source.substr(tokenBeginIndex, tokenEndIndex - tokenBeginIndex));
        tokenBeginIndex = tokenEndIndex;
    }

    if (container.empty()) {
        throw std::runtime_error{"No delimiter found in source: " + source};
    }

    return container;
}

std::vector<std::string> string::rsplit(const std::string &source, const char *delimiter, int maxSplit)
{
    if (source.empty())
        throw std::runtime_error{"Source cannot be empty"};

    std::string sequence = source;
    std::string::size_type tokenBeginIndex = 0;
    std::vector<std::string> container;

    while ((tokenBeginIndex = sequence.find_last_of(delimiter)) != std::string::npos) {
        if (maxSplit != -1 && ((int) (container.size() + 1 )) == maxSplit) {
            container.emplace_back(source.substr(0, tokenBeginIndex));
            break;
        }

        container.emplace(container.begin(), source.substr(tokenBeginIndex + 1));
        sequence = sequence.substr(0, tokenBeginIndex);
    }

    if (container.empty()) {
        throw std::runtime_error{"No delimiter found in source: " + source};
    }

    return container;
}

std::string string::trimSuffix(const std::string &source, const std::string &suffix)
{
    if (source.length() < suffix.length())
        throw std::runtime_error{"Suffix cannot be longer than source"};

    if (string::endsWith(source, suffix))
        return source.substr(0, source.length() - suffix.length());

    throw std::runtime_error{"Suffix '" + suffix + "' not found"};
}

std::string string::trimPrefix(const std::string &source, const std::string &prefix)
{
    if (source.length() < prefix.length())
        throw std::runtime_error{"Prefix cannot be longer than source"};

    if (string::startsWith(source, prefix))
        return source.substr(prefix.length() - 1);

    throw std::runtime_error{"Prefix '" + prefix + "' not found"};
}

bool string::startsWith(const std::string &source, const std::string &toMatch)
{
    return source.find(toMatch) == 0;
}

bool string::endsWith(const std::string &source, const std::string &toMatch)
{
    auto it = toMatch.begin();
    return source.size() >= toMatch.size() &&
           std::all_of(std::next(source.begin(), source.size() - toMatch.size()), source.end(), [&it](const char &c) {
               return c == *(it++);
           });
}

bool filesystem::exists(const std::string &name)
{
    struct stat buffer;
    return stat(name.c_str(), &buffer) == 0;
}

std::vector<std::string> filesystem::getDirContent(const std::string &dirPath)
{
    std::vector<std::string> content{};
    struct dirent *ent;
    DIR *dir = opendir(dirPath.c_str());

    if (dir != nullptr) {
        while ((ent = readdir(dir)) != nullptr) {
            content.emplace_back(ent->d_name);
        }
        closedir (dir);
    }

    return content;
}
