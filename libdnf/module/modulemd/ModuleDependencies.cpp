#include <modulemd/modulemd-simpleset.h>

#include "libdnf/utils/utils.hpp"
#include "ModuleDependencies.hpp"

ModuleDependencies::ModuleDependencies(ModulemdDependencies *dependencies)
        : dependencies(dependencies)
{}

std::vector <std::map<std::string, std::vector<std::string>>> ModuleDependencies::getRequires()
{
    auto cRequires = modulemd_dependencies_peek_requires(dependencies);
    return getRequirements(cRequires);
}

std::map<std::string, std::vector<std::string>> ModuleDependencies::wrapModuleDependencies(const char *moduleName, ModulemdSimpleSet *streams) const
{
    std::map<std::string, std::vector<std::string> > moduleRequirements;
    auto streamSet = modulemd_simpleset_dup(streams);

    for (auto item = streamSet; *item; ++item) {
        moduleRequirements[moduleName].emplace_back(*item);
        g_free(*item);
    }
    g_free(streamSet);

    return moduleRequirements;
}

std::vector<std::map<std::string, std::vector<std::string>>> ModuleDependencies::getRequirements(GHashTable *requirements) const
{
    std::vector<std::map<std::string, std::vector<std::string>>> requires;
    requires.reserve(g_hash_table_size(requirements));

    GHashTableIter iterator;
    gpointer key, value;

    g_hash_table_iter_init (&iterator, requirements);
    while (g_hash_table_iter_next(&iterator, &key, &value)) {
        auto moduleRequirements = wrapModuleDependencies(static_cast<const char *>(key),
                                                         static_cast<ModulemdSimpleSet *>(value));

        requires.push_back(moduleRequirements);
    }

    return requires;
}
