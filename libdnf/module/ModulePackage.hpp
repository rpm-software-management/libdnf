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

#ifndef LIBDNF_MODULEPACKAGE_HPP
#define LIBDNF_MODULEPACKAGE_HPP

#include <memory>
#include <string>

#include "libdnf/dnf-types.h"
#include "modulemd/ModuleMetadata.hpp"
#include "libdnf/repo/solvable/Package.hpp"
#include "../goal/IdQueue.hpp"

namespace libdnf {

class ModulePackage // TODO inherit in future; : public Package
{
public:
    ~ModulePackage();

    /**
    * @brief Create module provides based on modulemd metadata.
    */
    const char * getNameCStr() const;
    std::string getName() const;
    const char * getStreamCStr() const;
    std::string getStream() const;
    std::string getNameStream() const;
    std::string getNameStreamVersion() const;
    const std::string & getRepoID() const;
    std::string getVersion() const;
    long long getVersionNum() const;
    const char * getContextCStr() const;
    std::string getContext() const;
    const char * getArchCStr() const;
    std::string getArch() const;
    std::string getFullIdentifier() const;

    std::string getSummary() const;
    std::string getDescription() const;

    std::vector<std::string> getArtifacts() const;

    /**
    * @brief Return profiles matched by name.
    *
    * @return std::vector<ModuleProfile>
    */
    std::vector<ModuleProfile> getProfiles(const std::string &name) const;
    std::vector<ModuleProfile> getProfiles() const;

    std::vector<ModuleDependencies> getModuleDependencies() const;

    void addStreamConflict(const ModulePackage * package);

    Id getId() const { return id; };
    std::string getYaml() const { return metadata.getYaml(); };

private:
    friend struct ModulePackageContainer;

    ModulePackage(DnfSack * moduleSack, LibsolvRepo * repo,
        ModuleMetadata && metadata, const std::string & repoID);

    static Id createPlatformSolvable(DnfSack * moduleSack, const std::string &osReleasePath,
        const std::string install_root, const char *  platformModule);
    static Id createPlatformSolvable(DnfSack * sack, DnfSack * moduleSack,
        const std::vector<std::string> & osReleasePaths, const std::string install_root,
        const char *  platformModule);
    void createDependencies(Solvable *solvable) const;

    ModuleMetadata metadata;

    // TODO: remove after inheriting from Package
    DnfSack * moduleSack;
    std::string repoID;
    Id id;
};

}

#endif //LIBDNF_MODULEPACKAGE_HPP
