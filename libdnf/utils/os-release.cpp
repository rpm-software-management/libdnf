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

#include <algorithm>
#include <iostream>
#include <rpm/rpmlib.h>
#include <string>
#include <vector>

// basearch->{arch, ...} mapping, taken from DNF (dnf/rpm/__init__.py)
std::map<std::string, std::vector<std::string>>
archMap = {
    { "aarch64",  { "aarch64" } },
    { "alpha",    { "alpha", "alphaev4", "alphaev45", "alphaev5", "alphaev56", "alphaev6",
                    "alphaev67", "alphaev68", "alphaev7", "alphapca56" } },
    { "arm",      { "armv5tejl", "armv5tel", "armv5tl", "armv6l", "armv7l", "armv8l" } },
    { "armhfp",   { "armv7hl", "armv7hnl", "armv8hl", "armv8hnl", "armv8hcnl" } },
    { "i386",     { "i386", "athlon", "geode", "i386", "i486", "i586", "i686"} },
    { "ia64",     { "ia64" } },
    { "mips",     { "mips" } },
    { "mipsel",   { "mipsel" } },
    { "mips64",   { "mips64" } },
    { "mips64el", { "mips64el" } },
    { "noarch",   { "noarch" } },
    { "ppc",      { "ppc" } },
    { "ppc64",    { "ppc64", "ppc64iseries", "ppc64p7", "ppc64pseries" } },
    { "ppc64le",  { "ppc64le" } },
    { "s390",     { "s390" } },
    { "s390x",    { "s390x" } },
    { "sh3",      { "sh3" } },
    { "sh4",      { "sh4", "sh4a" } },
    { "sparc",    { "sparc", "sparc64", "sparc64v", "sparcv8", "sparcv9", "sparcv9v" } },
    { "x86_64",   { "x86_64", "amd64", "ia32e" } },
};

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

static void initLibRpm()
{
    static bool libRpmInitiated{false};
    if (!libRpmInitiated) {
        if (rpmReadConfigFiles(NULL, NULL) != 0) {
            throw std::runtime_error("failed to read rpm config files\n");
        }
        libRpmInitiated = true;
    }
}

std::string
getBaseArch()
{
    const char *value;

    initLibRpm();
    // hy_detect_arch(&value);  // this is how DNF does it
    rpmGetArchInfo(&value, NULL);  // let's just poke RPM
    std::string arch(value);

    // find the base architecture
    for (auto const& i : archMap) {
        if (std::any_of(i.second.begin(), i.second.end(),
                        [arch](std::string j){return j == arch;})) {
            return i.first;
        }
    }

    // no match found
    return "";
}
