#ifndef LIBDNF_PROFILEMAKER_HPP
#define LIBDNF_PROFILEMAKER_HPP

#include <string>
#include <memory>

#include "Profile.hpp"

#include <modulemd/modulemd.h>

class ProfileMaker
{
public:
    static std::shared_ptr<Profile> getProfile(const std::string &profileName, std::shared_ptr<ModulemdModule> modulemd);
};


#endif //LIBDNF_PROFILEMAKER_HPP
