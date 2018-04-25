#include <iostream>
#include "ModuleDefaultsContainer.hpp"
#include "../ModulePackageContainer.hpp"

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

void ModuleDefaultsContainer::fromFile(const std::string &path, int priority)
{
    GError *error = nullptr;
    g_autoptr(GPtrArray) failures;
    g_autoptr(GPtrArray) data = modulemd_objects_from_file_ext(path.c_str(), &failures, &error);

    saveDefaults(data, priority);
    reportFailures(failures);
}

std::string ModuleDefaultsContainer::getDefaultStreamFor(std::string moduleName)
{
    auto moduleDefaults = defaults[moduleName];
    if (!moduleDefaults)
        throw ModulePackageContainer::NoStreamException("Missing default for " + moduleName);
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
