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
#include "modulemd/ModuleProfile.hpp"
#include "modulemd/ModuleDependencies.hpp"
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
    bool operator==(const ModulePackage &r) const;
    /**
    * @brief Return profiles matched by name (which is possibly a globby pattern).
    *
    * @return std::vector<ModuleProfile>
    */
    std::vector<ModuleProfile> getProfiles(const std::string &name) const;
    std::vector<ModuleProfile> getProfiles() const;
    ModuleProfile getDefaultProfile() const;

    std::vector<ModuleDependencies> getModuleDependencies() const;

    void addStreamConflict(const ModulePackage * package);

    Id getId() const { return id; };
    std::string getYaml() const;

    /**
     * @brief Return whether context string is static. It is important for proper behaviour of modular solver
     *
     * @return bool
     */
    bool getStaticContext() const;
    /**
     * @brief Returns strings of modules requires ("nodejs", "nodejs:12", "nodejs:-11")
     *
     * @param removePlatform When true, the method will not return requires with stream "platform" (default false)
     * @return std::vector< std::string >
     */
    std::vector<std::string> getRequires(bool removePlatform=false);

private:
    friend struct ModulePackageContainer;
    friend struct ModuleMetadata;

    ModulePackage(DnfSack * moduleSack, LibsolvRepo * repo,
        ModulemdModuleStream * mdStream, const std::string & repoID, const std::string & context = {});

    ModulePackage(const ModulePackage & mpkg);
    ModulePackage & operator=(const ModulePackage & mpkg);

    static Id createPlatformSolvable(DnfSack * moduleSack, const std::string &osReleasePath,
        const std::string install_root, const char *  platformModule);
    static Id createPlatformSolvable(DnfSack * sack, DnfSack * moduleSack,
        const std::vector<std::string> & osReleasePaths, const std::string install_root,
        const char *  platformModule);
    void createDependencies(Solvable *solvable) const;
    /// return vector with string requires like "nodejs:11", "nodejs", or "nodejs:-11"
    static std::vector<std::string> getRequires(ModulemdModuleStream * mdStream, bool removePlatform);
    static std::string getNameStream(ModulemdModuleStream * mdStream);

    ModulemdModuleStream * mdStream;

    // TODO: remove after inheriting from Package
    DnfSack * moduleSack;
    std::string repoID;
    Id id;
};

inline bool ModulePackage::operator==(const ModulePackage &r) const
{
    return id == r.id && moduleSack == r.moduleSack;
}

inline std::vector<std::string> ModulePackage::getRequires(bool removePlatform)
{
    return getRequires(mdStream, removePlatform);
}

/**
 * @brief Return module $name:$stream.
 *
 * @return std::string
 */
inline std::string ModulePackage::getNameStream() const
{
    return getNameStream(mdStream);
}

}

#endif //LIBDNF_MODULEPACKAGE_HPP
