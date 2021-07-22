#include <string>
#include <openssl/sha.h>
#include <openssl/evp.h>


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
    static constexpr int digestLength = SHA_DIGEST_LENGTH;

private:
    EVP_MD_CTX *md_ctx;
};
