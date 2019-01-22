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

#include <fnmatch.h>
#include <iostream>
#include <utility>

#include "ModuleMetadata.hpp"
#include "../hy-util-private.hpp"

namespace libdnf {

std::vector<ModuleMetadata> ModuleMetadata::metadataFromString(const std::string &fileContent)
{
    GError *error = nullptr;
    g_autoptr(GPtrArray) failures;
    g_autoptr(GPtrArray) data = modulemd_objects_from_string_ext(fileContent.c_str(), &failures, &error);

    reportFailures(failures);
    return wrapModulemdModule(data);
}

std::vector<ModuleMetadata> ModuleMetadata::wrapModulemdModule(GPtrArray *data)
{
    if (data == nullptr)
        return {};

    std::vector<ModuleMetadata> moduleCluster;
    for (unsigned int i = 0; i < data->len; i++) {
        auto module = g_ptr_array_index(data, i);
        if (!MODULEMD_IS_MODULE(module))
            continue;

        g_object_ref(module);
        auto modulemd = std::unique_ptr<ModulemdModule>(static_cast<ModulemdModule *>(module));
        moduleCluster.emplace_back(ModuleMetadata(std::move(modulemd)));
    }

    return moduleCluster;
}

void ModuleMetadata::reportFailures(const GPtrArray *failures)
{
    for (unsigned int i = 0; i < failures->len; i++) {
        auto item = static_cast<ModulemdSubdocument *>(g_ptr_array_index(failures, i));
        std::cerr << "Module yaml error: " << modulemd_subdocument_get_gerror(item)->message << "\n";
    }
}

ModuleMetadata::ModuleMetadata(std::unique_ptr<ModulemdModule> && modulemd)
: modulemd(std::move(modulemd)) {}

ModuleMetadata::~ModuleMetadata() = default;

const char * ModuleMetadata::getName() const
{
    return modulemd_module_peek_name(modulemd.get());
}

const char * ModuleMetadata::getStream() const
{
    return modulemd_module_peek_stream(modulemd.get());
}

long long ModuleMetadata::getVersion() const
{
    return (long long) modulemd_module_peek_version(modulemd.get());
}

const char * ModuleMetadata::getContext() const
{
    return modulemd_module_peek_context(modulemd.get());
}

const char * ModuleMetadata::getArchitecture() const
{
    return modulemd_module_peek_arch(modulemd.get());
}

std::string ModuleMetadata::getDescription() const
{
    const char *description = modulemd_module_peek_description(modulemd.get());
    return description ? description : "";
}

std::string ModuleMetadata::getSummary() const
{
    const char *summary = modulemd_module_peek_summary(modulemd.get());
    return summary ? summary : "";
}

std::vector<ModuleDependencies> ModuleMetadata::getDependencies() const
{
    auto cDependencies = modulemd_module_peek_dependencies(modulemd.get());
    std::vector<ModuleDependencies> dependencies;

    for (unsigned int i = 0; i < cDependencies->len; i++) {
        dependencies.emplace_back(static_cast<ModulemdDependencies *>(g_ptr_array_index(cDependencies, i)));
    }

    return dependencies;
}

std::vector<std::string> ModuleMetadata::getArtifacts() const
{
    ModulemdSimpleSet *cArtifacts = modulemd_module_peek_rpm_artifacts(modulemd.get());
    gchar **rpms = modulemd_simpleset_dup(cArtifacts);

    std::vector<std::string> artifacts;
    for (auto item = rpms; *item; ++item) {
        artifacts.emplace_back(*item);
        g_free(*item);
    }
    g_free(rpms);

    return artifacts;
}

std::vector<ModuleProfile>
ModuleMetadata::getProfiles(const std::string & profileName) const
{
    GHashTable *cRequires = modulemd_module_peek_profiles(modulemd.get());
    std::vector<ModuleProfile> profiles;
    profiles.reserve(g_hash_table_size(cRequires));

    GHashTableIter iterator;
    gpointer key, value;

    g_hash_table_iter_init (&iterator, cRequires);
    if (profileName.empty()) {
        while (g_hash_table_iter_next(&iterator, &key, &value)) {
            auto profile = (ModulemdProfile *) value;
            profiles.push_back(ModuleProfile(profile));
        }
    } else {
        auto profileNameCStr = profileName.c_str();
        gboolean glob = hy_is_glob_pattern(profileNameCStr);
        while (g_hash_table_iter_next(&iterator, &key, &value)) {
            std::string keyStr = static_cast<char *>(key);
            if (glob && fnmatch(profileNameCStr, static_cast<char *>(key), 0) == 0) {
                auto profile = (ModulemdProfile *) value;
                profiles.push_back(ModuleProfile(profile));
            } else if (strcmp(profileNameCStr, static_cast<char *>(key)) == 0) {
                auto profile = (ModulemdProfile *) value;
                profiles.push_back(ModuleProfile(profile));
            }
        }
    }
    return profiles;
}

std::string
ModuleMetadata::getYaml() const
{
    auto yamlCString = modulemd_module_dumps(modulemd.get());
    std::string yaml = yamlCString ? yamlCString : "";
    g_free(yamlCString);
    return yaml;
}

}
