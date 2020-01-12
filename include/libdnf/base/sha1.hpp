#ifndef LIBDNF_BASE_SHA1_HPP
#define LIBDNF_BASE_SHA1_HPP

#include <string>
#include <openssl/sha.h>


/*
USAGE:

SHA1Hash h;
h.update("foo");
h.update("bar");
std::cout << h.hexdigest() << std::endl;
*/


class SHA1Hash {
public:
    SHA1Hash();
    void update(const char *data);
    std::string hexdigest();
    static constexpr int DIGEST_LENGTH = SHA_DIGEST_LENGTH;

private:
    SHA_CTX ctx;
};

#endif
