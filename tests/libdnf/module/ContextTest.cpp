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

// XXX: look into sharing assert_list_names in the future
static gboolean pkglist_has_nevra(GPtrArray *pkglist, const char *nevra)
{
    for (guint i = 0; i < pkglist->len; i++) {
        DnfPackage *pkg = static_cast<DnfPackage *>(pkglist->pdata[i]);
        if (g_str_equal(dnf_package_get_nevra(pkg), nevra))
            return TRUE;
    }
    return FALSE;
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

    // try to install a nonexistent module
    const char *module_specs[] = {"nonexistent", NULL};
    g_assert(!dnf_context_module_install(context, module_specs, &error));
    g_assert(error);
    g_assert(strstr(error->message, "Unable to resolve argument 'nonexistent'"));
    g_clear_pointer(&error, g_error_free);

    // wrong stream
    module_specs[0] = "httpd:nonexistent";
    g_assert(!dnf_context_module_install(context, module_specs, &error));
    g_assert(error);
    g_assert(strstr(error->message, "Unable to resolve argument 'httpd:nonexistent'"));
    g_clear_pointer(&error, g_error_free);

    // try to install non-existent profile
    module_specs[0] = "httpd:2.4/nonexistent";
    g_assert(!dnf_context_module_install(context, module_specs, &error));
    g_assert(error);
    g_assert(strstr(error->message, "No profile found matching 'nonexistent'"));
    g_clear_pointer(&error, g_error_free);

    // install default profile from modulemd-defaults
    module_specs[0] = "httpd:2.4";
    g_assert(dnf_context_module_install(context, module_specs, &error));
    g_assert_no_error(error);
    HyGoal goal = dnf_context_get_goal(context);
    g_assert_cmpint(hy_goal_run_flags(goal, DNF_NONE), ==, 0);
    g_autoptr(GPtrArray) pkgs = hy_goal_list_installs(goal, &error);
    g_assert_no_error(error);
    g_assert(pkgs);
    g_assert(pkglist_has_nevra(pkgs, "httpd-2.4.25-8.x86_64"));
    g_assert(pkglist_has_nevra(pkgs, "libnghttp2-1.21.1-1.x86_64"));

    // Verify we can install the default stream from modulemd-defaults.
    // This would fail with EnableMultipleStreamsException if it didn't match
    // the 2.4 stream since we enabled the 2.4 stream just above.
    module_specs[0] = "httpd";
    g_assert(dnf_context_module_install(context, module_specs, &error));
    g_assert_no_error(error);
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
