#include <modulemd/modulemd-simpleset.h>
#include <utility>
#include <iostream>

#include "ModuleMetadata.hpp"
#include "profile/ProfileMaker.hpp"
#include "profile/ModuleProfile.hpp"

std::vector<std::shared_ptr<ModuleMetadata> > ModuleMetadata::metadataFromString(const std::string &fileContent)
{
    GError *error = nullptr;
    g_autoptr(GPtrArray) failures;
    g_autoptr(GPtrArray) data = modulemd_objects_from_string_ext(fileContent.c_str(), &failures, &error);

    reportFailures(failures);
    return wrapModulemdModule(data);
}

std::vector<std::shared_ptr<ModuleMetadata> > ModuleMetadata::wrapModulemdModule(GPtrArray *data)
{
    if (data == nullptr)
        return {};

    std::vector<std::shared_ptr<ModuleMetadata> > moduleCluster;
    for (unsigned int i = 0; i < data->len; i++) {
        auto module = g_ptr_array_index(data, i);
        if (!MODULEMD_IS_MODULE(module))
            continue;

        g_object_ref(module);
        auto modulemd = std::shared_ptr<ModulemdModule>((ModulemdModule *) module, g_object_unref);
        moduleCluster.push_back(std::make_shared<ModuleMetadata>(modulemd));
    }

    return moduleCluster;
}

void ModuleMetadata::reportFailures(const GPtrArray *failures)
{
    for (unsigned int i = 0; i < failures->len; i++) {
        auto item = static_cast<ModulemdSubdocument *>(g_ptr_array_index(failures, i));
        std::cerr << "Module yaml error: " << modulemd_subdocument_get_gerror(item)->message << "\n";
    }
}

ModuleMetadata::ModuleMetadata(const std::shared_ptr<ModulemdModule> &modulemd)
        : modulemd(modulemd)
{}

ModuleMetadata::~ModuleMetadata() = default;

std::string ModuleMetadata::getName() const
{
    const char *name = modulemd_module_peek_name(modulemd.get());
    return name ? name : "";
}

std::string ModuleMetadata::getStream() const
{
    const char *stream = modulemd_module_peek_stream(modulemd.get());
    return stream ? stream : "";
}

long long ModuleMetadata::getVersion() const
{
    return (long long) modulemd_module_peek_version(modulemd.get());
}

std::string ModuleMetadata::getContext() const
{
    const char *context = modulemd_module_peek_context(modulemd.get());
    return context ? context : "";
}

std::string ModuleMetadata::getArchitecture() const
{
    const char *arch = modulemd_module_peek_arch(modulemd.get());
    return arch ? arch : "";
}

std::string ModuleMetadata::getDescription() const
{
    const char *description = modulemd_module_peek_description(modulemd.get());
    return description ? description : "";
}

std::string ModuleMetadata::getSummary() const
{
    const char *summary = modulemd_module_peek_summary(modulemd.get());
    return summary ? summary : "";
}

std::vector<std::shared_ptr<ModuleDependencies> > ModuleMetadata::getDependencies() const
{
    auto cDependencies = modulemd_module_peek_dependencies(modulemd.get());
    std::vector<std::shared_ptr<ModuleDependencies> > dependencies;

    for (unsigned int i = 0; i < cDependencies->len; i++) {
        dependencies.push_back(std::make_shared<ModuleDependencies>((ModulemdDependencies *) g_ptr_array_index(cDependencies, i)));
    }

    return dependencies;
}

std::vector<std::string> ModuleMetadata::getArtifacts() const
{
    ModulemdSimpleSet *cArtifacts = modulemd_module_peek_rpm_artifacts(modulemd.get());
    gchar **rpms = modulemd_simpleset_dup(cArtifacts);

    std::vector<std::string> artifacts;
    for (auto item = rpms; *item; ++item) {
        artifacts.emplace_back(*item);
        g_free(*item);
    }
    g_free(rpms);

    return artifacts;
}

std::vector<std::shared_ptr<ModuleProfile>> ModuleMetadata::getProfiles() const
{
    GHashTable *cRequires = modulemd_module_peek_profiles(modulemd.get());
    std::vector<std::shared_ptr<ModuleProfile> > profiles;
    profiles.reserve(g_hash_table_size(cRequires));

    GHashTableIter iterator;
    gpointer key, value;

    g_hash_table_iter_init (&iterator, cRequires);
    while (g_hash_table_iter_next(&iterator, &key, &value)) {
        auto profile = (ModulemdProfile *) value;
        profiles.push_back(std::make_shared<ModuleProfile>(profile));
    }

    return profiles;
}

std::shared_ptr<Profile> ModuleMetadata::getProfile(const std::string &profileName) const
{
    return ProfileMaker::getProfile(profileName, modulemd);
}
