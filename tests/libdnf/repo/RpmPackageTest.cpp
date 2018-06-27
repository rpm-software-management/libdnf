#include "RpmPackageTest.hpp"

#include "libdnf/repo/RpmPackage.hpp"
#include "libdnf/sack/query.hpp"
#include "libdnf/sack/packageset.hpp"
#include "libdnf/utils/memory.hpp"
#include "tests/libdnf/TestMacros.hpp"
#include "tests/libdnf/CppUnitMacros.hpp"

CPPUNIT_TEST_SUITE_REGISTRATION(RpmPackageTest);

namespace {

std::unique_ptr<libdnf::RpmPackage> newRpmPackage(const TestSack &sack, const char *name)
{
    libdnf::Query query{sack.getSack().get()};
    query.addFilter(HY_PKG_NAME, HY_EQ, name);

    auto packageSet = query.runSet();
    auto package = std::make_unique<libdnf::RpmPackage>(sack.getSack().get(), packageSet->operator[](0));

    return package;
}

std::unique_ptr<libdnf::RpmPackage> newRpmPackage(const TestSack &sack, const char *name, const char *repoName)
{
    libdnf::Query query{sack.getSack().get()};
    query.addFilter(HY_PKG_NAME, HY_EQ, name);
    query.addFilter(HY_PKG_REPONAME, HY_EQ, repoName);
    auto packageSet = query.runSet();
    auto package = std::make_unique<libdnf::RpmPackage>(sack.getSack().get(), packageSet->operator[](0));

    return package;
}

};

RpmPackageTest::RpmPackageTest()
        : sack{TESTREPODATADIR, UNITTEST_DIR}
{}

void RpmPackageTest::setUp()
{
    sack.loadRepos(HY_SYSTEM_REPO_NAME, nullptr);
}

void RpmPackageTest::tearDown()
{}

void RpmPackageTest::testSummary()
{
    auto package = newRpmPackage(sack, "penny-lib");
    auto summary = package->getSummary();
    CPPUNIT_ASSERT_CHAR_EQUAL("in my ears", summary);
}

void RpmPackageTest::testIdentical()
{
    auto package1 = newRpmPackage(sack, "penny-lib");
    auto package2 = newRpmPackage(sack, "flying");
    auto package3 = newRpmPackage(sack, "penny-lib");

    CPPUNIT_ASSERT(*package1 == *package3);
    CPPUNIT_ASSERT_FAIL(*package2 == *package3);
}

void RpmPackageTest::testVersions()
{
    auto package = newRpmPackage(sack, "baby");
    CPPUNIT_ASSERT_CHAR_EQUAL("6:5.0-11", package->getEvr());
    CPPUNIT_ASSERT_EQUAL(6ul, package->getEpoch());
    CPPUNIT_ASSERT_CHAR_EQUAL("5.0", package->getVersion());
    CPPUNIT_ASSERT_CHAR_EQUAL("11", package->getRelease());

    package = newRpmPackage(sack, "jay");
    // epoch missing if it's 0:
    CPPUNIT_ASSERT_CHAR_EQUAL("5.0-0", package->getEvr());
    CPPUNIT_ASSERT_EQUAL(0ul, package->getEpoch());
    CPPUNIT_ASSERT_CHAR_EQUAL("5.0", package->getVersion());
    CPPUNIT_ASSERT_CHAR_EQUAL("0", package->getRelease());
}

void RpmPackageTest::testNoSourcerpm()
{
    auto package = newRpmPackage(sack, "baby");
    CPPUNIT_ASSERT(package->getSourceRpm() == nullptr);
}

void RpmPackageTest::testSourcerpm()
{
    auto package = newRpmPackage(sack, "tour");
    CPPUNIT_ASSERT_CHAR_EQUAL("tour-4-6.src.rpm", package->getSourceRpm());

    package = newRpmPackage(sack, "mystery-devel");
    CPPUNIT_ASSERT_CHAR_EQUAL("mystery-19.67-1.src.rpm", package->getSourceRpm());
}

void RpmPackageTest::testGetRequires()
{
    auto package = newRpmPackage(sack, "flying");
    auto requires = package->getRequires();

    CPPUNIT_ASSERT_EQUAL(1, requires->count());
    CPPUNIT_ASSERT_CHAR_EQUAL("P-lib >= 3", requires->get(0)->toString());

    package = newRpmPackage(sack, "walrus");
    CPPUNIT_ASSERT_EQUAL(2, package->getRequires()->count());
}

void RpmPackageTest::testChecksumFail()
{
    auto package = newRpmPackage(sack, "walrus");
    auto checksum = package->getChecksum();
    CPPUNIT_ASSERT(std::get<0>(checksum) == nullptr);
    checksum = package->getHeaderChecksum();
    CPPUNIT_ASSERT(std::get<0>(checksum) == nullptr);
}

