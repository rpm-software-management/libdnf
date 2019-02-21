#ifndef LIBDNF_GZFILE_HPP
#define LIBDNF_GZFILE_HPP

#include "File.hpp"

namespace libdnf {

class CompressedFile : public File
{
public:
    explicit CompressedFile(const std::string &filePath);
    ~CompressedFile() override;

    void open(const char *mode) override;

    std::string getContent() override;
};

}


#endif //LIBDNF_GZFILE_HPP
