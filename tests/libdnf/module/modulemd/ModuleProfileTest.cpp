#include "ModuleProfileTest.hpp"

CPPUNIT_TEST_SUITE_REGISTRATION(ModuleProfileTest);

#include "libdnf/log.hpp"
#include "libdnf/dnf-sack-private.hpp"

#include <algorithm>

void ModuleProfileTest::setUp()
{
    g_autoptr(GError) error = nullptr;

    dnf_context_set_config_file_path("");
    context = dnf_context_new();
    dnf_context_set_release_ver(context, "26");
    dnf_context_set_arch(context, "x86_64");
    dnf_context_set_platform_module(context, "platform:26");
    dnf_context_set_install_root(context, TESTDATADIR "/modules/");
    g_autoptr(DnfLock) lock = dnf_lock_new();
    dnf_lock_set_lock_dir(lock, "/tmp");
    dnf_context_set_repo_dir(context, TESTDATADIR "/modules/yum.repos.d/");
    dnf_context_set_solv_dir(context, "/tmp");
    dnf_context_setup(context, nullptr, &error);
    g_assert_no_error(error);

    DnfState *state = dnf_context_get_state(context);
    dnf_context_setup_sack(context, state, &error);
    g_assert_no_error(error);

    auto sack = dnf_context_get_sack(context);
    libdnf::ModulePackageContainer *modules = dnf_sack_get_module_container(sack);
    std::vector<libdnf::ModulePackage *> packages = modules->getModulePackages();
    profiles = packages[0]->getProfiles();
}

void ModuleProfileTest::tearDown()
{
    g_object_unref(context);
}

void ModuleProfileTest::testGetName()
{
    CPPUNIT_ASSERT((profiles[0].getName() == "default"));
    CPPUNIT_ASSERT((profiles[1].getName() == "minimal"));
}

void ModuleProfileTest::testGetDescription()
{
    CPPUNIT_ASSERT((profiles[0].getDescription() == ""));
    CPPUNIT_ASSERT((profiles[1].getDescription() == ""));
}

void ModuleProfileTest::testGetContent()
{
    std::vector<std::string> content = profiles[0].getContent();
    CPPUNIT_ASSERT(content.size() == 4);
    CPPUNIT_ASSERT(content[0] == "bash");
    CPPUNIT_ASSERT(content[1] == "glibc");
    CPPUNIT_ASSERT(content[2] == "kernel");
    CPPUNIT_ASSERT(content[3] == "systemd");

    content = profiles[1].getContent();
    CPPUNIT_ASSERT(content.size() == 1);
    CPPUNIT_ASSERT(content[0] == "glibc");
}
