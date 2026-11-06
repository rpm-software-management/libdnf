#include "NoModulemdTest.hpp"

#include "libdnf/dnf-context.hpp"
#include "libdnf/dnf-repo-loader.h"
#include "libdnf/sack/packageset.hpp"
#include "libdnf/sack/query.hpp"

#include <memory>

CPPUNIT_TEST_SUITE_REGISTRATION(NoModulemdTest);

void NoModulemdTest::setUp()
{
    dnf_context_set_config_file_path("");
    context = dnf_context_new();
}

void NoModulemdTest::tearDown()
{
    g_object_unref(context);
}

// The repository ContextTest filters by its module metadata, which is ignored
// here: its modular packages are available like any other.
void NoModulemdTest::testModularMetadataIgnored()
{
    g_autoptr(GError) error = nullptr;

    dnf_context_set_release_ver(context, "26");
    dnf_context_set_arch(context, "x86_64");
    dnf_context_set_install_root(context, TESTDATADIR "/modules/");
    g_autoptr(DnfLock) lock = dnf_lock_new();
    dnf_lock_set_lock_dir(lock, "/tmp");
    dnf_context_set_repo_dir(context, TESTDATADIR "/modules/yum.repos.d/");
    dnf_context_set_solv_dir(context, "/tmp");
    CPPUNIT_ASSERT(dnf_context_setup(context, nullptr, &error));

    DnfRepo * repo = dnf_repo_loader_get_repo_by_id(dnf_context_get_repo_loader(context), "test", &error);
    CPPUNIT_ASSERT(repo);
    g_autoptr(DnfState) state = dnf_state_new();
    dnf_repo_check(repo, G_MAXUINT, state, &error);
    g_clear_error(&error);

    CPPUNIT_ASSERT(dnf_context_setup_sack(context, dnf_context_get_state(context), &error));
    auto sack = dnf_context_get_sack(context);
    CPPUNIT_ASSERT(!std::unique_ptr<libdnf::PackageSet>(dnf_sack_get_module_excludes(sack)));

    // from a stream that isn't enabled, excluded with modularity
    libdnf::Query query{sack};
    query.addFilter(HY_PKG_NEVRA_STRICT, HY_EQ, "httpd-2.2.10-1.x86_64");
    CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(1), query.size());

    CPPUNIT_ASSERT(dnf_context_install(context, "httpd-2.4.25-8.x86_64", &error));
}
