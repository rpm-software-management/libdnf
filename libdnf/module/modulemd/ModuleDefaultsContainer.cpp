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

#include <iostream>

#include "ModuleDefaultsContainer.hpp"

ModuleDefaultsContainer::ModuleDefaultsContainer()
{
    prioritizer = std::shared_ptr<ModulemdPrioritizer>(modulemd_prioritizer_new(), g_object_unref);
}

void ModuleDefaultsContainer::fromString(const std::string &content, int priority)
{
    GError *error = nullptr;
    g_autoptr(GPtrArray) failures;
    g_autoptr(GPtrArray) data = modulemd_objects_from_string_ext(content.c_str(), &failures, &error);

    saveDefaults(data, priority);
    reportFailures(failures);
}

std::string ModuleDefaultsContainer::getDefaultStreamFor(std::string moduleName)
{
    auto moduleDefaults = defaults[moduleName];
    if (!moduleDefaults)
        return "";
        // TODO: get the exception back
        // throw ModulePackageContainer::NoStreamException("Missing default for " + moduleName);
    return modulemd_defaults_peek_default_stream(moduleDefaults.get());
}

void ModuleDefaultsContainer::saveDefaults(GPtrArray *data, int priority)
{
    if (data == nullptr) {
        return;
    }

    GError *error = nullptr;
    modulemd_prioritizer_add(prioritizer.get(), data, priority, &error);
    checkAndThrowException<ConflictException>(error);
}

void ModuleDefaultsContainer::resolve()
{
    GError *error = nullptr;
    g_autoptr(GPtrArray) data = modulemd_prioritizer_resolve(prioritizer.get(), &error);
    checkAndThrowException<ResolveException>(error);

    for (unsigned int i = 0; i < data->len; i++) {
        auto item = g_ptr_array_index(data, i);
        if (!MODULEMD_IS_DEFAULTS(item))
            continue;

        g_object_ref(item);
        auto moduleDefaults = std::shared_ptr<ModulemdDefaults>((ModulemdDefaults *) item, g_object_unref);
        std::string name = modulemd_defaults_peek_module_name(moduleDefaults.get());
        defaults.insert(std::make_pair(name, moduleDefaults));
    }
}

template<typename T>
void ModuleDefaultsContainer::checkAndThrowException(GError *error)
{
    if (error != nullptr) {
        std::string message = error->message;
        g_error_free(error);
        throw T(message);
    }
}

void ModuleDefaultsContainer::reportFailures(const GPtrArray *failures) const
{
    for (unsigned int i = 0; i < failures->len; i++) {
        auto item = static_cast<ModulemdSubdocument *>(g_ptr_array_index(failures, i));
        std::cerr << "Module defaults error: " << modulemd_subdocument_get_gerror(item)->message << "\n";
    }
}

std::map<std::string, std::string> ModuleDefaultsContainer::getDefaultStreams()
{
    // TODO: make sure resolve() was called first

    // return {name: stream} map
    std::map<std::string, std::string> result;
    for (auto const & iter : defaults) {
        auto name = iter.first;
        auto moduleDefaults = iter.second;
        auto defaultStream = modulemd_defaults_peek_default_stream(moduleDefaults.get());
        if (!defaultStream) {
            // if default stream is not set, then the default is disabled -> skip
            continue;
        }
        result[name] = defaultStream;
    }
    return result;
}
