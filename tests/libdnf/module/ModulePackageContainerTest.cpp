#include "ModulePackageContainerTest.hpp"

CPPUNIT_TEST_SUITE_REGISTRATION(ModulePackageContainerTest);

#include "libdnf/log.hpp"
#include "libdnf/dnf-sack-private.hpp"

#include <algorithm>

void ModulePackageContainerTest::setUp()
{
    g_autoptr(GError) error = nullptr;

    context = dnf_context_new();
    dnf_context_set_release_ver(context, "26");
    dnf_context_set_arch(context, "x86_64");
    dnf_context_set_platform_module(context, "platform:26");
    dnf_context_set_install_root(context, TESTDATADIR "/modules/");
    dnf_context_set_repo_dir(context, TESTDATADIR "/modules/yum.repos.d/");
    dnf_context_set_solv_dir(context, "/tmp");
    dnf_context_setup(context, nullptr, &error);
    g_assert_no_error(error);

    DnfState *state = dnf_context_get_state(context);
    dnf_context_setup_sack(context, state, &error);
    g_assert_no_error(error);

    auto sack = dnf_context_get_sack(context);
    modules = dnf_sack_get_module_container(sack);
}

void ModulePackageContainerTest::tearDown()
{
    g_object_unref(context);
}

void ModulePackageContainerTest::testEnabledModules()
{
    const std::vector<std::string> specs = {"httpd:2.4", "base-runtime:f26" };
    for (const auto &spec : specs) {
        const auto &qRes = modules->query(spec);
        for (const auto &pkg : qRes)
            CPPUNIT_ASSERT(modules->isEnabled(pkg->getName(), pkg->getStream()));
    }
}

void ModulePackageContainerTest::testDisableModules()
{
    modules->disable("httpd");
    modules->disable("base-runtime");

    for (const auto &it : modules->getDisabledStreams()) {
        CPPUNIT_ASSERT(it.first == "httpd" || it.first == "base-runtime");
        if (it.first == "httpd")
            CPPUNIT_ASSERT(it.second == "2.4");
        else
            CPPUNIT_ASSERT(it.second == "f26");
    }

    modules->save();
}

void ModulePackageContainerTest::testDisabledModules()
{
    CPPUNIT_ASSERT(!modules->isEnabled("httpd", "2.4"));
    CPPUNIT_ASSERT(!modules->isEnabled("httpd", "2.2"));
    CPPUNIT_ASSERT(!modules->isEnabled("base-runtime", "f26"));
}

void ModulePackageContainerTest::testEnableModules()
{
    modules->enable("httpd", "2.4");
    modules->enable("base-runtime", "f26");

    for (const auto &it : modules->getEnabledStreams()) {
        CPPUNIT_ASSERT(it.first == "httpd" || it.first == "base-runtime");
        if (it.first == "httpd")
            CPPUNIT_ASSERT(it.second == "2.4");
        else
            CPPUNIT_ASSERT(it.second == "f26");
    }

    modules->save();
}

void ModulePackageContainerTest::testRollback()
{
    modules->disable("httpd");
    modules->disable("base-runtime");

    CPPUNIT_ASSERT(!modules->isEnabled("httpd", "2.4"));
    CPPUNIT_ASSERT(!modules->isEnabled("base-runtime", "f26"));

    modules->rollback();

    CPPUNIT_ASSERT(modules->isEnabled("httpd", "2.4"));
    CPPUNIT_ASSERT(modules->isEnabled("base-runtime", "f26"));
}

void ModulePackageContainerTest::testInstallProfile()
{
    modules->install("httpd", "2.4", "default");
    {
        auto installed = modules->getInstalledProfiles()["httpd"];
        auto it = std::find(installed.begin(), installed.end(), "default");
        CPPUNIT_ASSERT(it != installed.end());
    }

    modules->install("httpd", "2.4", "doc");
    {
        auto installed = modules->getInstalledProfiles()["httpd"];
        auto it = std::find(installed.begin(), installed.end(), "doc");
        CPPUNIT_ASSERT(it != installed.end());
    }

    modules->install("httpd", "2.4", "default");
    {
        auto installed = modules->getInstalledProfiles()["httpd"];
        CPPUNIT_ASSERT(installed.size() == 2);
    }

    modules->save();
}


void ModulePackageContainerTest::testRemoveProfile()
{
    modules->uninstall("httpd", "2.4", "default");
    {
        auto installed = modules->getInstalledProfiles()["httpd"];
        auto it = std::find(installed.begin(), installed.end(), "default");
        CPPUNIT_ASSERT(it == installed.end());
        auto removed = modules->getRemovedProfiles()["httpd"];
        it = std::find(removed.begin(), removed.end(), "default");
        CPPUNIT_ASSERT(it != removed.end());
    }

    modules->uninstall("httpd", "2.4", "doc");
    {
        auto installed = modules->getInstalledProfiles()["httpd"];
        auto it = std::find(installed.begin(), installed.end(), "doc");
        CPPUNIT_ASSERT(it == installed.end());
        auto removed = modules->getRemovedProfiles()["httpd"];
        it = std::find(removed.begin(), removed.end(), "doc");
        CPPUNIT_ASSERT(it != removed.end());
    }

    auto installed = modules->getInstalledProfiles()["httpd"];
    CPPUNIT_ASSERT(installed.empty());

    modules->save();
}
