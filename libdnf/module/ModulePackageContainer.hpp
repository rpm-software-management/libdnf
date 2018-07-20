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

#ifndef LIBDNF_MODULEPACKAGECONTAINER_HPP
#define LIBDNF_MODULEPACKAGECONTAINER_HPP

#include <map>
#include <memory>
#include <string>
#include <vector>
#include <set>

#include "ModulePackage.hpp"

class ModulePackageContainer
{
public:
    struct Exception : public std::runtime_error
    {
        explicit Exception(const std::string &what) : runtime_error(what) {}
    };

    struct NoModuleException : public Exception
    {
        explicit NoModuleException(const std::string &moduleName) : Exception("No such module: " + moduleName) {}
    };

    struct NoStreamException : public Exception
    {
        explicit NoStreamException(const std::string &moduleStream) : Exception("No such stream: " + moduleStream) {}
    };

    struct EnabledStreamException : public Exception
    {
        explicit EnabledStreamException(const std::string &moduleName) : Exception("No enabled stream for module: " + moduleName) {}
    };

    explicit ModulePackageContainer(const std::shared_ptr<Pool> &pool, const char * arch);
    ~ModulePackageContainer() = default;

    void add(const std::shared_ptr<ModulePackage> &package);
    void add(const std::vector<std::shared_ptr<ModulePackage>> &packages);
    void add(const std::map<Id, std::shared_ptr<ModulePackage>> &packages);
    std::shared_ptr<ModulePackage> getModulePackage(Id id);
    std::vector<std::shared_ptr<ModulePackage>> getModulePackages();

    void enable(const std::string &name, const std::string &stream);

    std::vector<std::shared_ptr<ModulePackage>> getActiveModulePackages(const std::map<std::string, std::string> &defaultStreams);


    std::shared_ptr<Pool> getPool() { return pool; };

private:
    std::vector<std::shared_ptr<ModulePackage>> getActiveModulePackages(const std::vector<std::shared_ptr<ModulePackage>> &modulePackages);

    std::map<Id, std::shared_ptr<ModulePackage>> modules;
    std::shared_ptr<Pool> pool;
};


#endif //LIBDNF_MODULEPACKAGECONTAINER_HPP
