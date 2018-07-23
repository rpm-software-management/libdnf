#include "ModuleTest.hpp"

CPPUNIT_TEST_SUITE_REGISTRATION(ModuleTest);

#include "libdnf/log.hpp"
#include "libdnf/dnf-module.h"
#include "libdnf/dnf-state.h"
#include "libdnf/dnf-repo-loader.h"
#include "libdnf/conf/ConfigParser.hpp"

static auto logger(libdnf::Log::getLogger());

void ModuleTest::setUp()
{
    GError *error = nullptr;
    context = dnf_context_new();

    dnf_context_set_release_ver(context, "f26");
    dnf_context_set_arch(context, "x86_64");
    constexpr auto install_root = TESTDATADIR "/modules/";
    dnf_context_set_install_root(context, install_root);
    constexpr auto repos_dir = TESTDATADIR "/modules/yum.repos.d/";
    dnf_context_set_platform_module(context, "platform:26");
    dnf_context_set_repo_dir(context, repos_dir);
    dnf_context_set_solv_dir(context, "/tmp");
    dnf_context_setup(context, nullptr, &error);
    g_assert_no_error(error);

    dnf_context_setup_sack(context, dnf_state_new(), &error);
    g_assert_no_error(error);

    auto loader = dnf_context_get_repo_loader(context);
    auto repo = dnf_repo_loader_get_repo_by_id(loader, "test", &error);
    g_assert_no_error(error);
    g_assert(repo != nullptr);
}

void ModuleTest::tearDown()
{
    g_object_unref(context);
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

void ModuleTest::testEnable()
{
    GPtrArray *repos = dnf_context_get_repos(context);
    auto sack = dnf_context_get_sack(context);
    auto install_root = dnf_context_get_install_root(context);
    auto platformModule = dnf_context_get_platform_module(context);

    logger->debug("called ModuleTest::testEnable()");

    /* call with empty module list should throw error */
    {
        std::vector<std::string> module_list;
        CPPUNIT_ASSERT_THROW(libdnf::dnf_module_enable(module_list, sack, repos, install_root, platformModule), libdnf::ModuleExceptionList);
    }

    /* call with invalid specs should fail */
    {
        std::vector<std::string> module_list{"moduleA:", "moduleB#streamB", "moduleC:streamC#profileC"};
        CPPUNIT_ASSERT_THROW(libdnf::dnf_module_enable(module_list, sack, repos, install_root, platformModule), libdnf::ModuleExceptionList);
    }

    /* call with valid specs should succeed */
    {
        std::vector<std::string> module_list{"httpd"};
        CPPUNIT_ASSERT(libdnf::dnf_module_enable(module_list, sack, repos, install_root, platformModule));
        libdnf::ConfigParser parser{};
        parser.read(TESTDATADIR "/modules/etc/dnf/modules.d/httpd.module");
        CPPUNIT_ASSERT(parser.getValue("httpd", "state") == "enabled");
        CPPUNIT_ASSERT(parser.getValue("httpd", "stream") == "2.4");
    }

    /* call with enabled module should succeed */
    {
        std::vector<std::string> module_list{"httpd:2.4"};
        CPPUNIT_ASSERT(libdnf::dnf_module_enable(module_list, sack, repos, install_root, platformModule));
        libdnf::ConfigParser parser{};
        parser.read(TESTDATADIR "/modules/etc/dnf/modules.d/httpd.module");
        CPPUNIT_ASSERT(parser.getValue("httpd", "state") == "enabled");
        CPPUNIT_ASSERT(parser.getValue("httpd", "stream") == "2.4");
    }
}
