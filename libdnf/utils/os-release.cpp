/*
 * Copyright (C) 2019 Red Hat, Inc.
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

#include "os-release.hpp"
#include "File.hpp"

#include <iostream>

std::map<std::string, std::string>
getOsReleaseData(const std::string & path)
{
    std::map<std::string, std::string> result;

    auto file = libdnf::File::newFile(path);
    file->open("r");
    std::string line;
    while (file->readLine(line)) {
        // remove trailing spaces and newline
        line.erase(line.find_last_not_of(" \n") + 1);

        // skip empty lines
        if (line.empty()) {
            continue;
        }

        // skip comments
        auto commentCharPosition = line.find('#');
        if (commentCharPosition == 0) {
            continue;
        }

        // split string by '=' into key and value
        auto equalCharPosition = line.find('=');
        if (equalCharPosition == std::string::npos) {
            throw std::runtime_error("Invalid format (missing '=') " + line);
        }
        auto key = line.substr(0, equalCharPosition);
        auto value = line.substr(equalCharPosition + 1, line.length());

        // remove quotes if present
        if (value.at(0) == '"' && value.back() == '"') {
            value = value.substr(1, value.length() - 2);
        }

        result.insert({key, value});
    }
    return result;
}
