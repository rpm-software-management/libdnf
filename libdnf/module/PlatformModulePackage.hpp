#ifndef LIBDNF_PLATFORMMODULEPACKAGE_HPP
#define LIBDNF_PLATFORMMODULEPACKAGE_HPP

#include <solv/solvable.h>
#include <solv/repo.h>
#include <string>

#include "libdnf/hy-types.h"
#include "libdnf/hy-repo-private.hpp"

class PlatformModulePackage
{
public:
    static Id createSolvable(Pool *pool, HyRepo repo, const std::string &osReleasePath, const std::string &arch);

    PlatformModulePackage(Pool *pool, HyRepo repo, const std::string &osReleasePath, const std::string &arch);
    Id getId() const { return id; }

private:
    static std::string getPlatformStream(const std::string &osReleasePath);

    Id id;
};


#endif //LIBDNF_PLATFORMMODULEPACKAGE_HPP
