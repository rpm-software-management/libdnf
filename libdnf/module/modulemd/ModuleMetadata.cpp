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

#include "ModuleMetadata.hpp"

#include "../ModulePackageContainer.hpp"

#include "bgettext/bgettext-lib.h"
#include "tinyformat/tinyformat.hpp"
#include "../../log.hpp"

namespace libdnf {

ModuleMetadata::ModuleMetadata(): resultingModuleIndex(NULL), moduleMerger(NULL) {}

ModuleMetadata::ModuleMetadata(const ModuleMetadata & m)
        : resultingModuleIndex(m.resultingModuleIndex), moduleMerger(m.moduleMerger)
{
    if (resultingModuleIndex != nullptr) {
        g_object_ref(resultingModuleIndex);
    }
    if (moduleMerger != nullptr) {
        g_object_ref(moduleMerger);
    }
}

ModuleMetadata & ModuleMetadata::operator=(const ModuleMetadata & m)
{
    if (this != &m) {
        if (resultingModuleIndex != nullptr) {
            g_object_unref(resultingModuleIndex);
        }
        if (moduleMerger != nullptr) {
            g_object_unref(moduleMerger);
        }
        resultingModuleIndex = m.resultingModuleIndex;
        moduleMerger = m.moduleMerger;
        if (resultingModuleIndex != nullptr) {
            g_object_ref(resultingModuleIndex);
        }
        if (moduleMerger != nullptr) {
            g_object_ref(moduleMerger);
        }
    }
    return *this;
}

ModuleMetadata::~ModuleMetadata()
{
    if (resultingModuleIndex != nullptr) {
        g_object_unref(resultingModuleIndex);
    }
    if (moduleMerger != nullptr) {
        g_object_unref(moduleMerger);
    }
}

void ModuleMetadata::addMetadataFromString(const std::string & yaml, int priority)
{
    GError *error = NULL;
    g_autoptr(GPtrArray) failures = NULL;

    ModulemdModuleIndex * mi = modulemd_module_index_new();
    gboolean success = modulemd_module_index_update_from_string(mi, yaml.c_str(), TRUE, &failures, &error);
    if(!success){
        ModuleMetadata::reportFailures(failures);
    }
    if (error)
        throw ModulePackageContainer::ResolveException( tfm::format(_("Failed to update from string: %s"), error->message));

    if (!moduleMerger){
        moduleMerger = modulemd_module_index_merger_new();
        if (resultingModuleIndex){
            // Priority is set to 0 in order to use the current resultingModuleIndex data as a baseline
            modulemd_module_index_merger_associate_index(moduleMerger, resultingModuleIndex, 0);
            g_clear_pointer(&resultingModuleIndex, g_object_unref);
        }
    }

    modulemd_module_index_merger_associate_index(moduleMerger, mi, priority);
    g_object_unref(mi);
}

void ModuleMetadata::resolveAddedMetadata()
{
    if (!moduleMerger)
        return;

    GError *error = NULL;

    resultingModuleIndex = modulemd_module_index_merger_resolve(moduleMerger, &error);
    if (error && !resultingModuleIndex){
        throw ModulePackageContainer::ResolveException(tfm::format(_("Failed to resolve: %s"),
                                                                    (error->message) ? error->message : "Unknown error"));
    }
    if (error) {
        auto logger(libdnf::Log::getLogger());
        logger->debug(tfm::format(_("There were errors while resolving modular defaults: %s"), error->message));
    }

    modulemd_module_index_upgrade_defaults(resultingModuleIndex, MD_DEFAULTS_VERSION_ONE, &error);
    if (error)
        throw ModulePackageContainer::ResolveException(tfm::format(_("Failed to upgrade defaults: %s"), error->message));
    modulemd_module_index_upgrade_streams(resultingModuleIndex, MD_MODULESTREAM_VERSION_TWO, &error);
    if (error)
        throw ModulePackageContainer::ResolveException(tfm::format(_("Failed to upgrade streams: %s"), error->message));
    g_clear_pointer(&moduleMerger, g_object_unref);
}

std::vector<ModulePackage *> ModuleMetadata::getAllModulePackages(DnfSack * moduleSack,
                                                                  LibsolvRepo * repo,
                                                                  const std::string & repoID,
                                                                  std::vector<std::tuple<LibsolvRepo *, ModulemdModuleStream *, std::string>> & modulesV2)
{
    std::vector<ModulePackage *> result;
    if (!resultingModuleIndex)
        return result;

    char ** moduleNames = modulemd_module_index_get_module_names_as_strv(resultingModuleIndex);

    for (char **moduleName = moduleNames; moduleName && *moduleName; moduleName++) {
        ModulemdModule * m =  modulemd_module_index_get_module(resultingModuleIndex, *moduleName);
        GPtrArray * streams = modulemd_module_get_all_streams(m);
        //TODO(amatej): replace with
        //GPtrArray * streams = modulemd_module_index_search_streams_by_nsvca_glob(resultingModuleIndex, NULL);
        for (unsigned int i = 0; i < streams->len; i++){
            ModulemdModuleStream * moduleMdStream = static_cast<ModulemdModuleStream *>(g_ptr_array_index(streams, i));
            if (modulemd_module_stream_get_mdversion(moduleMdStream) > 2) {
                result.push_back(new ModulePackage(moduleSack, repo, moduleMdStream, repoID));
            } else {
                g_object_ref(moduleMdStream);
                modulesV2.push_back(std::make_tuple(repo, moduleMdStream, repoID));
            }
        }
    }

    g_strfreev(moduleNames);
    return result;
}

std::map<std::string, std::string> ModuleMetadata::getDefaultStreams()
{
    std::map<std::string, std::string> moduleDefaults;
    if (!resultingModuleIndex)
        return moduleDefaults;

    GHashTable * table = modulemd_module_index_get_default_streams_as_hash_table(resultingModuleIndex, NULL);
    GHashTableIter iterator;
    gpointer key, value;
    g_hash_table_iter_init(&iterator, table);
    while (g_hash_table_iter_next(&iterator, &key, &value)) {
        moduleDefaults[(char *)key] = (char *) value; //defaultStream;
    }

    g_hash_table_unref(table);
    return moduleDefaults;
}

std::vector<std::string> ModuleMetadata::getDefaultProfiles(std::string moduleName, std::string moduleStream)
{
    std::vector<std::string> output;
    if (!resultingModuleIndex)
        return output;

    ModulemdModule * myModule = modulemd_module_index_get_module(resultingModuleIndex, moduleName.c_str());
    ModulemdDefaultsV1 * myDefaults = (ModulemdDefaultsV1 *) modulemd_module_get_defaults(myModule);

    char ** list = modulemd_defaults_v1_get_default_profiles_for_stream_as_strv(myDefaults, moduleStream.c_str(), NULL);

    for (char **iter = list; iter && *iter; iter++) {
        output.emplace_back(*iter);
    }

    g_strfreev(list);
    return output;
}

void ModuleMetadata::reportFailures(const GPtrArray *failures)
{
    for (unsigned int i = 0; i < failures->len; i++) {
        ModulemdSubdocumentInfo * item = (ModulemdSubdocumentInfo *)(g_ptr_array_index(failures, i));
        std::cerr << "Module yaml error: " << modulemd_subdocument_info_get_gerror(item)->message << "\n";
    }
}

ModulemdObsoletes * ModuleMetadata::getNewestActiveObsolete(ModulePackage *modulePkg)
{
    ModulemdModule * myModule = modulemd_module_index_get_module(resultingModuleIndex, modulePkg->getNameCStr());
    if (myModule == nullptr) {
        return nullptr;
    }

    GError *error = NULL;
    ModulemdModuleStream * myStream = modulemd_module_get_stream_by_NSVCA(myModule,
                                                                          modulePkg->getStreamCStr(),
                                                                          modulePkg->getVersionNum(),
                                                                          modulePkg->getContextCStr(),
                                                                          modulePkg->getArchCStr(),
                                                                          &error);
    if (error) {
        auto logger(libdnf::Log::getLogger());
        logger->debug(tfm::format(_("Cannot retrieve module obsoletes because no stream matching %s: %s"),
                                  modulePkg->getFullIdentifier(), error->message));
        return nullptr;
    }

    if (myStream == nullptr) {
        return nullptr;
    }

    return modulemd_module_stream_v2_get_obsoletes_resolved(MODULEMD_MODULE_STREAM_V2(myStream));
}

}
