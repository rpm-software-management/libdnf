#include "QueryTest.hpp"

#include "libdnf/dnf-sack-private.hpp"
#include "libdnf/hy-iutil-private.hpp"

CPPUNIT_TEST_SUITE_REGISTRATION(QueryTest);

#define UNITTEST_DIR "/tmp/libdnfXXXXXX"

void QueryTest::setUp()
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

    // loads modular data into ModulePackageContainer (No module enabled)
    dnf_sack_filter_modules_v2(sack, nullptr, nullptr, tmpdir, "platform_id:f33", false, false, false);

    libdnf::ModulePackageContainer * modules = dnf_sack_get_module_container(sack);
    CPPUNIT_ASSERT(modules->enable("perl-DBI", "master", false));
    CPPUNIT_ASSERT(modules->enable("perl", "5.23", false));
    // Modify modular data and make modules active (enabled - "perl-DBI:master", "perl:5.23")
    dnf_sack_filter_modules_v2(sack, modules, nullptr, tmpdir, nullptr, true, false, false);

    HyQuery query = new libdnf::Query(sack);
    std::vector<libdnf::AdvisoryPkg> advisoryPkgs;
    query->getAdvisoryPkgs(HY_EQ, advisoryPkgs);

    CPPUNIT_ASSERT(advisoryPkgs.size() > 0);

    delete query;
}

void QueryTest::tearDown()
{
    dnf_remove_recursive_v2(tmpdir, NULL);
    delete repo;
    g_object_unref(sack);
    g_free(tmpdir);
}

void QueryTest::testQueryGetAdvisoryPkgs()
{
    HyQuery query = new libdnf::Query(sack);
    std::vector<libdnf::AdvisoryPkg> advisoryPkgs;

    // Apply advisory only from active context - receave advisory package only from releted collection
    query->getAdvisoryPkgs(HY_EQ, advisoryPkgs);
    CPPUNIT_ASSERT(advisoryPkgs.size() == 1);

    CPPUNIT_ASSERT_EQUAL(std::string("test-perl-DBI"), std::string(advisoryPkgs[0].getNameString()));
    CPPUNIT_ASSERT_EQUAL(std::string("1-2.module_el8+6587+9879afr5"), std::string(advisoryPkgs[0].getEVRString()));
    //CPPUNIT_ASSERT(!g_strcmp0(advisoryPkgs[1].getNameString(), "test-perl-DBI"));

    // When modules are setup but none are enabled all collections are not applicable - no enabled module
    libdnf::ModulePackageContainer * modules = dnf_sack_get_module_container(sack);
    modules->reset("perl", false);
    modules->reset("perl-DBI", false);
    dnf_sack_filter_modules_v2(sack, modules, nullptr, tmpdir, nullptr, true, false, false);

    advisoryPkgs.clear();
    query->getAdvisoryPkgs(HY_EQ, advisoryPkgs);
    CPPUNIT_ASSERT(advisoryPkgs.size() == 0);

    // When I enable module from collection that contains non present pkg it doesn't show up
    CPPUNIT_ASSERT(modules->enable("perl", "5.23", false));
    dnf_sack_filter_modules_v2(sack, modules, nullptr, tmpdir, nullptr, true, false, false);

    advisoryPkgs.clear();
    query->getAdvisoryPkgs(HY_EQ, advisoryPkgs);
    CPPUNIT_ASSERT(advisoryPkgs.size() == 0);

    // When I enable a module with multiple collections I will receave advisory packages only for active context
    CPPUNIT_ASSERT(modules->enable("perl-DBI", "master", false));
    dnf_sack_filter_modules_v2(sack, modules, nullptr, tmpdir, nullptr, true, false, false);

    advisoryPkgs.clear();
    query->getAdvisoryPkgs(HY_EQ, advisoryPkgs);
    CPPUNIT_ASSERT(advisoryPkgs.size() == 1);
    CPPUNIT_ASSERT_EQUAL(std::string("test-perl-DBI"), std::string(advisoryPkgs[0].getNameString()));
    CPPUNIT_ASSERT_EQUAL(std::string("1-2.module_el8+6587+9879afr5"), std::string(advisoryPkgs[0].getEVRString()));

    delete query;
}

void QueryTest::testQueryFilterAdvisory()
{
    // When modules are not setup all advisory collections are applicable and there is no modular filtering
    HyQuery query = new libdnf::Query(sack);
    query->addFilter(HY_PKG_ADVISORY_TYPE, HY_EQ, "enhancement");
    CPPUNIT_ASSERT(query->size() == 1);

    // We get test-perl-DBI twice because its in two collections
    libdnf::PackageSet pset = *(query->getResultPset());
    DnfPackage *pkg = dnf_package_new(sack, pset[0]);
    CPPUNIT_ASSERT(!g_strcmp0(dnf_package_get_name(pkg), "test-perl-DBI"));
    g_object_unref(pkg);
    delete query;

    // When module are setup but none are enabled all collections are not applicable - no enabled module
    libdnf::ModulePackageContainer * modules = dnf_sack_get_module_container(sack);
    modules->reset("perl", false);
    modules->reset("perl-DBI", false);
    dnf_sack_filter_modules_v2(sack, modules, nullptr, tmpdir, nullptr, true, false, false);
    query = new libdnf::Query(sack);
    query->addFilter(HY_PKG_ADVISORY_TYPE, HY_EQ, "enhancement");
    CPPUNIT_ASSERT(query->size() == 0);
    delete query;

    // When I enable module from collection that contains non present pkg it doesn't show up
    CPPUNIT_ASSERT(modules->enable("perl", "5.23"));
    modules->save();
    dnf_sack_filter_modules_v2(sack, modules, nullptr, tmpdir, nullptr, true, false, false);
    query = new libdnf::Query(sack);
    query->addFilter(HY_PKG_ADVISORY_TYPE, HY_EQ, "enhancement");
    CPPUNIT_ASSERT(query->size() == 0);
    delete query;

    // When I enable a module from multiple collections that contain a present package I get them
    CPPUNIT_ASSERT(modules->enable("perl-DBI", "master"));
    modules->save();
    dnf_sack_filter_modules_v2(sack, modules, nullptr, tmpdir, nullptr, true, false, false);
    query = new libdnf::Query(sack);
    query->addFilter(HY_PKG_ADVISORY_TYPE, HY_EQ, "enhancement");
    CPPUNIT_ASSERT(query->size() == 1);
    libdnf::PackageSet pset2 = *(query->getResultPset());
    pkg = dnf_package_new(sack, pset2[0]);
    CPPUNIT_ASSERT(!g_strcmp0(dnf_package_get_name(pkg), "test-perl-DBI"));
    g_object_unref(pkg);
    delete query;
}
