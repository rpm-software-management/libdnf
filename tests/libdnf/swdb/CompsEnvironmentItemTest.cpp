#include <chrono>
#include <cstdio>
#include <iostream>
#include <string>

#include "libdnf/swdb/item_comps_environment.hpp"
#include "libdnf/swdb/swdb.hpp"

#include "CompsEnvironmentItemTest.hpp"

CPPUNIT_TEST_SUITE_REGISTRATION(CompsEnvironmentItemTest);

void
CompsEnvironmentItemTest::setUp()
{
    conn = std::make_shared< SQLite3 >(":memory:");
    Swdb swdb(conn);
    swdb.createDatabase();
}

void
CompsEnvironmentItemTest::tearDown()
{
}

static std::shared_ptr< CompsEnvironmentItem >
createCompsEnvironment(std::shared_ptr< SQLite3 > conn)
{
    auto env = std::make_shared< CompsEnvironmentItem >(conn);
    env->setEnvironmentId("minimal");
    env->setName("Minimal Environment");
    env->setTranslatedName("translated(Minimal Environment)");
    env->setPackageTypes(CompsPackageType::DEFAULT);
    env->addGroup("core", true, CompsPackageType::MANDATORY);
    env->addGroup("base", false, CompsPackageType::OPTIONAL);
    env->save();
    return env;
}

void
CompsEnvironmentItemTest::testCreate()
{
    auto env = createCompsEnvironment(conn);

    CompsEnvironmentItem env2(conn, env->getId());
    env2.loadGroups();
    CPPUNIT_ASSERT(env2.getId() == env->getId());
    CPPUNIT_ASSERT(env2.getEnvironmentId() == env->getEnvironmentId());
    CPPUNIT_ASSERT(env2.getName() == env->getName());
    CPPUNIT_ASSERT(env2.getTranslatedName() == env->getTranslatedName());
    CPPUNIT_ASSERT(env2.getPackageTypes() == env->getPackageTypes());

    {
        auto group = env2.getGroups().at(0);
        CPPUNIT_ASSERT(group->getGroupId() == "base");
        CPPUNIT_ASSERT(group->getInstalled() == false);
        CPPUNIT_ASSERT(group->getGroupType() == CompsPackageType::OPTIONAL);
    }
    {
        auto group = env2.getGroups().at(1);
        CPPUNIT_ASSERT(group->getGroupId() == "core");
        CPPUNIT_ASSERT(group->getInstalled() == true);
        CPPUNIT_ASSERT(group->getGroupType() == CompsPackageType::MANDATORY);
    }
}

void
CompsEnvironmentItemTest::testGetTransactionItems()
{
    Transaction trans(conn);
    auto env = createCompsEnvironment(conn);
    trans.addItem(env, "", TransactionItemAction::INSTALL, TransactionItemReason::USER);
    trans.save();

    Transaction trans2(conn, trans.getId());
    auto transItem = trans.getItems().at(0);
    auto env2 = transItem->getCompsEnvironmentItem();
    env2->loadGroups();
    {
        auto group = env2->getGroups().at(0);
        CPPUNIT_ASSERT(group->getGroupId() == "base");
    }
    {
        auto group = env2->getGroups().at(1);
        CPPUNIT_ASSERT(group->getGroupId() == "core");
    }
}
