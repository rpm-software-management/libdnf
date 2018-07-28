#include <solv/pool.h>
#include <memory>
#include <solv/repo.h>
#include <solv/poolarch.h>

#include "DependencyTest.hpp"

CPPUNIT_TEST_SUITE_REGISTRATION(DependencyTest);

void DependencyTest::setUp()
{
    sack = dnf_sack_new();

    dependency = std::unique_ptr<libdnf::Dependency>(new libdnf::Dependency(sack, "foo = 1.0"));
}

void DependencyTest::tearDown()
{
    g_object_unref(sack);
}

void DependencyTest::testName()
{
    CPPUNIT_ASSERT(strcmp("foo", dependency->getName()) == 0);
}

void DependencyTest::testVersion()
{
    CPPUNIT_ASSERT(strcmp("1.0", dependency->getVersion()) == 0);
}

void DependencyTest::testParse()
{
    std::unique_ptr<libdnf::Dependency> dependency = std::unique_ptr<libdnf::Dependency>(
        new libdnf::Dependency(sack, "bar"));

    CPPUNIT_ASSERT(strcmp("bar", dependency->getName()) == 0);
    CPPUNIT_ASSERT(strcmp("", dependency->getVersion()) == 0);
}




