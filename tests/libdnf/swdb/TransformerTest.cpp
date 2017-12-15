#include <string>

#include "TransformerTest.hpp"
#include "libdnf/swdb/item_rpm.hpp"
#include "libdnf/swdb/swdb.hpp"
#include "libdnf/swdb/transaction.hpp"
#include "libdnf/swdb/transactionitem.hpp"

CPPUNIT_TEST_SUITE_REGISTRATION(TransformerTest);

TransformerMock::TransformerMock()
  : Transformer("", "")
{
}

void
TransformerTest::setUp()
{
    swdb = std::unique_ptr<SQLite3>(new SQLite3(":memory:"));
    history = std::unique_ptr<SQLite3>(new SQLite3(":memory:"));
    SwdbCreateDatabase(*swdb.get());

    const char * sql = R"**(
        CREATE TABLE pkgtups (
            pkgtupid INTEGER PRIMARY KEY,
            name TEXT NOT NULL,
            arch TEXT NOT NULL,
            epoch TEXT NOT NULL,
            version TEXT NOT NULL,
            release TEXT NOT NULL,
            checksum TEXT
        );
        CREATE TABLE trans_beg (
            tid INTEGER PRIMARY KEY,
            timestamp INTEGER NOT NULL,
            rpmdb_version TEXT NOT NULL,
            loginuid INTEGER
        );
        CREATE TABLE trans_end (
            tid INTEGER PRIMARY KEY REFERENCES trans_beg,
            timestamp INTEGER NOT NULL,
            rpmdb_version TEXT NOT NULL,
            return_code INTEGER NOT NULL
        );
        CREATE TABLE trans_cmdline (
            tid INTEGER NOT NULL REFERENCES trans_beg,
            cmdline TEXT NOT NULL
        );
        CREATE TABLE trans_data_pkgs (
            tid INTEGER NOT NULL REFERENCES trans_beg,
            pkgtupid INTEGER NOT NULL REFERENCES pkgtups,
            done BOOL NOT NULL DEFAULT FALSE, state TEXT NOT NULL
        );
    )**";

    history.get()->exec(sql);

    sql = R"**(
        INSERT INTO
            pkgtups
        VALUES
            (1, 'chrony', 'x86_64', 1, '3.1', '4.fc26', 'sha256:6cec2091'),
            (2, 'kernel', 'x86_64', 0, '4.11.6', '301.fc26', 'sha256:8dc6bb96')
    )**";

    history.get()->exec(sql);

    sql = R"**(
        INSERT INTO
            trans_beg
        VALUES
            (1, 1513267401, '2213:9795b6a4db5e5368628b5240ec63a629833c5594', 1000),
            (2, 1513267535, '2213:9eab991133c166f8bcf3ecea9fb422b853f7aebc', 1000);
    )**";

    history.get()->exec(sql);

    sql = R"**(
        INSERT INTO
            trans_end
        VALUES
            (1, 1513267509, '2213:9eab991133c166f8bcf3ecea9fb422b853f7aebc', 0),
            (2, 1513267539, '2214:e02004142740afb5b6d148d50bc84be4ab41ad13', 0);
    )**";

    history.get()->exec(sql);

    sql = R"**(
        INSERT INTO
            trans_cmdline
        VALUES
            (1, 'upgrade -y'),
            (2, '-y install Foo');
    )**";

    history.get()->exec(sql);

    sql = R"**(
        INSERT INTO
            trans_data_pkgs
        VALUES
            (1, 1, 'TRUE', 'Upgrade'),
            (2, 2, 'TRUE', 'Install');
    )**";

    history.get()->exec(sql);
}

void
TransformerTest::tearDown()
{
}

void
TransformerTest::testTransformRPMItems()
{
    transformer.transformRPMItems(*swdb.get(), *history.get());

    RPMItem chrony(*swdb.get(), 1);
    CPPUNIT_ASSERT(chrony.getId() == 1);
    CPPUNIT_ASSERT(chrony.getName() == "chrony");
    CPPUNIT_ASSERT(chrony.getEpoch() == 1);
    CPPUNIT_ASSERT(chrony.getVersion() == "3.1");
    CPPUNIT_ASSERT(chrony.getRelease() == "4.fc26");
    // CPPUNIT_ASSERT(chrony.getChecksumType() == "sha256");
    // CPPUNIT_ASSERT(chrony.getChecksumData() == "6cec2091");

    RPMItem kernel(*swdb.get(), 2);
    CPPUNIT_ASSERT(kernel.getId() == 2);
    CPPUNIT_ASSERT(kernel.getName() == "kernel");
    CPPUNIT_ASSERT(kernel.getEpoch() == 0);
    CPPUNIT_ASSERT(kernel.getVersion() == "4.11.6");
    CPPUNIT_ASSERT(kernel.getRelease() == "301.fc26");
    // CPPUNIT_ASSERT(kernel.getChecksumType() == "sha256");
    // CPPUNIT_ASSERT(kernel.getChecksumData() == "8dc6bb96");
}

void
TransformerTest::testTransformTrans()
{
    Transaction first(*swdb.get(), 1);

    CPPUNIT_ASSERT(first.getId() == 1);
    CPPUNIT_ASSERT(first.getDtBegin() == 1513267401);
    CPPUNIT_ASSERT(first.getDtEnd() == 1513267509);
    CPPUNIT_ASSERT(first.getRpmdbVersionBegin() == "2213:9795b6a4db5e5368628b5240ec63a629833c5594");
    CPPUNIT_ASSERT(first.getRpmdbVersionEnd() == "2213:9eab991133c166f8bcf3ecea9fb422b853f7aebc");
    CPPUNIT_ASSERT(first.getUserId() == 1000);
    CPPUNIT_ASSERT(first.getCmdline() == "upgrade -y");
    CPPUNIT_ASSERT(first.getDone() == true);

    first.loadItems();
    auto items = first.getItems();
    CPPUNIT_ASSERT(!items.empty());
    for (auto item : items) {
        if (item->getId() == 1) {
            CPPUNIT_ASSERT(item->getAction() == TransactionItemAction::UPGRADE);
            CPPUNIT_ASSERT(item->getDone() == true);
            // TODO reason, repo, replaced
        }
        else {
            CPPUNIT_ASSERT(false);
        }
    }

    Transaction second(*swdb.get(), 1);

    second.loadItems();
    items = second.getItems();
    CPPUNIT_ASSERT(!items.empty());
    for (auto item : items) {
        if (item->getId() == 2) {
            CPPUNIT_ASSERT(item->getAction() == TransactionItemAction::INSTALL);
            CPPUNIT_ASSERT(item->getDone() == true);
        }
        else {
            CPPUNIT_ASSERT(false);
        }
    }

    CPPUNIT_ASSERT(second.getId() == 2);
    CPPUNIT_ASSERT(second.getDtBegin() == 1513267535);
    CPPUNIT_ASSERT(second.getDtEnd() == 1513267539);
    CPPUNIT_ASSERT(second.getRpmdbVersionBegin() ==
                   "2213:9eab991133c166f8bcf3ecea9fb422b853f7aebc");
    CPPUNIT_ASSERT(second.getRpmdbVersionEnd() == "2214:e02004142740afb5b6d148d50bc84be4ab41ad13");
    CPPUNIT_ASSERT(second.getUserId() == 1000);
    CPPUNIT_ASSERT(second.getCmdline() == "-y install Foo");
    CPPUNIT_ASSERT(second.getDone() == true);
}
