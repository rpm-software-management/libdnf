#include <string>

#include "TransformerTest.hpp"
#include "libdnf/swdb/item_rpm.hpp"
#include "libdnf/swdb/swdb.hpp"

CPPUNIT_TEST_SUITE_REGISTRATION (TransformerTest);

TransformerMock::TransformerMock ()
  : Transformer ("", "")
{
}

void
TransformerTest::setUp ()
{
    swdb = std::unique_ptr<SQLite3> (new SQLite3 (":memory:"));
    history = std::unique_ptr<SQLite3> (new SQLite3 (":memory:"));
    SwdbCreateDatabase (*swdb.get ());

    const char *sql = R"**(
        CREATE TABLE pkgtups (
            pkgtupid INTEGER PRIMARY KEY,
            name TEXT NOT NULL,
            arch TEXT NOT NULL,
            epoch TEXT NOT NULL,
            version TEXT NOT NULL,
            release TEXT NOT NULL,
            checksum TEXT
        )
    )**";

    history.get ()->exec (sql);

    sql = R"**(
        INSERT INTO
            pkgtups
        VALUES
            (1, 'chrony', 'x86_64', 1, '3.1', '4.fc26', 'sha256:6cec2091'),
            (2, 'kernel', 'x86_64', 0, '4.11.6', '301.fc26', 'sha256:8dc6bb96')
    )**";

    history.get ()->exec (sql);
}

void
TransformerTest::tearDown ()
{
}

void
TransformerTest::testTransformRPMItems ()
{
    transformer.transformRPMItems (*swdb.get (), *history.get ());

    RPMItem chrony (*swdb.get (), 1);
    CPPUNIT_ASSERT (chrony.getId () == 1);
    CPPUNIT_ASSERT (chrony.getName () == "chrony");
    CPPUNIT_ASSERT (chrony.getEpoch () == 1);
    CPPUNIT_ASSERT (chrony.getVersion () == "3.1");
    CPPUNIT_ASSERT (chrony.getRelease () == "4.fc26");
    CPPUNIT_ASSERT (chrony.getChecksumType () == "sha256");
    CPPUNIT_ASSERT (chrony.getChecksumData () == "6cec2091");

    RPMItem kernel (*swdb.get (), 2);
    CPPUNIT_ASSERT (kernel.getId () == 2);
    CPPUNIT_ASSERT (kernel.getName () == "kernel");
    CPPUNIT_ASSERT (kernel.getEpoch () == 0);
    CPPUNIT_ASSERT (kernel.getVersion () == "4.11.6");
    CPPUNIT_ASSERT (kernel.getRelease () == "301.fc26");
    CPPUNIT_ASSERT (kernel.getChecksumType () == "sha256");
    CPPUNIT_ASSERT (kernel.getChecksumData () == "8dc6bb96");
}
