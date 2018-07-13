#include "PlatformModulePackage.hpp"
#include "libdnf/utils/File.hpp"

#include <algorithm>
#include <sstream>

Id PlatformModulePackage::createSolvable(Pool *pool, HyRepo repo, const std::string &osReleasePath,
                                         const std::string &arch)
{
    Id id = repo_add_solvable(repo->libsolv_repo);
    Solvable *solvable = pool_id2solvable(pool, id);

    std::string name = "platform";
    std::string stream = getPlatformStream(osReleasePath);
    long version = 0;
    std::string context = "00000000";

    std::ostringstream ss;

    // create solvable with:
    //   Name: $name:$stream:$version:$context
    //   Version: 0
    //   Arch: $arch
    ss << name << "-" << stream << "-" << version << "-" << context;
    solvable_set_str(solvable, SOLVABLE_NAME, ss.str().c_str());
    solvable_set_str(solvable, SOLVABLE_EVR, "0");
    solvable_set_str(solvable, SOLVABLE_ARCH, arch.c_str());


    // create Provide: module($name)
    ss.str(std::string());
    ss << "module(" << name << ")";
    auto depId = pool_str2id(pool, ss.str().c_str(), 1);
    solvable_add_deparray(solvable, SOLVABLE_PROVIDES, depId, -1);

    // create Provide: module($name:$stream)
    ss.str(std::string());
    ss << "module(" << name << ":" << stream << ")";
    depId = pool_str2id(pool, ss.str().c_str(), 1);
    solvable_add_deparray(solvable, SOLVABLE_PROVIDES, depId, -1);

    // create Provide: module($name:$stream:$version)
    ss.str(std::string());
    ss << "module(" << name << ":" << stream << ":" << version << ")";
    depId = pool_str2id(pool, ss.str().c_str(), 1);
    solvable_add_deparray(solvable, SOLVABLE_PROVIDES, depId, -1);

    return id;
}

PlatformModulePackage::PlatformModulePackage(Pool *pool, HyRepo repo, const std::string &osReleasePath,
                                             const std::string &arch)
{
    id = createSolvable(pool, repo, osReleasePath, arch);
}

std::string PlatformModulePackage::getPlatformStream(const std::string &osReleasePath)
{
    auto file = libdnf::File::newFile(osReleasePath);
    std::string line;
    while (file->readLine(line)) {
        if (line.find("PLATFORM_ID") != std::string::npos) {
            std::remove_if(std::begin(line), std::end(line), isspace);

            auto equalCharPosition = line.find('=');
            if (equalCharPosition == std::string::npos)
                throw std::runtime_error("Invalid format (missing '=') of PLATFORM_ID in " + osReleasePath);

            auto platform = line.substr(equalCharPosition + 1);
            auto colonCharPosition = platform.find(':');
            if (colonCharPosition == std::string::npos) {
                throw std::runtime_error("Invalid format (missing ':' in value) of PLATFORM_ID in " + osReleasePath);
            }
            return platform.substr(colonCharPosition + 1);
        }
    }

    throw std::runtime_error("Missing PLATFORM_ID in " + osReleasePath);
}
