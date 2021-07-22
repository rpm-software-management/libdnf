#include <cstring>

#include <iomanip>
#include <sstream>

#include "sha1.hpp"


SHA1Hash::SHA1Hash()
{
    md_ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(md_ctx, EVP_sha1(), NULL);
}


void
SHA1Hash::update(const char * data)
{
    EVP_DigestUpdate(md_ctx, data, strlen(data));
}


std::string
SHA1Hash::hexdigest()
{
    unsigned char md[digestLength];
    EVP_DigestFinal_ex(md_ctx, md, NULL);

    std::stringstream ss;
    for(int i=0; i<digestLength; i++) {
        ss << std::setfill('0') << std::setw(2) << std::hex << static_cast<int>(md[i]);
    }

    EVP_MD_CTX_free(md_ctx);
    return ss.str();
}
