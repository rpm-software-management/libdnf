/*
 * Copyright (C) 2018 Red Hat, Inc.
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
