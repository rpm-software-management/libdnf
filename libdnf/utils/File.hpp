#ifndef LIBDNF_FILE_HPP
#define LIBDNF_FILE_HPP

#include "../error.hpp"

#include <fstream>
#include <memory>
#include <string>

namespace libdnf {

class File
{
public:
    struct IOError : Error
    {
        explicit IOError(const std::string &what) : Error(what) {}
    };

    struct OpenError : IOError
    {
        explicit OpenError(const std::string &filePath) : IOError("Cannot open file \"" + filePath + "\".") {}
        explicit OpenError(const std::string &filePath, const std::string &detail) :
            IOError("Cannot open file \"" + filePath + "\": " + detail) {}
    };

    struct CloseError : IOError
    {
        explicit CloseError(const std::string &filePath) : IOError("Cannot close file \"" + filePath + "\".") {}
    };

    struct NotOpenedException : Exception
    {
        explicit NotOpenedException(const std::string &filePath) :
            Exception("File \"" + filePath + "\" is not opened.") {}
    };

    struct ReadError : IOError
    {
        explicit ReadError(const std::string & msg) : IOError(msg) {}
    };

    static std::unique_ptr<File> newFile(const std::string &filePath);

    explicit File(const std::string &filePath);
    File(const File&) = delete;
    File & operator=(const File&) = delete;
    virtual ~File();

    virtual void open(const char *mode);
    void close();

    size_t read(char *buffer, size_t count);
    bool readLine(std::string &line);
    virtual std::string getContent();

protected:
    std::string filePath;
    FILE *file;
};

}

#endif //LIBDNF_FILE_HPP
