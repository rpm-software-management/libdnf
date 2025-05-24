#include "utils.hpp"
#include "libdnf/dnf-sack-private.hpp"
#include "libdnf/sack/advisorymodule.hpp"
#include <librepo/librepo.h>

#include <tinyformat/tinyformat.hpp>

#include <algorithm>
#include <sys/stat.h>
#include <dirent.h>
#include <cstring>
#include <glob.h>
#include <stdexcept>

extern "C" {
#include <solv/solv_xfopen.h>
};

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <memory>
#include <random>

namespace libdnf {

bool isAdvisoryApplicable(libdnf::Advisory & advisory, DnfSack * sack)
{
    for (auto & advisoryModule: advisory.getModules()) {
        if (advisoryModule.isApplicable()) {
            return true;
        }
    }

    return false;
}

namespace string {

std::vector<std::string> split(const std::string &source, const char *delimiter, int maxSplit)
{
    if (source.empty())
        throw std::runtime_error{"Source cannot be empty"};

    std::string::size_type tokenBeginIndex = 0;
    std::vector<std::string> container;

    while ((tokenBeginIndex = source.find_first_not_of(delimiter, tokenBeginIndex)) != source.npos) {
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

std::vector<std::string> rsplit(const std::string &source, const char *delimiter, int maxSplit)
{
    if (source.empty())
        throw std::runtime_error{"Source cannot be empty"};

    std::string sequence = source;
    std::string::size_type tokenBeginIndex = 0;
    std::vector<std::string> container;

    while ((tokenBeginIndex = sequence.find_last_of(delimiter)) != sequence.npos) {
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

std::string trimSuffix(const std::string &source, const std::string &suffix)
{
    if (source.length() < suffix.length())
        throw std::runtime_error{"Suffix cannot be longer than source"};

    if (endsWith(source, suffix))
        return source.substr(0, source.length() - suffix.length());

    throw std::runtime_error{"Suffix '" + suffix + "' not found"};
}

std::string trimPrefix(const std::string &source, const std::string &prefix)
{
    if (source.length() < prefix.length())
        throw std::runtime_error{"Prefix cannot be longer than source"};

    if (startsWith(source, prefix))
        return source.substr(prefix.length() - 1);

    throw std::runtime_error{"Prefix '" + prefix + "' not found"};
}

std::string trim(const std::string &source)
{
    size_t first = source.find_first_not_of(" \t");
    if (first == source.npos) return "";
    size_t last = source.find_last_not_of(" \t");
    return source.substr(first, (last - first + 1));
}

bool startsWith(const std::string & source, const std::string & toMatch)
{
    return source.compare(0, toMatch.size(), toMatch) == 0;
}

bool endsWith(const std::string & source, const std::string & toMatch)
{
    auto toMatchSize = toMatch.size();
    return source.size() >= toMatchSize && source.compare(
        source.size() - toMatchSize, toMatchSize, toMatch) == 0;
}

}

bool haveFilesSameContent(const char * filePath1, const char * filePath2)
{
    static constexpr int BLOCK_SIZE = 4096;
    bool ret = false;
    int fd1 = -1;
    int fd2 = -1;
    do {
        if ((fd1 = open(filePath1, 0)) == -1)
            break;
        if ((fd2 = open(filePath2, 0)) == -1)
            break;
        auto len1 = lseek(fd1, 0, SEEK_END);
        auto len2 = lseek(fd2, 0, SEEK_END);
        if (len1 != len2)
            break;
        ret = true;
        if (len1 == 0)
            break;
        lseek(fd1, 0, SEEK_SET);
        lseek(fd2, 0, SEEK_SET);
        char buf1[BLOCK_SIZE], buf2[BLOCK_SIZE];
        ssize_t readed;
        do {
            readed = read(fd1, buf1, BLOCK_SIZE);
            auto readed2 = read(fd2, buf2, BLOCK_SIZE);
            if (readed2 != readed || memcmp(buf1, buf2, readed) != 0) {
                ret = false;
                break;
            }
        } while (readed == BLOCK_SIZE);
    } while (false);

    if (fd1 != -1)
        close(fd1);
    if (fd2 != -1)
        close(fd2);
    return ret;
}

static bool
saveFile(const char * filePath, const char * newFileContent, unsigned long newFileContentLen)
{
    int fd = -1;
    if ((fd = open(filePath,  O_WRONLY | O_CREAT | O_TRUNC, 0644)) == -1)
        return false;
    auto writtenSize = write(fd, newFileContent, newFileContentLen);
    close(fd);
    return (static_cast<unsigned long>(writtenSize) == newFileContentLen);
}

bool updateFile(const char * filePath, const char * newFileContent)
{
    static constexpr int BLOCK_SIZE = 4096;
    int fd = -1;
    const char * tmpFileContent = newFileContent;
    auto newFileContentLen = strlen(newFileContent);

    if ((fd = open(filePath,  O_RDONLY)) == -1)
        return saveFile(filePath, newFileContent, newFileContentLen);
    auto len = lseek(fd, 0, SEEK_END);
    if (len < 0 || (unsigned long)len != newFileContentLen) {
        close(fd);
        return saveFile(filePath, newFileContent, newFileContentLen);
    }
    // No need to update file when newFileContentLen and len of file == 0
    if (newFileContentLen > 0) {
        lseek(fd, 0, SEEK_SET);
        char buf1[BLOCK_SIZE];
        ssize_t readed;
        do {
            readed = read(fd, buf1, BLOCK_SIZE);
            if (readed < 0 && errno == EINTR)
                continue;
            if (readed < 0 || memcmp(buf1, tmpFileContent, readed) != 0) {
                close(fd);
                return saveFile(filePath, newFileContent, newFileContentLen);
            }
            tmpFileContent = tmpFileContent + BLOCK_SIZE;
        } while (readed == BLOCK_SIZE);
    }

    if (fd != -1)
        close(fd);
    return true;
}

namespace filesystem {

bool exists(const std::string &name)
{
    struct stat buffer;
    return stat(name.c_str(), &buffer) == 0;
}

bool isDIR(const std::string& dirPath)
{
    struct stat buf;
    lstat(dirPath.c_str(), &buf);
    return S_ISDIR(buf.st_mode);
}

std::string getRealpath(const std::string & path)
{
    char * resolved = realpath(path.c_str(), nullptr);
    if (!resolved) {
        //throw std::runtime_error("realpath error for \"" + path + "\": " + strerror(errno));
        return {};
    }
    std::unique_ptr<char, decltype(&::free)> resolved_owner(resolved, &::free);
    return resolved;
}

bool isSubdirectory(const std::string & parent, const std::string & child)
{
    std::string parent_path = getRealpath(parent);
    std::string child_path = getRealpath(child);

    if (parent_path.empty() || child_path.empty()) {
        return false;
    }

    if (parent_path.back() != '/')
        parent_path += '/';

    return child_path.compare(0, parent_path.size(), parent_path) == 0;
}

std::string pathJoin(const std::string & p1, const std::string & p2)
{
    auto ret = p1;
    if (ret.back() != '/')
        ret.push_back('/');
    return ret + p2;
}

std::vector<std::string> getDirContent(const std::string & dirPath)
{
    std::vector<std::string> content{};
    struct dirent *ent;
    DIR *dir = opendir(dirPath.c_str());

    if (dir != nullptr) {
        while ((ent = readdir(dir)) != nullptr) {
            const auto * file_name = ent->d_name;
            if (strcmp(file_name, "..") == 0 ||
                    strcmp(file_name, ".") == 0 )
                continue;

            content.emplace_back(pathJoin(dirPath, file_name));
        }
        closedir (dir);
    }

    return content;
}

std::vector<std::string> createSortedFileList(
    const std::vector<std::string> & directories, const std::string & file_name_pattern) {
    std::vector<std::string> paths;

    for (const auto & dir : directories) {
        const auto pattern = pathJoin(dir, file_name_pattern);
        glob_t globResult;
        auto ret = glob(pattern.c_str(), GLOB_MARK | GLOB_NOSORT, NULL, &globResult );
        if (ret != 0 && ret != GLOB_NOMATCH) {
            globfree(&globResult);
            continue;
        }
        for (size_t i = 0; i < globResult.gl_pathc; ++i) {
            auto path = globResult.gl_pathv[i];
            if (path[strlen(path)-1] == '/') {
                continue;
            }
            auto * path_fname = basename(path);
            bool found{false};
            for (const auto & path_in_list : paths) {
                if (strcmp(path_fname, basename(path_in_list.c_str())) == 0) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                paths.push_back(path);
            }
        }
        globfree(&globResult);
    }

    // sort all drop-in configuration files alphabetically by their names
    std::sort(paths.begin(), paths.end(), [](const std::string & p1, const std::string & p2) {
        return strcmp(basename(p1.c_str()), basename(p2.c_str())) < 0;
    });

    return paths;
}

void decompress(const char * inPath, const char * outPath, mode_t outMode, const char * compressType)
{
    auto inFd = open(inPath, O_RDONLY);
    if (inFd == -1)
        throw std::runtime_error(tfm::format("Error opening %s: %s", inPath, strerror(errno)));
    if (!compressType)
        compressType = inPath;
    auto inFile = solv_xfopen_fd(compressType, inFd, "r");
    if (inFile == NULL) {
        close(inFd);
        throw std::runtime_error(tfm::format("solv_xfopen_fd: Can't open stream for %s", inPath));
    }
    auto outFd = open(outPath, O_WRONLY | O_CREAT | O_TRUNC, outMode);
    if (outFd == -1) {
        int err = errno;
        fclose(inFile);
        throw std::runtime_error(tfm::format("Error opening %s: %s", outPath, strerror(err)));
    }
    char buf[4096];
    while (auto readBytes = fread(buf, 1, sizeof(buf), inFile)) {
        auto writtenBytes = write(outFd, buf, readBytes);
        if (writtenBytes == -1) {
            int err = errno;
            close(outFd);
            fclose(inFile);
            throw std::runtime_error(tfm::format("Error writing to %s: %s", outPath, strerror(err)));
        }
        if (writtenBytes != static_cast<int>(readBytes)) {
            close(outFd);
            fclose(inFile);
            throw std::runtime_error(tfm::format("Unknown error while writing to %s", outPath));
        }
    }
    if (!feof(inFile)) {
        close(outFd);
        fclose(inFile);
        throw std::runtime_error(tfm::format("Unknown error while reading %s", inPath));
    }
    close(outFd);
    fclose(inFile);
}

static void checksum(const char * type, const char * inPath, const char * checksum_valid, bool * valid_out, gchar ** calculated_out)
{
    GError * errP{nullptr};
    gboolean valid;
    LrChecksumType lr_type = lr_checksum_type(type);

    if (lr_type == LR_CHECKSUM_UNKNOWN)
        throw libdnf::Error(tfm::format("Unknown checksum type %s", type));

    auto inFd = open(inPath, O_RDONLY);

    if (inFd == -1)
        throw libdnf::Error(tfm::format("Error opening %s: %s", inPath, strerror(errno)));

    auto ret = lr_checksum_fd_compare(lr_type,
                      inFd,
                      /**
                       * If checksum_valid references a string, pass it in, else use
                       * an empty string
                       */
                      checksum_valid ? checksum_valid : "",
                      TRUE,
                      &valid,
                      calculated_out,
                      &errP);

    close(inFd);
    if (!ret)
      throw libdnf::Error(tfm::format("Error calculating checksum %s: (%d, %s)", inPath, errP->code, errP->message));
    if (valid_out)
        *valid_out = valid == TRUE; /* gboolean -> bool */
}


bool checksum_check(const char * type, const char * inPath, const char * checksum_valid)
{
    bool valid;
    checksum(type, inPath, checksum_valid, &valid, NULL);
    return valid;
}

std::string checksum_value(const char * type, const char * inPath)
{
    g_autofree gchar *calculated = NULL;
    checksum(type, inPath, NULL, NULL, &calculated);
    std::string out(calculated);
    return out;
}

}

namespace numeric {
int random(const int min, const int max) {
    std::random_device rd;
    std::default_random_engine gen(rd());
    std::uniform_int_distribution<int> dist(min, max);
    return dist(gen);
}
}

}
