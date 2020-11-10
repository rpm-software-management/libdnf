#include "DnfPackageTest.hpp"

#include "libdnf/dnf-sack-private.hpp"
#include "libdnf/hy-iutil-private.hpp"

CPPUNIT_TEST_SUITE_REGISTRATION(DnfPackageTest);

#define UNITTEST_DIR "/tmp/libdnfXXXXXX"

void DnfPackageTest::setUp()
{
    g_autoptr(GError) error = nullptr;

    tmpdir = g_strdup(UNITTEST_DIR);
    char *retptr = mkdtemp(tmpdir);
    CPPUNIT_ASSERT(retptr);

    sack = dnf_sack_new();
    // Cache should not be needed, setting just to be safe
    dnf_sack_set_cachedir(sack, tmpdir);
    dnf_sack_set_arch(sack, "x86_64", NULL);
    dnf_sack_setup(sack, 0, NULL);
    repo = hy_repo_create("test_advisory_repo");
    std::string repodata = std::string(TESTDATADIR "/advisories/repodata/");
    hy_repo_set_string(repo, HY_REPO_MD_FN, (repodata + "repomd.xml").c_str());
    hy_repo_set_string(repo, HY_REPO_PRIMARY_FN, (repodata + "primary.xml.gz").c_str());
    hy_repo_set_string(repo, HY_REPO_UPDATEINFO_FN, (repodata + "updateinfo.xml.gz").c_str());
    hy_repo_set_string(repo, MODULES_FN, (repodata + "modules.yaml.gz").c_str());
    dnf_sack_load_repo(sack, repo, DNF_SACK_LOAD_FLAG_USE_UPDATEINFO, &error);

    HyQuery query = new libdnf::Query(sack);
    std::vector<libdnf::AdvisoryPkg> advisoryPkgs;
    query->getAdvisoryPkgs(HY_EQ, advisoryPkgs);

    CPPUNIT_ASSERT(advisoryPkgs.size() > 0);

    delete query;
}

void DnfPackageTest::tearDown()
{
    dnf_remove_recursive_v2(tmpdir, NULL);
    delete repo;
    g_object_unref(sack);
    g_free(tmpdir);
}

void DnfPackageTest::testDnfPackageGetAdvisories()
{
    HyQuery query = new libdnf::Query(sack);
    DnfPackage * p = dnf_package_new(sack, query->getIndexItem(0));

    CPPUNIT_ASSERT(!g_strcmp0(dnf_package_get_nevra(p), "test-perl-DBI-1-2.module_el8+6587+9879afr5.x86_64"));

    GPtrArray *advisories;

    // When modules are not setup all advisory collections are applicable
    advisories = dnf_package_get_advisories(p, HY_EQ);
    CPPUNIT_ASSERT(advisories->len == 1);

    dnf_sack_filter_modules_v2(sack, nullptr, nullptr, tmpdir, nullptr, false, false, false);

    // When module are setup but none are enabled all collections are not applicable
    advisories = dnf_package_get_advisories(p, HY_EQ);
    CPPUNIT_ASSERT(advisories->len == 0);

    libdnf::ModulePackageContainer * modules = dnf_sack_get_module_container(sack);

    // When I enable module from collection that doesn't contain p package I don't get the advisory
    CPPUNIT_ASSERT(modules->enable("perl", "5.23"));
    advisories = dnf_package_get_advisories(p, HY_EQ);
    CPPUNIT_ASSERT(advisories->len == 0);

    // When I enable module from collection that contains p package I get the advisory
    CPPUNIT_ASSERT(modules->enable("perl-DBI", "master"));
    advisories = dnf_package_get_advisories(p, HY_EQ);
    CPPUNIT_ASSERT(advisories->len == 1);

    delete query;
}

