#include "ModuleTest.hpp"

CPPUNIT_TEST_SUITE_REGISTRATION(ModuleTest);

#include "libdnf/log.hpp"
#include "libdnf/dnf-module.h"

static auto logger(libdnf::Log::getLogger());

void ModuleTest::setUp()
{
}

void ModuleTest::tearDown()
{
}

void ModuleTest::testDummy()
{
    std::vector<std::string> module_list;
    bool ret;

    logger->debug("called ModuleTest::testDummy()");

    /* call with empty module list should fail */
    ret = libdnf::dnf_module_dummy(module_list);
    CPPUNIT_ASSERT(ret == false);

    /* add some modules to the list and try again */
    module_list.push_back("moduleA");
    module_list.push_back("moduleB");
    module_list.push_back("moduleC");

    ret = libdnf::dnf_module_dummy(module_list);
    CPPUNIT_ASSERT(ret == true);
}

