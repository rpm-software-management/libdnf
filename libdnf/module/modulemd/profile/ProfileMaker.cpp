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

#include "ProfileMaker.hpp"
#include "ModuleProfile.hpp"
#include "NullProfile.hpp"

std::shared_ptr<Profile> ProfileMaker::getProfile(const std::string &profileName, std::shared_ptr<ModulemdModule> modulemd)
{
    GHashTable *profiles = modulemd_module_peek_profiles(modulemd.get());

    GHashTableIter iterator;
    gpointer key, value;

    g_hash_table_iter_init(&iterator, profiles);
    while (g_hash_table_iter_next(&iterator, &key, &value)) {
        std::string keyStr = static_cast<char *>(key);
        if (profileName == keyStr) {
            auto *profile = (ModulemdProfile *) value;
            return std::make_shared<ModuleProfile>(profile);
        }
    }

    return std::make_shared<NullProfile>();
}
