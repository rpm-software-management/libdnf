#include "ModularityLabelTest.hpp"

#include "libdnf/dnf-context.h"
#include "libdnf/dnf-package.h"
#include "libdnf/dnf-repo.h"
#include "libdnf/dnf-rpmts-private.hpp"
#include "libdnf/dnf-sack-private.hpp"
#include "libdnf/hy-iutil-private.hpp"
#include "libdnf/sack/query.hpp"

CPPUNIT_TEST_SUITE_REGISTRATION(ModularityLabelTest);

#define UNITTEST_DIR "/tmp/libdnfXXXXXX"

void ModularityLabelTest::setUp()
{
    g_autoptr(GError) error = nullptr;

    tmpdir = g_strdup(UNITTEST_DIR);
    CPPUNIT_ASSERT(mkdtemp(tmpdir));

    sack = dnf_sack_new();
    dnf_sack_set_cachedir(sack, tmpdir);
    dnf_sack_set_arch(sack, "x86_64", NULL);
    dnf_sack_setup(sack, 0, NULL);
    repo = hy_repo_create("test_advisory_repo");
    std::string repodata = std::string(TESTDATADIR "/advisories/repodata/");
    hy_repo_set_string(repo, HY_REPO_MD_FN, (repodata + "repomd.xml").c_str());
    hy_repo_set_string(repo, HY_REPO_PRIMARY_FN, (repodata + "primary.xml.gz").c_str());
    hy_repo_set_string(repo, MODULES_FN, (repodata + "modules.yaml.gz").c_str());
    CPPUNIT_ASSERT(dnf_sack_load_repo(sack, repo, 0, &error));
}

void ModularityLabelTest::tearDown()
{
    dnf_remove_recursive_v2(tmpdir, NULL);
    delete repo;
    g_object_unref(sack);
    g_free(tmpdir);
}

// Adding a package with a modularity label to an RPM transaction: with
// libmodulemd it is refused unless its module stream is enabled, without it
// the package is treated as non-modular and accepted.
void ModularityLabelTest::testInstallLabelledPackage()
{
    g_autoptr(GError) error = nullptr;

    libdnf::Query query(sack);
    query.addFilter(HY_PKG_NAME, HY_EQ, "test-perl-DBI");
    query.addFilter(HY_PKG_RELEASE, HY_EQ, "2.module_el8+6587+9879afr5");
    CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(1), query.size());
    g_autoptr(GPtrArray) pkgs = query.run();
    auto pkg = static_cast<DnfPackage *>(g_ptr_array_index(pkgs, 0));
    // The fail-safe only applies to packages from a configured repository
    g_autoptr(DnfContext) context = dnf_context_new();
    g_autoptr(DnfRepo) dnfRepo = dnf_repo_new(context);
    dnf_package_set_repo(pkg, dnfRepo);

    // No module stream is enabled
    dnf_sack_filter_modules_v2(sack, nullptr, nullptr, tmpdir, "platform_id:f33", false, false, false);

    // The same package as in the repository, plus a modularity label
    const char * labelled = TESTDATADIR "/modularity-label/test-perl-DBI-1-2.module_el8+6587+9879afr5.x86_64.rpm";
    rpmts ts = rpmtsCreate();
    gboolean ret = dnf_rpmts_add_install_filename2(ts, labelled, TRUE, FALSE, pkg, &error);
    rpmtsFree(ts);
#ifdef WITH_MODULEMD
    CPPUNIT_ASSERT(!ret);
    CPPUNIT_ASSERT(error);
    CPPUNIT_ASSERT(strstr(error->message, "No available modular metadata"));
#else
    if (!ret) {
        CPPUNIT_FAIL(error->message);
    }
#endif
}
