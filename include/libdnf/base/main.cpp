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


#include "cache.hpp"
//include "sha1.hpp"
#include "sha1.cpp"

int main() {
    libdnf::Cache cache("cache");

    std::string url = "https://mirrors.fedoraproject.org/metalink?repo=updates-released-f31&arch=x86_64";
    SHA1Hash h;
    h.update(url.c_str());
    cache.add("metalink", h.hexdigest(), "metalink.xml");

    std::string repomd_sha1 = "35956520fefbc8618d05ee7646c99e83d40430b2";
    std::string repomd_path = "dl.fedoraproject.org/pub/fedora/linux/updates/31/Everything/x86_64/repodata/repomd.xml";
    cache.add("repomd", repomd_sha1, repomd_path);

    std::string solv_path = "/var/cache/dnf/updates.solv";
    cache.add("solv", repomd_sha1, solv_path);

    std::string solv_filenames_path = "/var/cache/dnf/updates-filenames.solvx";
    cache.add("filenames.solvx", repomd_sha1, solv_filenames_path);

    return 0;
}
