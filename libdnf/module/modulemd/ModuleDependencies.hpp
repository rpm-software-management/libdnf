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

#ifndef LIBDNF_MODULEDEPENDENCIES_HPP
#define LIBDNF_MODULEDEPENDENCIES_HPP


#include <map>
#include <memory>
#include <string>
#include <vector>

#include <modulemd/modulemd.h>

class ModuleDependencies
{
public:
    explicit ModuleDependencies(ModulemdDependencies *dependencies);

    std::vector<std::map<std::string, std::vector<std::string> > > getRequires();

private:
    std::map<std::string, std::vector<std::string>> wrapModuleDependencies(const char *moduleName, ModulemdSimpleSet *streams) const;
    std::vector<std::map<std::string, std::vector<std::string> > > getRequirements(GHashTable *requirements) const;

    ModulemdDependencies *dependencies;
};


#endif //LIBDNF_MODULEDEPENDENCIES_HPP
