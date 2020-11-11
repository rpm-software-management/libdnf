/*
Copyright (C) 2017-2020 Red Hat, Inc.

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


#ifndef TEST_LIBDNF_TRANSACTION_TEST_REASON_HPP
#define TEST_LIBDNF_TRANSACTION_TEST_REASON_HPP


#include "transaction_test_base.hpp"

#include <cppunit/TestCase.h>
#include <cppunit/extensions/HelperMacros.h>


class TransactionItemReasonTest : public TransactionTestBase {
    CPPUNIT_TEST_SUITE(TransactionItemReasonTest);
    CPPUNIT_TEST(test_no_transaction);
    CPPUNIT_TEST(test_empty_transaction);
    CPPUNIT_TEST(test_failed_transaction);
    CPPUNIT_TEST(test_install);
    CPPUNIT_TEST(test_remove);
    CPPUNIT_TEST(test_package_reason_change);

    CPPUNIT_TEST(test_compare_operators);
    CPPUNIT_TEST(test_TransactionItemReason_compare);
    CPPUNIT_TEST_SUITE_END();

public:
    void test_no_transaction();
    void test_empty_transaction();
    void test_failed_transaction();
    void test_install();
    void test_remove();
    void test_package_reason_change();

    void test_compare_operators();
    void test_TransactionItemReason_compare();
};


#endif  // TEST_LIBDNF_TRANSACTION_TEST_REASON_HPP
