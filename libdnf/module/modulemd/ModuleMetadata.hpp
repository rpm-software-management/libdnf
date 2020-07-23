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

#include <modulemd-2.0/modulemd.h>

#include "../ModulePackage.hpp"

namespace libdnf {

class ModuleMetadata
{
public:
    ModuleMetadata();
    ModuleMetadata(const ModuleMetadata & m);
    ModuleMetadata & operator=(const ModuleMetadata & m);
    ~ModuleMetadata();
    void addMetadataFromString(const std::string & yaml, int priority);
    void resolveAddedMetadata();
    std::vector<ModulePackage *> getAllModulePackages(DnfSack * moduleSack, LibsolvRepo * repo, const std::string & repoID);
    std::map<std::string, std::string> getDefaultStreams();
    std::vector<std::string> getDefaultProfiles(std::string moduleName, std::string moduleStream);
    ModulemdObsoletes * getNewestActiveObsolete(ModulePackage *p);

private:
    static void reportFailures(const GPtrArray *failures);
    ModulemdModuleIndex * resultingModuleIndex;
    ModulemdModuleIndexMerger * moduleMerger;
};

}

#endif //LIBDNF_MODULEMETADATA_HPP
