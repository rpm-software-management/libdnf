#include <modulemd/modulemd-simpleset.h>

#include "ModuleProfile.hpp"


ModuleProfile::ModuleProfile(ModulemdProfile *profile)
        : profile(profile)
{
}

std::string ModuleProfile::getName() const
{
    return modulemd_profile_peek_name(profile);
}

std::string ModuleProfile::getDescription() const
{
    return modulemd_profile_peek_description(profile);
}

std::vector<std::string> ModuleProfile::getContent() const
{
    ModulemdSimpleSet *profileRpms = modulemd_profile_peek_rpms(profile);
    gchar **cRpms = modulemd_simpleset_dup(profileRpms);

    std::vector<std::string> rpms;
    rpms.reserve(modulemd_simpleset_size(profileRpms));
    for (auto item = cRpms; *item; ++item) {
        rpms.emplace_back(*item);
        g_free(*item);
    }
    g_free(cRpms);

    return rpms;
}
