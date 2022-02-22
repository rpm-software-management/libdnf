#include "changelog.hpp"

#include <microdnf/iplugin.hpp>

#include <iostream>

using namespace microdnf;

namespace {

constexpr const char * PLUGIN_NAME = "dnf5_test_plugin";
constexpr Version PLUGIN_VERSION{0, 1, 0};
constexpr const char * PLUGIN_DESCRIPTION = "Test plugin.";
constexpr const char * PLUGIN_AUTHOR_NAME = "Jaroslav Rohel";
constexpr const char * PLUGIN_AUTHOR_EMAIL = "jrohel@redhat.com";

class ChangelogCmdPlugin : public IPlugin {
public:
    Version get_api_version() const noexcept override { return PLUGIN_API_VERSION; }

    const char * get_name() const noexcept override { return PLUGIN_NAME; }

    Version get_version() const noexcept override { return PLUGIN_VERSION; }

    const char * get_attribute(const char * attribute) const noexcept override {
        if (std::strcmp(attribute, "author_name") == 0)
            return PLUGIN_AUTHOR_NAME;
        if (std::strcmp(attribute, "author_email") == 0)
            return PLUGIN_AUTHOR_EMAIL;
        if (std::strcmp(attribute, "description") == 0)
            return PLUGIN_DESCRIPTION;
        return nullptr;
    }

    void init(microdnf::Context * context) override { this->context = context; }

    std::vector<std::unique_ptr<libdnf::cli::session::Command>> create_commands(libdnf::cli::session::Command & parent) override;

    //bool hook(HookId) override { return true; }

    void finish() noexcept override {}

private:
    microdnf::Context * context;
};


std::vector<std::unique_ptr<libdnf::cli::session::Command>> ChangelogCmdPlugin::create_commands(libdnf::cli::session::Command & parent) {
    std::vector<std::unique_ptr<libdnf::cli::session::Command>> commands;
    commands.push_back(std::make_unique<ChangelogCommand>(parent));
    return commands;
}


}


Version microdnf_plugin_get_api_version(void) { return PLUGIN_API_VERSION; }

const char * microdnf_plugin_get_name(void) { return PLUGIN_NAME; }

Version microdnf_plugin_get_version(void) { return PLUGIN_VERSION; }

IPlugin * microdnf_plugin_new_instance(Version api_version) try {
    if (api_version.major != PLUGIN_API_VERSION.major || api_version.minor != PLUGIN_API_VERSION.minor) {
        auto msg = fmt::format(
            "Unsupported API combination. API version implemented by plugin = \"{}.{}.{}\"."
            " API version in libdnf = \"{}.{}.{}\".",
            PLUGIN_API_VERSION.major,
            PLUGIN_API_VERSION.minor,
            PLUGIN_API_VERSION.micro,
            api_version.major,
            api_version.minor,
            api_version.micro);

        std::cout << "HAHA Test command" << std::endl;
        return nullptr;
    }
    return new ChangelogCmdPlugin;
} catch (...) {
    return nullptr;
}

void microdnf_plugin_delete_instance(IPlugin * plugin_object) { delete plugin_object; }
