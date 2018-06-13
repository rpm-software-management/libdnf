#ifndef LIBDNF_FILE_HPP
#define LIBDNF_FILE_HPP

#include <fstream>
#include <memory>
#include <string>

namespace libdnf {

class File
{
public:
    struct IOException : std::runtime_error
    {
        explicit IOException(const std::string &what) : std::runtime_error(what) {}
    };

    struct OpenException : IOException
    {
        explicit OpenException(const std::string &filePath) : IOException("Cannot open file: " + filePath) {}
    };

    struct CloseException : IOException
    {
        explicit CloseException(const std::string &filePath) : IOException("Cannot close file: " + filePath) {}
    };

    struct NotOpenedException : IOException
    {
        explicit NotOpenedException(const std::string &filePath) : IOException("File '" + filePath + "' is not opened.") {}
    };

    struct ShortReadException : IOException
    {
        explicit ShortReadException(const std::string &filePath) : IOException("Short read of file: " + filePath) {}
    };

    static std::shared_ptr<File> newFile(const std::string &filePath);

    explicit File(const std::string &filePath);
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
