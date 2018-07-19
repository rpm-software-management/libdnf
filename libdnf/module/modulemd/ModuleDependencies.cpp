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

#include "libdnf/utils/utils.hpp"
#include "ModuleDependencies.hpp"

ModuleDependencies::ModuleDependencies(ModulemdDependencies *dependencies)
        : dependencies(dependencies)
{}

std::vector <std::map<std::string, std::vector<std::string>>> ModuleDependencies::getRequires()
{
    auto cRequires = modulemd_dependencies_peek_requires(dependencies);
    return getRequirements(cRequires);
}

std::map<std::string, std::vector<std::string>> ModuleDependencies::wrapModuleDependencies(const char *moduleName, ModulemdSimpleSet *streams) const
{
    std::map<std::string, std::vector<std::string> > moduleRequirements;
    auto streamSet = modulemd_simpleset_dup(streams);

    for (auto item = streamSet; *item; ++item) {
        moduleRequirements[moduleName].emplace_back(*item);
        g_free(*item);
    }
    g_free(streamSet);

    return moduleRequirements;
}

std::vector<std::map<std::string, std::vector<std::string>>> ModuleDependencies::getRequirements(GHashTable *requirements) const
{
    std::vector<std::map<std::string, std::vector<std::string>>> requires;
    requires.reserve(g_hash_table_size(requirements));

    GHashTableIter iterator;
    gpointer key, value;

    g_hash_table_iter_init (&iterator, requirements);
    while (g_hash_table_iter_next(&iterator, &key, &value)) {
        auto moduleRequirements = wrapModuleDependencies(static_cast<const char *>(key),
                                                         static_cast<ModulemdSimpleSet *>(value));

        requires.push_back(moduleRequirements);
    }

    return requires;
}
