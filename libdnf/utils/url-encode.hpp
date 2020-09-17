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

#ifndef LIBDNF_UTILS_URLENCODE_HPP
#define LIBDNF_UTILS_URLENCODE_HPP

#include <string>


namespace libdnf {

/**
* @brief Converts the given input string to a URL encoded string
*
* All input characters that are not a-z, A-Z, 0-9, '-', '.', '_' or '~' are converted
* to their "URL escaped" version (%NN where NN is a two-digit hexadecimal number).
*
* @param src String to encode
* @param exclude A list of characters that won't be encoded
* @return URL encoded string
*/
std::string urlEncode(const std::string & src, const std::string & exclude = "");

/**
* @brief URL-decodes the input string
*
* All percent signs followed by two hex digits are decoded to the characters
* they represent.
*
* @param src The string to decode
* @return URL-decoded string
*/
std::string urlDecode(const std::string & src);

} // namespace libdnf

#endif