void RpmPackageTest::testChecksum()
{
    auto package = newRpmPackage(sack, "mystery-devel");
    auto checksum = package->getChecksum();
    CPPUNIT_ASSERT(std::get<1>(checksum) == G_CHECKSUM_SHA256);
    // Check the first and last bytes. Those need to match against information in primary.xml.gz.
    CPPUNIT_ASSERT_EQUAL(std::get<0>(checksum)[0], static_cast<const unsigned char>(0x2e));
    CPPUNIT_ASSERT_EQUAL(std::get<0>(checksum)[31], static_cast<const unsigned char>(0xf5));
}

void RpmPackageTest::testGetFiles()
{
    auto package = newRpmPackage(sack, "tour");
    auto files = package->getFiles();

    CPPUNIT_ASSERT_EQUAL(6ul, files.size());
}

void RpmPackageTest::testGetAdvisories()
{
    auto package = newRpmPackage(sack, "tour");
    auto advisories = package->getAdvisories(HY_GT);
    CPPUNIT_ASSERT_EQUAL(1ul, advisories.size());
    auto advisory = advisories.at(0);
    CPPUNIT_ASSERT_CHAR_EQUAL(advisory->getName(), "FEDORA-2008-9969");

    package = newRpmPackage(sack, "mystery-devel");
    advisories = package->getAdvisories(HY_GT | HY_EQ);
    CPPUNIT_ASSERT_EQUAL(1ul, advisories.size());

    advisories = package->getAdvisories(HY_LT | HY_EQ);
    CPPUNIT_ASSERT_EQUAL(1ul, advisories.size());
}

void RpmPackageTest::testLookupNum()
{
    auto package = newRpmPackage(sack, "tour");
    auto buildtime = package->getBuildTime();
    CPPUNIT_ASSERT_GREATER(1330473600ul, buildtime); // after 2012-02-29
    CPPUNIT_ASSERT_LESS(1456704000ul, buildtime); // before 2016-02-29
}

void RpmPackageTest::testInstalled()
{
    auto package1 = newRpmPackage(sack, "penny-lib", "main");
    auto package2 = newRpmPackage(sack, "penny-lib", HY_SYSTEM_REPO_NAME);
    CPPUNIT_ASSERT_FAIL(package1->isInstalled());
    CPPUNIT_ASSERT(package2->isInstalled());
}

void RpmPackageTest::testTwoSacks()
{
    TestSack otherSack{TESTREPODATADIR, UNITTEST_DIR "/tmp"};
    const char *path = pool_tmpjoin(otherSack.getPool(), otherSack.getRepoDirPath(), "change.repo", nullptr);

    otherSack.loadRepo("change", path, false);

    auto packageFromOtherSack = newRpmPackage(otherSack, "penny-lib");
    CPPUNIT_ASSERT_FAIL(packageFromOtherSack == nullptr);

    auto packageFromSack = newRpmPackage(sack, "penny-lib");
    CPPUNIT_ASSERT_FAIL(packageFromSack == nullptr);

    /* "penny-lib" is in both pools but at different offsets */
    Solvable *s1 = pool_id2solvable(otherSack.getPool(), packageFromOtherSack->getId());
    Solvable *s2 = pool_id2solvable(sack.getPool(), packageFromSack->getId());
    CPPUNIT_ASSERT_FAIL(s1->name == s2->name);
    CPPUNIT_ASSERT_EQUAL(0, packageFromOtherSack->compare(*packageFromSack));
}

void RpmPackageTest::testPackager()
{
    auto package = newRpmPackage(sack, "tour");
    CPPUNIT_ASSERT_CHAR_EQUAL("roll up <roll@up.net>", package->getPackager());
}

#define TOUR_45_46_DRPM_CHKSUM "\xc3\xc3\xd5\x72\xa4\x6b"\
    "\x1a\x66\x90\x6d\x42\xca\x17\x63\xef\x36\x20\xf7\x02"\
    "\x58\xaa\xac\x4c\x14\xbf\x46\x3e\xd5\x37\x16\xd4\x44"

void RpmPackageTest::testPresto()
{
    auto tour = newRpmPackage(sack, "tour");
    CPPUNIT_ASSERT(tour != nullptr);

    auto delta = tour->getDelta("4-5");
    const char *location = dnf_packagedelta_get_location(delta.get());
    CPPUNIT_ASSERT_CHAR_EQUAL("drpms/tour-4-5_4-6.noarch.drpm", location);

    const char *baseurl = dnf_packagedelta_get_baseurl(delta.get());
    CPPUNIT_ASSERT(baseurl == nullptr);

    auto size = dnf_packagedelta_get_downloadsize(delta.get());
    CPPUNIT_ASSERT_EQUAL(3132ul, size);

    int type;
    HyChecksum *csum = dnf_packagedelta_get_chksum(delta.get(), &type);
    CPPUNIT_ASSERT(type == G_CHECKSUM_SHA256);
    CPPUNIT_ASSERT(!memcmp(csum, TOUR_45_46_DRPM_CHKSUM, 32));
}

void RpmPackageTest::testGetFilesCommandLine()
{
    auto package = newRpmPackage(sack, "tour");
    auto files = package->getFiles();
    CPPUNIT_ASSERT_EQUAL(6ul, files.size());
}
