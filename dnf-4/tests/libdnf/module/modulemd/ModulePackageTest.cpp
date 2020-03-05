#include "ModulePackageTest.hpp"

CPPUNIT_TEST_SUITE_REGISTRATION(ModulePackageTest);

#include "libdnf/log.hpp"
#include "libdnf/dnf-sack-private.hpp"

#include <algorithm>

void ModulePackageTest::setUp()
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
    packages = modules->getModulePackages();
}

void ModulePackageTest::tearDown()
{
    g_object_unref(context);
}

void ModulePackageTest::testSimpleGetters()
{
    CPPUNIT_ASSERT(packages[0]->getName() == "base-runtime");
    CPPUNIT_ASSERT(!strcmp(packages[0]->getNameCStr(), "base-runtime"));

    CPPUNIT_ASSERT(packages[0]->getStream() == "f26");
    CPPUNIT_ASSERT(!strcmp(packages[0]->getStreamCStr(), "f26"));

    CPPUNIT_ASSERT(packages[0]->getNameStream() == "base-runtime:f26");
    CPPUNIT_ASSERT(packages[0]->getNameStreamVersion() == "base-runtime:f26:1");

    CPPUNIT_ASSERT(packages[0]->getRepoID() == "test");

    CPPUNIT_ASSERT(packages[0]->getVersion() == "1");
    CPPUNIT_ASSERT(packages[0]->getVersionNum() == 1);

    CPPUNIT_ASSERT(packages[0]->getContext() == "e3b0c442");
    CPPUNIT_ASSERT(!strcmp(packages[0]->getContextCStr(), "e3b0c442"));

    CPPUNIT_ASSERT(packages[0]->getArch() == "x86_64");
    CPPUNIT_ASSERT(!strcmp(packages[0]->getArchCStr(), "x86_64"));

    CPPUNIT_ASSERT(packages[0]->getFullIdentifier() == "base-runtime:f26:1:e3b0c442:x86_64");

    CPPUNIT_ASSERT(packages[0]->getSummary() == "Fake module");
    CPPUNIT_ASSERT(packages[0]->getDescription() == "Fake module");

}

void ModulePackageTest::testGetArtifacts()
{
    std::vector<std::string> rpms = packages[0]->getArtifacts();

    CPPUNIT_ASSERT(rpms.size() == 12);
    CPPUNIT_ASSERT(rpms[0] == "basesystem-0:11-3.noarch");
    CPPUNIT_ASSERT(rpms[2] == "bash-doc-0:4.4.12-2.noarch");
    CPPUNIT_ASSERT(rpms[11] == "systemd-0:233-3.x86_64");
}

void ModulePackageTest::testGetProfiles()
{
    std::vector<libdnf::ModuleProfile> profiles = packages[0]->getProfiles();

    CPPUNIT_ASSERT(profiles.size() == 2);
    CPPUNIT_ASSERT(profiles[0].getName() == "default");
    CPPUNIT_ASSERT(profiles[1].getName() == "minimal");

    profiles = packages[0]->getProfiles("minimal");
    CPPUNIT_ASSERT(profiles.size() == 1);
    CPPUNIT_ASSERT(profiles[0].getName() == "minimal");

    profiles = packages[0]->getProfiles("mini*");
    CPPUNIT_ASSERT(profiles.size() == 1);
    CPPUNIT_ASSERT(profiles[0].getName() == "minimal");
}

void ModulePackageTest::testGetModuleDependencies()
{
    std::vector<libdnf::ModuleDependencies> deps = packages[0]->getModuleDependencies();

    CPPUNIT_ASSERT(deps.size() == 0);

    deps = packages[3]->getModuleDependencies();
    CPPUNIT_ASSERT(deps.size() == 1);

    std::vector <std::map<std::string, std::vector<std::string>>> reqs = deps[0].getRequires();
    CPPUNIT_ASSERT(reqs.size() == 1);
    std::map<std::string, std::vector<std::string>> map = reqs[0];

    CPPUNIT_ASSERT(map.size() == 1);
    std::vector<std::string> base_runtime_reqs = map["base-runtime"];
    CPPUNIT_ASSERT(base_runtime_reqs.size() == 0);

    deps = packages[4]->getModuleDependencies();
    CPPUNIT_ASSERT(deps.size() == 1);

    reqs = deps[0].getRequires();
    CPPUNIT_ASSERT(reqs.size() == 1);
    map = reqs[0];

    CPPUNIT_ASSERT(map.size() == 1);
    base_runtime_reqs = map["base-runtime"];
    CPPUNIT_ASSERT(base_runtime_reqs.size() == 1);
    CPPUNIT_ASSERT(base_runtime_reqs[0] == "f26");
}
