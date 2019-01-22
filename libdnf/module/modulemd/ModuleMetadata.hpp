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

#ifndef LIBDNF_MODULEMETADATA_HPP
#define LIBDNF_MODULEMETADATA_HPP

#include <string>
#include <map>
#include <vector>

#include <modulemd/modulemd.h>

#include "ModuleDependencies.hpp"
#include "ModuleProfile.hpp"

namespace std {

template<>
struct default_delete<ModulemdModule> {
    void operator()(ModulemdModule * ptr) noexcept { g_object_unref(ptr); }
};

}

namespace libdnf {

class ModuleMetadata
{
public:
    static std::vector<ModuleMetadata> metadataFromString(const std::string &fileContent);

public:
    explicit ModuleMetadata(std::unique_ptr<ModulemdModule> && modulemd);
    ModuleMetadata(ModuleMetadata && src) = default;
    ~ModuleMetadata();
    const char * getName() const;
    const char * getStream() const;
    long long getVersion() const;
    const char * getContext() const;
    const char * getArchitecture() const;
    std::string getDescription() const;
    std::string getSummary() const;
    std::vector<ModuleDependencies> getDependencies() const;
    std::vector<std::string> getArtifacts() const;
    std::vector<ModuleProfile> getProfiles(const std::string & profileName = "") const;
    std::string getYaml() const;

private:
    static std::vector<ModuleMetadata> wrapModulemdModule(GPtrArray *data);

    std::unique_ptr<ModulemdModule> modulemd;
    static void reportFailures(const GPtrArray *failures);
};

}

#endif //LIBDNF_MODULEMETADATA_HPP
