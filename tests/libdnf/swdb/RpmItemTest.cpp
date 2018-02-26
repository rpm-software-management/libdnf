#include <chrono>
#include <cstdio>
#include <iostream>
#include <string>

#include "RpmItemTest.hpp"
#include "libdnf/swdb/RPMItem.hpp"
#include "libdnf/swdb/Transformer.hpp"

CPPUNIT_TEST_SUITE_REGISTRATION(RpmItemTest);

void
RpmItemTest::setUp()
{
    conn = std::make_shared< SQLite3 >(":memory:");
    Transformer::createDatabase(conn);
}

void
RpmItemTest::tearDown()
{
}

void
RpmItemTest::testCreate()
{
    // bash-4.4.12-5.fc26.x86_64
    RPMItem rpm(conn);
    rpm.setName("bash");
    rpm.setEpoch(0);
    rpm.setVersion("4.4.12");
    rpm.setRelease("5.fc26");
    rpm.setArch("x86_64");
    rpm.save();

    RPMItem rpm2(conn, rpm.getId());
    CPPUNIT_ASSERT(rpm2.getId() == rpm.getId());
    CPPUNIT_ASSERT(rpm2.getName() == rpm.getName());
    CPPUNIT_ASSERT(rpm2.getEpoch() == rpm.getEpoch());
    CPPUNIT_ASSERT(rpm2.getVersion() == rpm.getVersion());
    CPPUNIT_ASSERT(rpm2.getRelease() == rpm.getRelease());
}

void
RpmItemTest::testGetTransactionItems()
{
    // performance looks good: 100k records take roughly 3.3s to write, 0.2s to read
    // change following constant to modify number of tested RPMItems
    constexpr int num = 10;

    libdnf::swdb_private::Transaction trans(conn);

    auto create_start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < num; i++) {
        auto rpm = std::make_shared< RPMItem >(conn);
        rpm->setName("name_" + std::to_string(i));
        rpm->setEpoch(0);
        rpm->setVersion("1");
        rpm->setRelease("2");
        rpm->setArch("x86_64");
        auto ti = trans.addItem(rpm, "base", TransactionItemAction::INSTALL, TransactionItemReason::USER);
        ti->setState(TransactionItemState::DONE);
    }
    trans.begin();
    trans.finish(TransactionState::DONE);
    auto create_finish = std::chrono::high_resolution_clock::now();
    std::chrono::duration< double > create_duration = create_finish - create_start;

    auto read_start = std::chrono::high_resolution_clock::now();
    auto items = RPMItem::getTransactionItems(conn, trans.getId());
    for (auto i : items) {
        auto rpm = std::dynamic_pointer_cast< RPMItem >(i->getItem());
        // std::cout << rpm->getNEVRA() << std::endl;
    }
    auto read_finish = std::chrono::high_resolution_clock::now();
    std::chrono::duration< double > read_duration = read_finish - read_start;

    auto createMs = std::chrono::duration_cast< std::chrono::milliseconds >(create_duration);
    auto readMs = std::chrono::duration_cast< std::chrono::milliseconds >(read_duration);

    //CPPUNIT_ASSERT(createMs.count() == 0);
    //CPPUNIT_ASSERT(readMs.count() == 0);
}
