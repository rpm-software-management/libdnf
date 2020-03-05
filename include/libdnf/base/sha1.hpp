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


#ifndef LIBDNF_BASE_SHA1_HPP
#define LIBDNF_BASE_SHA1_HPP

#include <openssl/sha.h>

#include <string>


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
