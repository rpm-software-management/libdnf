#include <solv/pool.h>
#include "libdnf/repo/solvable/Dependency.hpp"
#include "DependencyContainerTest.hpp"

CPPUNIT_TEST_SUITE_REGISTRATION(DependencyContainerTest);

void DependencyContainerTest::setUp()
{
    sack = dnf_sack_new();
    container = std::unique_ptr<libdnf::DependencyContainer>(new libdnf::DependencyContainer(sack));
}

void DependencyContainerTest::tearDown()
{
    g_object_unref(sack);
}

void DependencyContainerTest::testAdd()
{
    auto dependency = new libdnf::Dependency(sack, "foo", "1.0", DNF_COMPARISON_EQ);

    container->add(dependency);
    libdnf::Dependency *actual = container->getPtr(0);

    CPPUNIT_ASSERT(strcmp(dependency->toString(), actual->toString()) == 0);
    delete actual;
    delete dependency;
}

void DependencyContainerTest::testExtend()
{
    auto otherContainer = new libdnf::DependencyContainer(sack);
    auto dependency = new libdnf::Dependency(sack, "foo", "1.0", DNF_COMPARISON_EQ);
    auto otherDependency = new libdnf::Dependency(sack, "bar", "1.1", DNF_COMPARISON_EQ);

    container->add(dependency);
    CPPUNIT_ASSERT_EQUAL(1, container->count());

    otherContainer->add(otherDependency);
    CPPUNIT_ASSERT_EQUAL(1, otherContainer->count());

    container->extend(otherContainer);
    libdnf::Dependency *actual = container->getPtr(0);
    CPPUNIT_ASSERT(strcmp(otherDependency->toString(), actual->toString()) == 0);

    delete otherContainer;
    delete dependency;
    delete otherDependency;
    delete actual;
}

void DependencyContainerTest::testGet()
{
    auto dependency = new libdnf::Dependency(sack, "foo", "1.0", DNF_COMPARISON_EQ);
    auto otherDependency = new libdnf::Dependency(sack, "bar", "1.1", DNF_COMPARISON_EQ);

    container->add(dependency);
    container->add(otherDependency);

    libdnf::Dependency *actual = container->getPtr(0);
    CPPUNIT_ASSERT(strcmp(dependency->toString(), actual->toString()) == 0);
    delete actual;

    actual = container->getPtr(1);
    CPPUNIT_ASSERT(strcmp(otherDependency->toString(), actual->toString()) == 0);

    delete dependency;
    delete otherDependency;
    delete actual;
}

void DependencyContainerTest::testCount()
{
    auto otherContainer = new libdnf::DependencyContainer(sack);
    auto dependency = new libdnf::Dependency(sack, "foo", "1.0", DNF_COMPARISON_EQ);

    CPPUNIT_ASSERT_EQUAL(0, container->count());
    container->add(dependency);
    CPPUNIT_ASSERT_EQUAL(1, container->count());

    otherContainer->add(dependency);
    CPPUNIT_ASSERT_EQUAL(1, otherContainer->count());

    container->extend(otherContainer);
    CPPUNIT_ASSERT_EQUAL(2, container->count());

    delete otherContainer;
    delete dependency;
}
