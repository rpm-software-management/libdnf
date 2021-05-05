#include "AdvisoryTest.hpp"

#include "libdnf/dnf-sack-private.hpp"
#include "libdnf/hy-iutil-private.hpp"

CPPUNIT_TEST_SUITE_REGISTRATION(AdvisoryTest);

#define UNITTEST_DIR "/tmp/libdnfXXXXXX"

void AdvisoryTest::setUp()
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
    advisory = advisoryPkgs[0].getAdvisory();
    delete query;
}

void AdvisoryTest::tearDown()
{
    dnf_remove_recursive_v2(tmpdir, NULL);
    delete repo;
    delete advisory;
    g_object_unref(sack);
    g_free(tmpdir);
}

void AdvisoryTest::testGetName()
{
    CPPUNIT_ASSERT(!strcmp(advisory->getName(), "FEDORA-2019-0329090518"));
}

void AdvisoryTest::testGetKind()
{
    CPPUNIT_ASSERT(advisory->getKind() == DNF_ADVISORY_KIND_ENHANCEMENT);
}

void AdvisoryTest::testGetDescription()
{
    CPPUNIT_ASSERT(!strcmp(advisory->getDescription(), "Enhance some stuff"));
}

void AdvisoryTest::testGetRights()
{
    CPPUNIT_ASSERT(!strcmp(advisory->getRights(), "Everyone has them"));
}

void AdvisoryTest::testGetSeverity()
{
    CPPUNIT_ASSERT(!strcmp(advisory->getSeverity(), "none"));
}

void AdvisoryTest::testGetTitle()
{
    CPPUNIT_ASSERT(!strcmp(advisory->getTitle(), "glibc bug fix"));
}

void AdvisoryTest::testGetPackages()
{
    std::vector<libdnf::AdvisoryPkg> pkgsvector;
    advisory->getPackages(pkgsvector);
    CPPUNIT_ASSERT(pkgsvector.size() == 4);
}

void AdvisoryTest::testGetApplicablePackagesModulesNotSetup()
{
    std::vector<libdnf::AdvisoryPkg> pkgsvector;

    // When modules are not setup all advisory collections are applicable and we get all packages
    advisory->getApplicablePackages(pkgsvector);
    CPPUNIT_ASSERT_EQUAL(2, static_cast<int>(pkgsvector.size()));
    CPPUNIT_ASSERT(!g_strcmp0(pkgsvector[0].getNameString(), "test-perl-DBI"));
    CPPUNIT_ASSERT(!g_strcmp0(pkgsvector[1].getNameString(), "not-present"));
}

void AdvisoryTest::testGetApplicablePackagesModulesSetupNoneEnabled()
{
    std::vector<libdnf::AdvisoryPkg> pkgsvector;

    // When module are setup but none are enabled no collections are applicable -> no packages
    libdnf::ModulePackageContainer * modules = dnf_sack_get_module_container(sack);
    modules->reset("perl", false);
    modules->reset("perl-DBI", false);
    dnf_sack_filter_modules_v2(sack, modules, nullptr, tmpdir, nullptr, true, false, false);

    advisory->getApplicablePackages(pkgsvector);
    CPPUNIT_ASSERT(pkgsvector.size() == 0);
}

void AdvisoryTest::testGetApplicablePackagesOneApplicableCollection()
{
    std::vector<libdnf::AdvisoryPkg> pkgsvector;

    // When I keep enabled only perl module I get packages from all collections that contain that module
    libdnf::ModulePackageContainer * modules = dnf_sack_get_module_container(sack);
    modules->reset("perl-DBI");
    dnf_sack_filter_modules_v2(sack, modules, nullptr, tmpdir, nullptr, true, false, false);

    advisory->getApplicablePackages(pkgsvector);
    CPPUNIT_ASSERT(pkgsvector.size() == 1);
    CPPUNIT_ASSERT(!g_strcmp0(pkgsvector[0].getNameString(), "not-present"));
}

void AdvisoryTest::testGetApplicablePackagesMultipleApplicableCollections()
{
    std::vector<libdnf::AdvisoryPkg> pkgsvector;

    // When I enable modules from multiple collections -> I get packages from all applicable collections
    // Enabled - "perl-DBI:master", "perl:5.23"
    advisory->getApplicablePackages(pkgsvector);
    CPPUNIT_ASSERT(pkgsvector.size() == 2);
    CPPUNIT_ASSERT(!g_strcmp0(pkgsvector[0].getNameString(), "test-perl-DBI"));
    CPPUNIT_ASSERT(!g_strcmp0(pkgsvector[1].getNameString(), "not-present"));
}

void AdvisoryTest::testGetModules()
{
    std::vector<libdnf::AdvisoryModule> modulesvector;
    modulesvector = advisory->getModules();
    CPPUNIT_ASSERT(modulesvector.size() == 4);
}

void AdvisoryTest::testGetReferences()
{
    std::vector<libdnf::AdvisoryRef> refsvector;
    advisory->getReferences(refsvector);
    CPPUNIT_ASSERT(refsvector.size() == 2);
}
