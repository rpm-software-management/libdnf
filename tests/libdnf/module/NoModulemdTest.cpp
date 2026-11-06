#include "NoModulemdTest.hpp"

#include "libdnf/dnf-sack-private.hpp"
#include "libdnf/hy-iutil-private.hpp"
#include "libdnf/module/ModulePackageContainer.hpp"
#include "libdnf/sack/query.hpp"

CPPUNIT_TEST_SUITE_REGISTRATION(NoModulemdTest);

#define UNITTEST_DIR "/tmp/libdnfXXXXXX"

void NoModulemdTest::setUp()
{
    g_autoptr(GError) error = nullptr;

    tmpdir = g_strdup(UNITTEST_DIR);
    CPPUNIT_ASSERT(mkdtemp(tmpdir));

    sack = dnf_sack_new();
    dnf_sack_set_cachedir(sack, tmpdir);
    dnf_sack_set_arch(sack, "x86_64", NULL);
    dnf_sack_setup(sack, 0, NULL);
    // Its only packages are modular, from the streams in modules.yaml.gz
    repo = hy_repo_create("test_advisory_repo");
    std::string repodata = std::string(TESTDATADIR "/advisories/repodata/");
    hy_repo_set_string(repo, HY_REPO_MD_FN, (repodata + "repomd.xml").c_str());
    hy_repo_set_string(repo, HY_REPO_PRIMARY_FN, (repodata + "primary.xml.gz").c_str());
    hy_repo_set_string(repo, MODULES_FN, (repodata + "modules.yaml.gz").c_str());
    CPPUNIT_ASSERT(dnf_sack_load_repo(sack, repo, 0, &error));
}

void NoModulemdTest::tearDown()
{
    dnf_remove_recursive_v2(tmpdir, NULL);
    delete repo;
    g_object_unref(sack);
    g_free(tmpdir);
}

void NoModulemdTest::testModularMetadataIgnored()
{
    dnf_sack_filter_modules_v2(sack, nullptr, nullptr, tmpdir, "platform_id:f33", false, false, false);

    // No module streams are loaded, so nothing is filtered: with libmodulemd,
    // these packages would be excluded since none of their streams is enabled.
    CPPUNIT_ASSERT(dnf_sack_get_module_container(sack)->empty());
    libdnf::Query query(sack);
    query.addFilter(HY_PKG_NAME, HY_EQ, "test-perl-DBI");
    CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(2), query.size());
}
