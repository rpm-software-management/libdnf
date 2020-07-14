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


#include "test_group.hpp"

#include "libdnf/comps/comps.hpp"

#include <filesystem>
#include <fstream>


CPPUNIT_TEST_SUITE_REGISTRATION(CompsGroupTest);


void CompsGroupTest::setUp() {}


void CompsGroupTest::tearDown() {}


void CompsGroupTest::test_merge() {
    std::filesystem::path data_path = PROJECT_SOURCE_DIR "/test/libdnf/comps/data/";

    libdnf::comps::Comps comps;
    comps.load_installed();
    comps.load_from_file(data_path / "core.xml");
    comps.load_from_file(data_path / "standard.xml");

    auto q_core = comps.get_group_sack().new_query();
    q_core.ifilter_installed(false);
    q_core.ifilter_id(libdnf::sack::QueryCmp::EQ, "core");
    auto core = q_core.get();
    CPPUNIT_ASSERT_EQUAL(std::string("core"), core->get_id());
    CPPUNIT_ASSERT_EQUAL(std::string("Core"), core->get_name());

    // load another definiton of the core group that changes name from "Core" to "Core v2"
    comps.load_from_file(data_path / "core-v2.xml");

    auto q_core2 = comps.get_group_sack().new_query();
    q_core2.ifilter_installed(false);
    q_core2.ifilter_id(libdnf::sack::QueryCmp::EQ, "core");
    auto core2 = q_core2.get();
    CPPUNIT_ASSERT_EQUAL(std::string("core"), core2->get_id());
    CPPUNIT_ASSERT_EQUAL(std::string("Core v2"), core2->get_name());
}
