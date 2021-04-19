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

#include "ModuleProfile.hpp"

namespace libdnf {

ModuleProfile::ModuleProfile(ModulemdProfile *profile)
        : profile(profile)
{
    g_object_ref(profile);
}

ModuleProfile::ModuleProfile(const ModuleProfile & p)
        : profile(p.profile)
{
    if (profile != nullptr) {
        g_object_ref(profile);
    }
}

ModuleProfile & ModuleProfile::operator=(const ModuleProfile & p)
{
    if (this != &p) {
        g_object_unref(profile);
        profile = p.profile;
        if (profile != nullptr) {
            g_object_ref(profile);
        }
    }
    return *this;
}

ModuleProfile::~ModuleProfile()
{
    if (profile != nullptr) {
        g_object_unref(profile);
    }
}

std::string ModuleProfile::getName() const
{
    if (!profile) {
        return {};
    }
    auto name = modulemd_profile_get_name(profile);
    return name ? name : "";
}

bool ModuleProfile::isDefault() const
{
    if (!profile) {
        return {};
    }
    return modulemd_profile_is_default(profile);
}

std::string ModuleProfile::getDescription() const
{
    if (!profile) {
        return {};
    }
    auto description =  modulemd_profile_get_description(profile, NULL);
    return description ? description : "";
}

std::vector<std::string> ModuleProfile::getContent() const
{
    if (!profile) {
        return {};
    }
    gchar **cRpms = modulemd_profile_get_rpms_as_strv(profile);

    std::vector<std::string> rpms;
    for (gchar **item = cRpms; *item; ++item) {
        rpms.emplace_back(*item);
        g_free(*item);
    }
    g_free(cRpms);

    return rpms;
}

}
