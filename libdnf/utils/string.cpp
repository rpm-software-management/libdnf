/*
Copyright (C) 2020-2021 Red Hat, Inc.

This file is part of libdnf: https://github.com/rpm-software-management/libdnf/

Libdnf is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 2.1 of the License, or
(at your option) any later version.

Libdnf is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with libdnf.  If not, see <https://www.gnu.org/licenses/>.
*/


#include "string.hpp"


namespace libdnf::utils::string {


std::vector<std::string> rsplit(const std::string & str, const std::string & delimiter, std::size_t limit) {
    std::vector<std::string> result;

    if (str.empty()) {
        result.push_back("");
        return result;
    }

    // no need to split anything, return immediately
    if (limit <= 1) {
        result.push_back(str);
        return result;
    }

    const std::size_t delimiter_length = delimiter.size();

    // count the number of parts in the input string
    std::size_t parts = 1;
    for (auto pos = str.find(delimiter); pos != std::string::npos; pos = str.find(delimiter, pos + delimiter_length)) {
        ++parts;
    }

    std::size_t previous = 0;
    std::size_t current = str.find(delimiter);

    // move `current` according to the number of leading parts not included in the `limit`
    if (parts > limit) {
        for (std::size_t to_join = parts - limit; to_join != 0; --to_join) {
            current = str.find(delimiter, current + delimiter_length);
        }
    }

    // split the remaining string as usual
    while (current != std::string::npos) {
        result.push_back(str.substr(previous, current - previous));
        previous = current + delimiter_length;
        current = str.find(delimiter, previous);
    }
    result.push_back(str.substr(previous));
    return result;
}


std::vector<std::string> split(const std::string & str, const std::string & delimiter, std::size_t limit) {
    std::vector<std::string> result;

    if (str.empty()) {
        result.push_back("");
        return result;
    }

    // no need to split anything, return immediately
    if (limit <= 1) {
        result.push_back(str);
        return result;
    }

    const std::size_t delimiter_length = delimiter.size();

    std::size_t current = str.find(delimiter);
    std::size_t previous = 0;
    while (current != std::string::npos) {
        // we reached the limit - append the whole remaining string
        if (limit != std::string::npos && result.size() + 1 >= limit) {
            result.push_back(str.substr(previous));
            return result;
        }

        result.push_back(str.substr(previous, current - previous));
        previous = current + delimiter_length;
        current = str.find(delimiter, previous);
    }
    result.push_back(str.substr(previous));
    return result;
}


}  // namespace libdnf::utils::string
