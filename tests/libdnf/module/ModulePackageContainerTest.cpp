#include "ModulePackageContainerTest.hpp"

CPPUNIT_TEST_SUITE_REGISTRATION(ModulePackageContainerTest);

#include "libdnf/log.hpp"
#include "libdnf/dnf-sack-private.hpp"

auto logger(libdnf::Log::getLogger());

void ModulePackageContainerTest::setUp()
{
    g_autoptr(GError) error = nullptr;

    context = dnf_context_new();
    dnf_context_set_release_ver(context, "26");
    dnf_context_set_arch(context, "x86_64");
    dnf_context_set_install_root(context, TESTDATADIR "/modules/");
    dnf_context_set_repo_dir(context, TESTDATADIR "/modules/yum.repos.d/");
    dnf_context_set_solv_dir(context, "/tmp");
    dnf_context_setup(context, nullptr, &error);
    g_assert_no_error(error);

    DnfState *state = dnf_context_get_state(context);
    dnf_context_setup_sack(context, state, &error);
    g_assert_no_error(error);

    modules = new ModulePackageContainer(false, TESTDATADIR "/modules/", "x86_64");
}

void ModulePackageContainerTest::tearDown()
{
    g_object_unref(context);
    delete modules;
}

void ModulePackageContainerTest::testEnabledModules()
{
    const char *hotfix = nullptr;
    auto sack = dnf_context_get_sack(context);
    dnf_sack_filter_modules_v2(sack, modules, &hotfix, TESTDATADIR "/modules/", "platform:26");

    const std::vector<std::string> specs = {"httpd:2.4", "base-runtime:f26" };
    for (const auto &spec : specs) {
        const auto &qRes = modules->query(spec);
        for (const auto &pkg : qRes)
            CPPUNIT_ASSERT(modules->isEnabled(pkg->getName(), pkg->getStream()));
    }
}

void ModulePackageContainerTest::testDisableModules()
{
    const char *hotfix = nullptr;
    auto sack = dnf_context_get_sack(context);
    dnf_sack_filter_modules_v2(sack, modules, &hotfix, TESTDATADIR "/modules/", "platform:26");

    modules->disable("httpd", "2.4");
    modules->disable("base-runtime", "f26");

    for (const auto &it : modules->getDisabledStreams()) {
        CPPUNIT_ASSERT(it.first == "httpd" || it.first == "base-runtime");
        if (it.first == "httpd")
            CPPUNIT_ASSERT(it.second == "2.4");
        else
            CPPUNIT_ASSERT(it.second == "f26");
    }

    modules->save("/etc/dnf/modules.d");
}

void ModulePackageContainerTest::testDisabledModules()
{
    const char *hotfix = nullptr;
    auto sack = dnf_context_get_sack(context);
    dnf_sack_filter_modules_v2(sack, modules, &hotfix, TESTDATADIR "/modules/", "platform:26");

    CPPUNIT_ASSERT(!modules->isEnabled("httpd", "2.4"));
    CPPUNIT_ASSERT(!modules->isEnabled("httpd", "2.2"));
    CPPUNIT_ASSERT(!modules->isEnabled("base-runtime", "f26"));
}

void ModulePackageContainerTest::testEnableModules()
{
    const char *hotfix = nullptr;
    auto sack = dnf_context_get_sack(context);
    dnf_sack_filter_modules_v2(sack, modules, &hotfix, TESTDATADIR "/modules/", "platform:26");

    modules->enable("httpd", "2.4");
    modules->enable("base-runtime", "f26");

    for (const auto &it : modules->getEnabledStreams()) {
        CPPUNIT_ASSERT(it.first == "httpd" || it.first == "base-runtime");
        if (it.first == "httpd")
            CPPUNIT_ASSERT(it.second == "2.4");
        else
            CPPUNIT_ASSERT(it.second == "f26");
    }

    modules->save("/etc/dnf/modules.d");
}

void ModulePackageContainerTest::testRollback()
{
    const char *hotfix = nullptr;
    auto sack = dnf_context_get_sack(context);
    dnf_sack_filter_modules_v2(sack, modules, &hotfix, TESTDATADIR "/modules/", "platform:26");

    modules->disable("httpd", "2.4");
    modules->disable("base-runtime", "f26");

    CPPUNIT_ASSERT(!modules->isEnabled("httpd", "2.4"));
    CPPUNIT_ASSERT(!modules->isEnabled("base-runtime", "f26"));

    modules->rollback();

    CPPUNIT_ASSERT(modules->isEnabled("httpd", "2.4"));
    CPPUNIT_ASSERT(modules->isEnabled("base-runtime", "f26"));
}
