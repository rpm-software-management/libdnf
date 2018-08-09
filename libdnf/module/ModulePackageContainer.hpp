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

    explicit ModulePackageContainer(bool allArch, std::string installRoot, const char * arch);
    ~ModulePackageContainer();

    void add(const std::string &fileContent);
    Id addPlatformPackage(const std::string &osReleasePath, const char *  platformModule);
    void createConflictsBetweenStreams();
    ModulePackagePtr getModulePackage(Id id);
    std::vector<ModulePackagePtr> getModulePackages();

    std::vector<ModulePackagePtr> requiresModuleEnablement(const libdnf::PackageSet & packages);
    /**
     * @brief mark module 'name' as part of 'stream'
     */
    void enable(const std::string &name, const std::string &stream);
    void enable(const ModulePackagePtr &module);
    /**
     * @brief unmark module 'name' from any streams
     */
    void disable(const std::string &name, const std::string &stream);
    void disable(const ModulePackagePtr &module);
    /**
     * @brief Reset module state so it's no longer enabled or disabled.
     */
    void reset(const std::string &name);
    void reset(const ModulePackagePtr &module);
    /**
     * @brief add profile to name:stream
     */
    void install(const std::string &name, const std::string &stream, const std::string &profile);
    void install(const ModulePackagePtr &module, const std::string &profile);
    /**
     * @brief remove profile from name:stream
     */
    void uninstall(const std::string &name, const std::string &stream, const std::string &profile);
    void uninstall(const ModulePackagePtr &module, const std::string &profile);
    /**
     * @brief commit module changes to storage
     */
    void save();
    /**
     * @brief discard all module changes and revert to storage state
     */
    void rollback();
    /**
     * @brief Are there any changes to be saved?
     */
    bool isChanged();

    bool isEnabled(const std::string &name, const std::string &stream);
    bool isEnabled(const ModulePackagePtr &module);

    bool isDisabled(const std::string &name);
    bool isDisabled(const ModulePackagePtr &module);

    std::string getReport();

    /**
     * @brief list of name:stream for module streams that are to be enable
     */
    std::map<std::string, std::string> getEnabledStreams();
    /**
     * @brief list of name:stream for module streams that are to be disabled
     */
    std::map<std::string, std::string> getDisabledStreams();
    /**
     * @brief list of name:stream for module streams that are to be reset
     */
    std::map<std::string, std::string> getResetStreams();
    /**
     * @brief list of name:<old_stream:new_stream> for modules whose stream has changed
     */
    std::map<std::string, std::pair<std::string, std::string>> getSwitchedStreams();
    /**
     * @brief list of name:[profiles] for module profiles being added
     */
    std::map<std::string, std::vector<std::string>> getInstalledProfiles();
    /**
     * @brief list of name:[profiles] for module profiles being removed
     */
    std::map<std::string, std::vector<std::string>> getRemovedProfiles();
    /**
    * @brief Query modules according libdnf::Nsvcap. But the search ignores profiles
    *
    */
    std::vector<ModulePackagePtr> query(libdnf::Nsvcap & moduleNevra);
    /**
    * @brief Requiers subject in format <name>, <name>:<stream>, or <name>:<stream>:<version>
    *
    * @param subject p_subject:...
    * @return std::vector< std::shared_ptr< ModulePackage > >
    */
    std::vector<ModulePackagePtr> query(std::string subject);
    std::vector<ModulePackagePtr> query(std::string name, std::string stream,
        std::string version, std::string context, std::string arch);
    void setModuleDefaults(const std::map<std::string, std::string> &defaultStreams);
    void resolveActiveModulePackages();
    bool isModuleActive(Id id);

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};


#endif //LIBDNF_MODULEPACKAGECONTAINER_HPP
