#include <cstring>

#include <iomanip>
#include <sstream>

#include "sha1.hpp"


SHA1Hash::SHA1Hash()
{
    SHA1_Init(&ctx);
}


void
SHA1Hash::update(const char * data)
{
    SHA1_Update(&ctx, (unsigned char *)data, strlen(data));
}


std::string
SHA1Hash::hexdigest()
{
    unsigned char md[DIGEST_LENGTH];
    SHA1_Final(md, &ctx);

    std::stringstream ss;
    for(int i=0; i<DIGEST_LENGTH; i++) {
        ss << std::setfill('0') << std::setw(2) << std::hex << static_cast<int>(md[i]);
    }

    return ss.str();
}
