#include <chrono>
#include <cstdio>
#include <iostream>
#include <string>

#include "RpmItemTest.hpp"
#include "libdnf/swdb/item_rpm.hpp"
#include "libdnf/swdb/swdb.hpp"

CPPUNIT_TEST_SUITE_REGISTRATION(RpmItemTest);

void
RpmItemTest::setUp()
{
    conn = std::unique_ptr<SQLite3>(new SQLite3(":memory:"));
    SwdbCreateDatabase(*conn.get());
}

void
RpmItemTest::tearDown()
{
}

void
RpmItemTest::testCreate()
{
    // bash-4.4.12-5.fc26.x86_64
    RPMItem rpm(*conn.get());
    rpm.setName("bash");
    rpm.setEpoch(0);
    rpm.setVersion("4.4.12");
    rpm.setRelease("5.fc26");
    rpm.setArch("x86_64");
    rpm.setChecksumType("sha256");
    rpm.setChecksumData("e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
    rpm.save();

    RPMItem rpm2(*conn.get(), rpm.getId());
    CPPUNIT_ASSERT(rpm2.getId() == rpm.getId());
    CPPUNIT_ASSERT(rpm2.getName() == rpm.getName());
    CPPUNIT_ASSERT(rpm2.getEpoch() == rpm.getEpoch());
    CPPUNIT_ASSERT(rpm2.getVersion() == rpm.getVersion());
    CPPUNIT_ASSERT(rpm2.getRelease() == rpm.getRelease());
    CPPUNIT_ASSERT(rpm2.getChecksumType() == rpm.getChecksumType());
    CPPUNIT_ASSERT(rpm2.getChecksumData() == rpm.getChecksumData());
}

void
RpmItemTest::testGetTransactionItems()
{
    // performance looks good: 100k records take roughly 3.3s to write, 0.2s to read
    // change following constant to modify number of tested RPMItems
    constexpr int num = 10;

    Transaction trans(*conn.get());

    auto create_start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < num; i++) {
        auto rpm = std::make_shared<RPMItem>(*conn.get());
        rpm->setName("name_" + std::to_string(i));
        rpm->setEpoch(0);
        rpm->setVersion("1");
        rpm->setRelease("2");
        rpm->setArch("x86_64");
        trans.addItem(
            rpm, "base", TransactionItemAction::INSTALL, TransactionItemReason::USER, NULL);
    }
    trans.save();
    trans.saveItems();
    auto create_finish = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> create_duration = create_finish - create_start;

    auto read_start = std::chrono::high_resolution_clock::now();
    auto items = RPMItem::getTransactionItems(*conn.get(), trans.getId());
    for (auto i : items) {
        auto rpm = std::dynamic_pointer_cast<RPMItem>(i->getItem());
        // std::cout << rpm->getNEVRA() << std::endl;
    }
    auto read_finish = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> read_duration = read_finish - read_start;

    std::cout << "Create RPMItems: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(create_duration).count()
              << "ms" << std::endl;
    std::cout << "Read RPMItems: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(read_duration).count()
              << "ms" << std::endl;
}
