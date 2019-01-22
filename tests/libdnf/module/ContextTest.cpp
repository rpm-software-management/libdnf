#include "ContextTest.hpp"

CPPUNIT_TEST_SUITE_REGISTRATION(ContextTest);

#include "libdnf/dnf-context.hpp"
#include "libdnf/dnf-repo-loader.h"
#include "libdnf/sack/query.hpp"
#include "libdnf/nevra.hpp"
#include "libdnf/utils/File.hpp"
#include "libdnf/sack/packageset.hpp"

#include <memory>


void ContextTest::setUp()
{
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

    state = dnf_context_get_state(context);
    dnf_context_setup_sack(context, state, &error);
    g_assert_no_error(error);

    auto sack = dnf_context_get_sack(context);
    auto moduleExcludes = std::unique_ptr<libdnf::PackageSet>(dnf_sack_get_module_excludes(sack));
    CPPUNIT_ASSERT(moduleExcludes->size() != 0);

    auto modules_fn = dnf_repo_get_filename_md(repo, "modules");

    auto yaml = libdnf::File::newFile(modules_fn);
    yaml->open("r");
    const auto &yamlContent = yaml->getContent();
    yaml->close();

    auto modules = libdnf::ModuleMetadata::metadataFromString(yamlContent);
    for (const auto &module : modules) {
        // default module:stream
        if ((g_strcmp0(module.getName(), "httpd") == 0) &&
            (g_strcmp0(module.getStream(), "2.4") == 0))
            sackHas(sack, module);

        // disabled stream
        if ((g_strcmp0(module.getName(), "httpd") == 0) &&
            (g_strcmp0(module.getStream(), "2.2") == 0))
            sackHasNot(sack, module);
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

void ContextTest::sackHas(DnfSack * sack, const libdnf::ModuleMetadata & module) const
{
    libdnf::Query query{sack};
    auto artifacts = module.getArtifacts();
    for (auto artifact : artifacts) {
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

void ContextTest::sackHasNot(DnfSack * sack, const libdnf::ModuleMetadata & module) const
{
    libdnf::Query query{sack};
    auto artifacts = module.getArtifacts();
    for (auto artifact : artifacts) {
        artifact = artifact.replace(artifact.find("-0:"), 3, "-");
        query.addFilter(HY_PKG_NEVRA_STRICT, HY_EQ, artifact.c_str());

        auto packageSet = const_cast<libdnf::PackageSet *>(query.runSet());
        CPPUNIT_ASSERT(dnf_packageset_count(packageSet) == 0);

        query.clear();
    }
}
