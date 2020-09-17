/*
 * Copyright (C) 2020 Red Hat, Inc.
 *
 * Licensed under the GNU Lesser General Public License Version 2.1
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "url-encode.hpp"


namespace libdnf {

std::string urlEncode(const std::string & src, const std::string & exclude) {
    auto noEncode = [&exclude](char ch)
    {
        if (isalnum(ch) || ch=='-' || ch == '.' || ch == '_' || ch == '~') {
            return true;
        }

        if (exclude.find(ch) != std::string::npos) {
            return true;
        }

        return false;
    };

    // compute length of encoded string
    auto len = src.length();
    for (auto ch : src) {
        if (!noEncode(ch))
            len += 2;
    }

    // encode the input string
    std::string encoded;
    encoded.reserve(len);
    for (auto ch : src) {
        if (noEncode(ch))
            encoded.push_back(ch);
        else {
            encoded.push_back('%');
            unsigned char hex;
            hex = static_cast<unsigned char>(ch) >> 4;
            hex += hex <= 9 ? '0' : 'a' - 10;
            encoded.push_back(hex);
            hex = static_cast<unsigned char>(ch) & 0x0F;
            hex += hex <= 9 ? '0' : 'a' - 10;
            encoded.push_back(hex);
        }
    }

    return encoded;
}

std::string urlDecode(const std::string & src) {
    std::string out;

    for (size_t i = 0; i < src.length(); ++i) {
        char ch = src[i];

        if (ch == '%') {
            out.push_back(stoi(src.substr(i + 1, 2), nullptr, 16));
            i += 2;
        } else {
            out.push_back(ch);
        }
    }

    return out;
}

} // namespace libdnf
