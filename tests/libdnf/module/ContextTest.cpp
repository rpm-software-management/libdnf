#include "ContextTest.hpp"

CPPUNIT_TEST_SUITE_REGISTRATION(ContextTest);

#include "libdnf/dnf-context.hpp"
#include "libdnf/dnf-repo-loader.h"
#include "libdnf/sack/query.hpp"
#include "libdnf/nevra.hpp"
#include "libdnf/utils/File.hpp"
#include "libdnf/sack/packageset.hpp"
#include "libdnf/dnf-sack-private.hpp"

#include <memory>


void ContextTest::setUp()
{
    dnf_context_set_config_file_path("");
    context = dnf_context_new();
}

void ContextTest::tearDown()
{
    g_object_unref(context);
}

void ContextTest::testLoadModules()
{
    GError *error = nullptr;

    /* set up local context */
    // set releasever to avoid crashing on missing /etc/os-release in the test data
    dnf_context_set_release_ver(context, "26");
    dnf_context_set_arch(context, "x86_64");
    constexpr auto install_root = TESTDATADIR "/modules/";
    dnf_context_set_install_root(context, install_root);
    g_autoptr(DnfLock) lock = dnf_lock_new();
    dnf_lock_set_lock_dir(lock, "/tmp");
    constexpr auto repos_dir = TESTDATADIR "/modules/yum.repos.d/";
    dnf_context_set_repo_dir(context, repos_dir);
    dnf_context_set_solv_dir(context, "/tmp");
    auto ret = dnf_context_setup(context, nullptr, &error);
    g_assert_no_error(error);
    g_assert(ret);

    /* load local metadata repo */
    DnfRepo *repo = dnf_repo_loader_get_repo_by_id(dnf_context_get_repo_loader(context), "test", &error);
    g_assert_no_error(error);
    g_assert(repo != nullptr);
    g_assert_cmpint(dnf_repo_get_enabled(repo), ==, DNF_REPO_ENABLED_METADATA | DNF_REPO_ENABLED_PACKAGES);
    g_assert_cmpint(dnf_repo_get_kind(repo), ==, DNF_REPO_KIND_LOCAL);

    DnfState *state = dnf_state_new();
    dnf_repo_check(repo, G_MAXUINT, state, &error);
    g_object_unref(state);

    // Setup platform to prevent logger critical message
    dnf_context_set_platform_module(context, "platform:26");

    state = dnf_context_get_state(context);
    dnf_context_setup_sack(context, state, &error);
    g_assert_no_error(error);

    auto sack = dnf_context_get_sack(context);
    auto moduleExcludes = std::unique_ptr<libdnf::PackageSet>(dnf_sack_get_module_excludes(sack));
    CPPUNIT_ASSERT(moduleExcludes->size() != 0);

    libdnf::ModulePackageContainer * c = dnf_sack_get_module_container(sack);
    std::vector<libdnf::ModulePackage *> packages = c->getModulePackages();
    for(auto const& package: packages) {
        if (package->getName() == "httpd"){
            // disabled stream
            if (package->getStream() == "2.2")
                sackHasNot(sack, package);
            // default module:stream
            if (package->getStream() == "2.4")
                sackHas(sack, package);
        }
    }

    {
        libdnf::Query query{sack};
        // no match with modular RPM $name -> keep
        std::string nevra = "grub2-2.02-0.40.x86_64";
        query.addFilter(HY_PKG_NEVRA_STRICT, HY_EQ, nevra.c_str());
        auto packageSet = const_cast<libdnf::PackageSet *>(query.runSet());
        CPPUNIT_ASSERT(dnf_packageset_count(packageSet) >= 1);
        auto package = dnf_package_new(sack, packageSet->operator[](0));
        CPPUNIT_ASSERT(dnf_package_get_nevra(package) == nevra);
        g_object_unref(package);
        query.clear();
    }

    {
        libdnf::Query query{sack};
        // $name matches with modular RPM $name -> exclude
        std::string nevra = "httpd-2.2.10-1.x86_64";
        query.addFilter(HY_PKG_NEVRA_STRICT, HY_EQ, nevra.c_str());
        auto packageSet = const_cast<libdnf::PackageSet *>(query.runSet());
        CPPUNIT_ASSERT(dnf_packageset_count(packageSet) == 0);
        query.clear();
    }

    {
        libdnf::Query query{sack};
        // Provides: $name matches with modular RPM $name -> exclude
        std::string nevra = "httpd-provides-name-3.0-1.x86_64";
        query.addFilter(HY_PKG_NEVRA_STRICT, HY_EQ, nevra.c_str());
        auto packageSet = const_cast<libdnf::PackageSet *>(query.runSet());
        CPPUNIT_ASSERT(dnf_packageset_count(packageSet) == 0);
        query.clear();
    }

    {
        libdnf::Query query{sack};
        // Provides: $name = ... matches with modular RPM $name -> exclude
        std::string nevra = "httpd-provides-name-version-release-3.0-1.x86_64";
        query.addFilter(HY_PKG_NEVRA_STRICT, HY_EQ, nevra.c_str());
        auto packageSet = const_cast<libdnf::PackageSet *>(query.runSet());
        CPPUNIT_ASSERT(dnf_packageset_count(packageSet) == 0);
        query.clear();
    }
}

void ContextTest::sackHas(DnfSack * sack, libdnf::ModulePackage * pkg) const
{
    libdnf::Query query{sack};
    auto artifacts = pkg->getArtifacts();
    for(auto & artifact: artifacts) {
        artifact = artifact.replace(artifact.find("-0:"), 3, "-");
        query.addFilter(HY_PKG_NEVRA_STRICT, HY_EQ, artifact.c_str());

        auto packageSet = const_cast<libdnf::PackageSet *>(query.runSet());
        CPPUNIT_ASSERT(dnf_packageset_count(packageSet) >= 1);
        auto package = dnf_package_new(sack, packageSet->operator[](0));
        CPPUNIT_ASSERT(dnf_package_get_nevra(package) == artifact);
        g_object_unref(package);

        query.clear();

    }
}

void ContextTest::sackHasNot(DnfSack * sack, libdnf::ModulePackage * pkg) const
{
    libdnf::Query query{sack};
    auto artifacts = pkg->getArtifacts();
    for(auto & artifact: artifacts) {
        artifact = artifact.replace(artifact.find("-0:"), 3, "-");
        query.addFilter(HY_PKG_NEVRA_STRICT, HY_EQ, artifact.c_str());

        auto packageSet = const_cast<libdnf::PackageSet *>(query.runSet());
        CPPUNIT_ASSERT(dnf_packageset_count(packageSet) == 0);

        query.clear();
    }
}
