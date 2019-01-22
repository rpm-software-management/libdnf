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
#include <string.h>

#include <modulemd/modulemd.h>

#include "ModuleDefaultsContainer.hpp"

namespace libdnf {

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

std::vector<std::string> ModuleDefaultsContainer::getDefaultProfiles(std::string & moduleName,
    std::string & moduleStream)
{
    auto moduleDefaults = defaults.find(moduleName);
    if (moduleDefaults == defaults.end())
        return {};
    auto defaultsPeek = modulemd_defaults_peek_profile_defaults(moduleDefaults->second.get());
    GHashTableIter iterator;
    gpointer key, value;
    auto moduleStreamCStr = moduleStream.c_str();
    std::vector<std::string> output;
    g_hash_table_iter_init(&iterator, defaultsPeek);
    while (g_hash_table_iter_next(&iterator, &key, &value)) {
        if (strcmp(moduleStreamCStr, static_cast<char *>(key)) == 0) {
            auto cValue = static_cast<ModulemdSimpleSet *>(value);
            auto profileSet = modulemd_simpleset_dup(cValue);
            for (auto item = profileSet; *item; ++item) {
                output.emplace_back(*item);
                g_free(*item);
            }
            g_free(profileSet);
            return output;
        }
    }
    return {};
}

std::string ModuleDefaultsContainer::getDefaultStreamFor(std::string moduleName)
{
    auto moduleDefaults = defaults.find(moduleName);
    if (moduleDefaults == defaults.end())
        return "";
        // TODO: get the exception back
        // throw ModulePackageContainer::NoStreamException("Missing default for " + moduleName);
    return modulemd_defaults_peek_default_stream(moduleDefaults->second.get());
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
        auto moduleDefaults = std::unique_ptr<ModulemdDefaults>(
            (ModulemdDefaults *) item);
        std::string name = modulemd_defaults_peek_module_name(moduleDefaults.get());
        defaults.insert(std::make_pair(name, std::move(moduleDefaults)));
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
        auto moduleDefaults = iter.second.get();
        auto defaultStream = modulemd_defaults_peek_default_stream(moduleDefaults);
        if (!defaultStream) {
            // if default stream is not set, then the default is disabled -> skip
            continue;
        }
        result[name] = defaultStream;
    }
    return result;
}

}
