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


#ifndef LIBDNF_BASE_CACHE_HPP
#define LIBDNF_BASE_CACHE_HPP

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>


namespace libdnf {


class Cache {
public:
    explicit Cache(std::string cache_dir);
    explicit Cache(std::string cache_dir, std::vector<Cache> references);

    /// Add a file into cache, return cached path
    std::string add(const std::string & object_type, const std::string & checksum, const std::string & path);

    /// Return cached path on cache hit or an empty string on cache miss
    std::string get(const std::string & object_type, const std::string & checksum) const;

private:
    std::string cacheDir;
    std::vector<Cache> references;
};


/*
Cache::Cache(std::string cache_dir)
    : cacheDir{cache_dir}
{
}


Cache::Cache(std::string cache_dir, std::vector<Cache> references)
    : cacheDir{cache_dir}
    , references{references}
{
}


std::string Cache::add(const std::string & object_type, const std::string & checksum, const std::string & path)
{
    std::string p = cacheDir + "/" + object_type;
    std::cout << p << std::endl;
    try {
        std::filesystem::create_directories(p);
    }
    catch (std::filesystem::filesystem_error) {
    }
    p = p + "/" + checksum;
    try {
        std::filesystem::copy(path, p);
    }
    catch (std::filesystem::filesystem_error) {
    }
    // set current date as mtime
    return p;
}
*/

}  // namespace libdnf


/*
TODO(dmach): handle selinux properly; mv could be a problem
TODO(dmach): make sure solv, solvx, rpm, deb cannot be injected from extra metadata from repomd.xml (security?)
TODO(dmach): make sure objectType does not contain relative paths, /, etc. (security)
TODO(dmach): legacy symlinks in cache
TODO(dmach): copyOnHit(false) - make copies of data available in 'references' caches

add("baseurl", url_checksum)
add("mirrorlist", url_checksum); individual baseurl records handled via "baseurl"
add("metalink", url_checksum); contains repomd.xml checksums
 - add("repomd", checksum)
   - add("primary", checksum)
   - add("filelists", checksum)
   - add("others", checksum)
   - add("group", checksum)
   - add("updateinfo", checksum)
   - add("prestodelta", checksum)
   - add("modules", checksum)
   + *_zck
 - add("rpm", nevra+checksum)
 - add("solv", repomd_checksum)
 - add("solvx", repomd_checksum)

 - any time add() is called, update mtime

 - get(type, key)
   - if cache miss, then try to get it from references

 - cleanAll();
 - cleanType(objectType);
 - cleanAllExceptSelected([<type, key>])
 // TODO(dmach): cache management
 // - remove packages immediately after transaction
 // - remove packages after some time period (1d, 1w, etc.)
 - cleanAllOlderThan(seconds, [types])
*/

#endif
