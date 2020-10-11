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


#ifndef TEST_LIBDNF_TRANSACTION_TEST_MIGRATION_HPP
#define TEST_LIBDNF_TRANSACTION_TEST_MIGRATION_HPP


#include "transaction_test_base.hpp"

#include "libdnf/utils/sqlite3/sqlite3.hpp"

#include <cppunit/TestCase.h>
#include <cppunit/extensions/HelperMacros.h>
#include <memory>


class TransactionMigrationTest : public TransactionTestBase {
    CPPUNIT_TEST_SUITE(TransactionMigrationTest);
    CPPUNIT_TEST(test_missing_schema_version);
    CPPUNIT_TEST(test_migration);
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp() override;

    void test_missing_schema_version();
    void test_migration();

private:
    std::unique_ptr<libdnf::utils::SQLite3> conn;
};


#endif  // TEST_LIBDNF_TRANSACTION_TEST_MIGRATION_HPP
