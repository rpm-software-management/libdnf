#pragma once


#include <string>
#include <cstdint>


class Object {
public:
    ~Object() { if (cstring != NULL) { free(cstring); } }

    std::string string;
    char * cstring = NULL;
    bool boolean = true;
    int32_t int32 = 32;
    int64_t int64 = 64;
    uint64_t uint64 = 64;
};
