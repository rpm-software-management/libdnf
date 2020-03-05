/*
Copyright (C) 2020 Red Hat, Inc.

This file is part of libdnf: https://github.com/rpm-software-management/libdnf/

Libdnf is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

Libdnf is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with libdnf.  If not, see <https://www.gnu.org/licenses/>.
*/


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
