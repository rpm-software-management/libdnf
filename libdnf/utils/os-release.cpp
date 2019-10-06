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
#include <array>
#include <map>
#include <rpm/rpmlib.h>
#include <sstream>
#include <string>
#include <vector>

namespace libdnf {

// sorted by precedence (see os-release(5) for details)
std::array<const std::string, 2> paths = {"/etc/os-release", "/usr/lib/os-release"};
// whitelists used for sanity-checking the os-release data when constructing a
// User-Agent string (to avoid reporting rare systems or platforms that could
// be tracked)
std::map<std::string, std::vector<std::string>> distros = {
    // taken from the {fedora,generic}-release.spec files
    { "Fedora", { "cinnamon", "cloud", "container", "coreos", "generic", "iot",
                  "kde", "matecompiz", "server", "silverblue", "snappy", "soas",
                  "workstation", "xfce" } },
};
std::array<const std::string, 1> canons = { "Linux" };

std::map<std::string, std::string> getOsReleaseData()
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

static std::string getCanonOs()
{
    const char *value;
    initLibRpm();
    rpmGetOsInfo(&value, NULL);
    return value;
}

std::string getUserAgent(const std::map<std::string, std::string> & osReleaseData)
{
    std::ostringstream oss;

    // start with the basic libdnf version string
    oss << USER_AGENT;

    // mandatory OS data (bail out if missing or unknown)
    if (!osReleaseData.count("NAME") || !osReleaseData.count("VERSION_ID"))
        return oss.str();
    std::string name = osReleaseData.at("NAME");
    std::string version = osReleaseData.at("VERSION_ID");
    if (!distros.count(name))
        return oss.str();

    // mandatory platform data from RPM (bail out if missing or unknown)
    std::string canon = getCanonOs();
    std::string arch = getBaseArch();
    if (canon.empty() || arch.empty()
        || std::find(canons.begin(), canons.end(), canon) == canons.end())
        return oss.str();

    // optional OS data (use fallback values if missing or unknown)
    std::string variant = "generic";
    auto list = distros[name];
    if (osReleaseData.count("VARIANT_ID")) {
        std::string value = osReleaseData.at("VARIANT_ID");
        if (std::find(list.begin(), list.end(), value) != list.end())
            variant = value;
    }

    // good to go!
    oss << " (" << name << " " << version << "; " << variant << "; "
        << canon << "." << arch << ")";

    return oss.str();
}

std::string getUserAgent()
{
    std::map<std::string, std::string> osdata;
    try {
        osdata = getOsReleaseData();
    } catch (...) {}
    return getUserAgent(osdata);
}

}
