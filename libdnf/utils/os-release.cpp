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
#include "utils.hpp"
#include "libdnf/dnf-context.h"

#include <algorithm>
#include <map>
#include <rpm/rpmlib.h>
#include <string>
#include <vector>

namespace libdnf {

std::map<std::string, std::string> getOsReleaseData(const std::vector<std::string> & paths)
{
    std::map<std::string, std::string> result;

    // find the first existing file path
    auto it = std::find_if(paths.begin(), paths.end(), libdnf::filesystem::exists);
    if (it == paths.end())
        throw std::runtime_error("os-release file not found");
    std::string path = *it;

    auto file = libdnf::File::newFile(path);
    file->open("r");
    std::string line;
    while (file->readLine(line)) {
        // remove trailing spaces and newline
        line.erase(line.find_last_not_of(" \n") + 1);

        // skip empty lines
        if (line.empty()) continue;
        // skip comments
        if (line.front() == '#') continue;

        // split string by '=' into key and value
        auto pos = line.find('=');
        if (pos == line.npos)
            throw std::runtime_error("Invalid format (missing '='): " + line);
        auto key    = string::trim(line.substr(0, pos));
        auto value  = string::trim(line.substr(pos + 1, line.length()));

        // remove quotes if present
        if (!value.empty() && value.front() == '"' && value.back() == '"') {
            value = value.substr(1, value.length() - 2);
        }

        result.insert({key, value});
    }
    return result;
}

static void initLibRpm()
{
    static bool libRpmInitiated{false};
    if (libRpmInitiated) return;
    if (rpmReadConfigFiles(NULL, NULL) != 0) {
        throw std::runtime_error("failed to read rpm config files\n");
    }
    libRpmInitiated = true;
}

static std::string getBaseArch()
{
    const char *value;
    initLibRpm();
    rpmGetArchInfo(&value, NULL);
    value = find_base_arch(value);
    return value ? std::string(value) : "";
}

}
