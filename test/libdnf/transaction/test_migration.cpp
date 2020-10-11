/*
Copyright (C) 2020 Red Hat, Inc.

This file is part of libdnf: https://github.com/rpm-software-management/libdnf/

Libdnf is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

Libdnf is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with libdnf.  If not, see <https://www.gnu.org/licenses/>.
*/


#include "test_migration.hpp"

#include "libdnf/transaction/db/db.hpp"

#include <string>


using namespace libdnf::transaction;


CPPUNIT_TEST_SUITE_REGISTRATION(TransactionMigrationTest);


static const char * SQL_1_1_CREATE_TABLES =
#include "libdnf/transaction/db/sql/1.1_create_tables.sql"
    ;


void TransactionMigrationTest::setUp() {
    TransactionTestBase::setUp();
    auto path = persistdir->get_path();
    path /= "history.sqlite";
    conn = std::make_unique<libdnf::utils::SQLite3>(path.native());
    conn->exec(SQL_1_1_CREATE_TABLES);
}


void TransactionMigrationTest::test_missing_schema_version() {
    conn->exec("DELETE FROM config WHERE key='version';");

    auto base = new_base();
    auto trans = base->get_transaction_sack().new_transaction();

    try {
        trans->start();
    }
    catch (std::runtime_error & ex) {
        CPPUNIT_ASSERT_EQUAL(
            std::string(ex.what()),
            std::string("Unable to get 'version' from table 'config'"));
    }
}


void TransactionMigrationTest::test_migration() {
    // the initial version is "1.1"
    std::string version = transaction_get_schema_version(*conn);
    CPPUNIT_ASSERT_EQUAL(std::string("1.1"), version);

    const char * SQL_TRANS_INSERT_WITHOUT_COMMENT = R"**(
INSERT INTO
    trans (
        id,
        dt_begin,
        dt_end,
        rpmdb_version_begin,
        rpmdb_version_end,
        releasever,
        user_id,
        cmdline,
        state
    )
VALUES
    (1, 1, 1, '', '', '1', -1, '', 1);
)**";
    conn->exec(SQL_TRANS_INSERT_WITHOUT_COMMENT);

    // run a query to trigger database migration
    auto base = new_base();
    auto & sack = base->get_transaction_sack();
    auto q = sack.new_query();
    q.ifilter_id(libdnf::sack::QueryCmp::EXACT, 1);
    auto trans = q.get();

    // test that the Transaction has an empty comment field
    CPPUNIT_ASSERT_EQUAL(std::string(), trans->get_comment());

    // the latest version is "1.2"
    version = transaction_get_schema_version(*conn);
    CPPUNIT_ASSERT_EQUAL(std::string("1.2"), version);
}
