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


#ifndef LIBDNF_UTILS_STRING_HPP
#define LIBDNF_UTILS_STRING_HPP

#include <string>
#include <vector>


namespace libdnf::utils::string {

/// @replaces libdnf:utils.hpp:function:fromCstring(const char * cstring);
std::string from_cstring(const char * cstring);

// SPLIT

/// @replaces libdnf:utils.hpp:function:split(const std::string & source, const char * delimiter, int maxSplit)
std::vector<std::string> split(const std::string & str, const char * separator, int maxsplit);

/// @replaces libdnf:utils.hpp:function:rsplit(const std::string & source, const char * delimiter, int maxSplit)
std::vector<std::string> rsplit(const std::string & str, const char * separator, int maxsplit);

// STARTS/ENDS WITH

/// @replaces libdnf:utils.hpp:function:startsWith(const std::string & source, const std::string & toMatch)
bool startswith(const std::string & str, const std::string & prefix);

/// @replaces libdnf:utils.hpp:function:endsWith(const std::string & source, const std::string & toMatch)
bool endswith(const std::string & str, const std::string & suffix);

// STRIP

/// @replaces libdnf:utils.hpp:function:trim(const std::string & source)
std::string strip(const std::string & str);
std::string strip(const std::string & str, const std::string & chars);

std::string lstrip(const std::string & str);
std::string lstrip(const std::string & str, const std::string & chars);

std::string rstrip(const std::string & str);
std::string rstrip(const std::string & str, const std::string & chars);

// PREFIX/SUFFIX REMOVAL
std::string remove_suffix(const std::string & str, const std::string & suffix);
std::string remove_prefix(const std::string & str, const std::string & prefix);

}  // namespace libdnf::utils::string

#endif
