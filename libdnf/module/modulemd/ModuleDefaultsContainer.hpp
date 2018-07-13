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

#ifndef LIBDNF_MODULEDEFAULTSCONTAINER_HPP
#define LIBDNF_MODULEDEFAULTSCONTAINER_HPP

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <modulemd/modulemd.h>

class ModuleDefaultsContainer
{
public:
    struct Exception : public std::runtime_error
    {
        explicit Exception(const std::string &what) : std::runtime_error(what) {}
    };

    struct ConflictException : public Exception
    {
        explicit ConflictException(const std::string &what) : Exception(what) {}
    };

    struct ResolveException : public Exception
    {
        explicit ResolveException(const std::string &what) : Exception(what) {}
    };

    ModuleDefaultsContainer();
    ~ModuleDefaultsContainer() = default;

    void fromString(const std::string &content, int priority);

    std::string getDefaultStreamFor(std::string moduleName);
    std::map<std::string, std::string> getDefaultStreams();

    void resolve();

private:
    void saveDefaults(GPtrArray *data, int priority);

    template<typename T>
    void checkAndThrowException(GError *error);

    std::shared_ptr<ModulemdPrioritizer> prioritizer;
    std::map<std::string, std::shared_ptr<ModulemdDefaults>> defaults;
    void reportFailures(const GPtrArray *failures) const;
};


#endif //LIBDNF_MODULEDEFAULTSCONTAINER_HPP
