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
#include "libdnf/nsvcap.hpp"

//class ModulePackageContainer;
//typedef std::shared_ptr<ModulePackageContainer> ModulePackageContainerPtr;

struct ModulePackageContainer
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

    explicit ModulePackageContainer(bool allArch, const char * arch);
    ~ModulePackageContainer();

    void add(const std::string &fileContent);
    Id addPlatformPackage(const std::string &osReleasePath, const std::string install_root,
        const char *  platformModule);
    void createConflictsBetweenStreams();
    std::shared_ptr<ModulePackage> getModulePackage(Id id);
    std::vector<std::shared_ptr<ModulePackage>> getModulePackages();

    std::vector<std::shared_ptr<ModulePackage>> requiresModuleEnablement(const libdnf::PackageSet & packages);
    void enable(const std::string &name, const std::string &stream);
    /**
    * @brief Query modules according libdnf::Nsvcap. But the search ignores profiles
    *
    */
    std::vector<std::shared_ptr<ModulePackage>> query(libdnf::Nsvcap & moduleNevra);
    /**
    * @brief Requiers subject in format <name>, <name>:<stream>, or <name>:<stream>:<version>
    *
    * @param subject p_subject:...
    * @return std::vector< std::shared_ptr< ModulePackage > >
    */
    std::vector<std::shared_ptr<ModulePackage>> query(std::string subject);
    void resolveActiveModulePackages(const std::map<std::string, std::string> &defaultStreams);
    bool isModuleActive(Id id);

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};


#endif //LIBDNF_MODULEPACKAGECONTAINER_HPP
