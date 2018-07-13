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

#include "profile/ModuleProfile.hpp"
#include "ModuleDependencies.hpp"

class ModuleMetadata
{
public:
    static std::vector<std::shared_ptr<ModuleMetadata> > metadataFromString(const std::string &fileContent);

public:
    explicit ModuleMetadata(const std::shared_ptr<ModulemdModule> &modulemd);
    ~ModuleMetadata();

    std::string getName() const;
    std::string getStream() const;
    long long getVersion() const;
    std::string getContext() const;
    const char * getArchitecture() const;
    std::string getDescription() const;
    std::string getSummary() const;
    std::vector<std::shared_ptr<ModuleDependencies> > getDependencies() const;
    std::vector<std::string> getArtifacts() const;
    std::vector<std::shared_ptr<ModuleProfile>> getProfiles() const;
    std::shared_ptr<Profile> getProfile(const std::string &profileName) const;

private:
    static std::vector<std::shared_ptr<ModuleMetadata> > wrapModulemdModule(GPtrArray *data);

    std::shared_ptr<ModulemdModule> modulemd;
    static void reportFailures(const GPtrArray *failures);
};


#endif //LIBDNF_MODULEMETADATA_HPP
