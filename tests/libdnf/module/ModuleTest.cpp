#include "ModuleTest.hpp"

CPPUNIT_TEST_SUITE_REGISTRATION(ModuleTest);

#include "libdnf/log.hpp"
#include "libdnf/dnf-module.h"
#include "libdnf/dnf-state.h"
#include "libdnf/dnf-repo-loader.h"
#include "libdnf/conf/ConfigParser.hpp"

#include "libdnf/utils/logger.hpp"
#include <iostream>
class MyLogger : public Logger {
public:
    void write(int source, time_t time, pid_t pid, Level level, const std::string & message) {
        std::cout << source << " " << time << " " << pid << " " << levelToCStr(level) << ": " << message << std::endl;
    }

};
static MyLogger myLogger;

void ModuleTest::setUp()
{
    libdnf::Log::setLogger(&myLogger);
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
    auto logger = libdnf::Log::getLogger();
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
    auto logger = libdnf::Log::getLogger();
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

void ModuleTest::testDisable()
{
    auto logger = libdnf::Log::getLogger();
    GPtrArray *repos = dnf_context_get_repos(context);
    auto sack = dnf_context_get_sack(context);
    auto install_root = dnf_context_get_install_root(context);
    auto platformModule = dnf_context_get_platform_module(context);

    logger->debug("called ModuleTest::testDisable()");

    /* call with empty module list should throw error */
    {
        std::vector<std::string> module_list;
        CPPUNIT_ASSERT_THROW(libdnf::dnf_module_disable(module_list, sack, repos, install_root, platformModule), libdnf::ModuleExceptionList);
    }

    /* call with invalid specs should fail */
    {
        std::vector<std::string> module_list{"moduleA:", "moduleB#streamB", "moduleC:streamC#profileC"};
        CPPUNIT_ASSERT_THROW(libdnf::dnf_module_disable(module_list, sack, repos, install_root, platformModule), libdnf::ModuleExceptionList);
    }

    /* call with valid specs should succeed */
    {
        std::vector<std::string> module_list{"httpd:2.4", "base-runtime"};
        CPPUNIT_ASSERT(libdnf::dnf_module_disable(module_list, sack, repos, install_root, platformModule));
        libdnf::ConfigParser parser{};
        parser.read(TESTDATADIR "/modules/etc/dnf/modules.d/httpd.module");
        CPPUNIT_ASSERT(parser.getValue("httpd", "state") == "disabled");
        CPPUNIT_ASSERT(parser.getValue("httpd", "stream") == "");
        parser.read(TESTDATADIR "/modules/etc/dnf/modules.d/base-runtime.module");
        CPPUNIT_ASSERT(parser.getValue("base-runtime", "state") == "disabled");
        CPPUNIT_ASSERT(parser.getValue("base-runtime", "stream") == "");
    }

    /* call with disabled module should succeed */
    {
        std::vector<std::string> module_list{"httpd:2.4"};
        CPPUNIT_ASSERT(libdnf::dnf_module_disable(module_list, sack, repos, install_root, platformModule));
        libdnf::ConfigParser parser{};
        parser.read(TESTDATADIR "/modules/etc/dnf/modules.d/httpd.module");
        CPPUNIT_ASSERT(parser.getValue("httpd", "state") == "disabled");
        CPPUNIT_ASSERT(parser.getValue("httpd", "stream") == "");
    }
}

static void
debugDumpQueryResults(std::vector<std::shared_ptr<ModulePackage>> &results)
{
    auto logger = libdnf::Log::getLogger();
    std::ostringstream oss;

    oss << "Results count = " << results.size();
    logger->debug(oss.str());

    int i = 0;
    for (const auto &result : results) {
        auto desc = result->getFullIdentifier();
        if (desc == "")
           desc = "<null description>";
        std::ostringstream().swap(oss);
        oss << " Result #" << ++i << ": " << desc;
        logger->debug(oss.str());
    }
}

static bool
moduleExists(const std::string &modspec,
             std::vector<std::shared_ptr<ModulePackage>> &modules)
{
    for (const auto &module : modules) {
        if (module->getFullIdentifier() == modspec) {
            return true;
        }
    }
    return false;
}

void ModuleTest::testQuery()
{
    auto logger = libdnf::Log::getLogger();
    GPtrArray *repos = dnf_context_get_repos(context);
    auto sack = dnf_context_get_sack(context);
    auto install_root = dnf_context_get_install_root(context);
    auto platformModule = dnf_context_get_platform_module(context);

    logger->debug("called ModuleTest::testQuery()");

    {
        logger->debug("query with no filters");

        std::vector<std::shared_ptr<ModulePackage>> results;

        results = libdnf::dnf_module_query(std::vector<libdnf::Filter>(), sack, repos, install_root, platformModule);
        debugDumpQueryResults(results);

        const std::vector<std::string> module_list {
            "base-runtime:f26:1:e3b0c442:x86_64",
            "base-runtime:f26:2:e3b0c442:x86_64",
            "base-runtime:rhel73:1:e3b0c442:x86_64",
            "httpd:2.2:1:e3b0c442:x86_64",
            "httpd:2.4:1:3b6beaf8:x86_64",
            "httpd:2.4:2:3b6beaf8:x86_64",
        };
        CPPUNIT_ASSERT(results.size() == module_list.size());
        for (const auto &modspec : module_list)
            CPPUNIT_ASSERT_MESSAGE(modspec, moduleExists(modspec, results));
    }

    {
        logger->debug("query with filtering for latest");

        std::vector<std::shared_ptr<ModulePackage>> results;
        std::vector<libdnf::Filter> filters;

        filters.push_back(libdnf::Filter(HY_MOD_LATEST, HY_EQ, 1));

        results = libdnf::dnf_module_query(filters, sack, repos, install_root, platformModule);
        debugDumpQueryResults(results);

        const std::vector<std::string> module_list {
            "base-runtime:f26:2:e3b0c442:x86_64",
            "base-runtime:rhel73:1:e3b0c442:x86_64",
            "httpd:2.2:1:e3b0c442:x86_64",
            "httpd:2.4:2:3b6beaf8:x86_64",
        };
        CPPUNIT_ASSERT(results.size() == module_list.size());
        for (const auto &modspec : module_list)
            CPPUNIT_ASSERT_MESSAGE(modspec, moduleExists(modspec, results));
    }

    {
        logger->debug("query with filtering by enabled modules");

        std::vector<std::shared_ptr<ModulePackage>> results;
        std::vector<libdnf::Filter> filters;

        filters.push_back(libdnf::Filter(HY_MOD_ENABLED, HY_EQ, 1));

        results = libdnf::dnf_module_query(filters, sack, repos, install_root, platformModule);
        debugDumpQueryResults(results);

        const std::vector<std::string> module_list {
        // NOTE: base-runtime was disabled by testDisable()
        //  "base-runtime:f26:1:e3b0c442:x86_64",
        //  "base-runtime:f26:2:e3b0c442:x86_64",
            "httpd:2.4:1:3b6beaf8:x86_64",
            "httpd:2.4:2:3b6beaf8:x86_64",
        };
        CPPUNIT_ASSERT(results.size() >= module_list.size());
        for (const auto &modspec : module_list)
            CPPUNIT_ASSERT_MESSAGE(modspec, moduleExists(modspec, results));
    }

    {
        logger->debug("query with filtering by disabled modules");

        std::vector<std::shared_ptr<ModulePackage>> results;
        std::vector<libdnf::Filter> filters;

        filters.push_back(libdnf::Filter(HY_MOD_DISABLED, HY_EQ, 1));

        results = libdnf::dnf_module_query(filters, sack, repos, install_root, platformModule);
        debugDumpQueryResults(results);

        const std::vector<std::string> module_list {
        // NOTE: base-runtime was disabled by testDisable()
            "base-runtime:f26:1:e3b0c442:x86_64",
            "base-runtime:f26:2:e3b0c442:x86_64",
            "base-runtime:rhel73:1:e3b0c442:x86_64",
        };
        CPPUNIT_ASSERT(results.size() == module_list.size());
        for (const auto &modspec : module_list)
            CPPUNIT_ASSERT_MESSAGE(modspec, moduleExists(modspec, results));
    }

    {
        logger->debug("query with filtering by default modules");

        std::vector<std::shared_ptr<ModulePackage>> results;
        std::vector<libdnf::Filter> filters;

        filters.push_back(libdnf::Filter(HY_MOD_DEFAULT, HY_EQ, 1));

        results = libdnf::dnf_module_query(filters, sack, repos, install_root, platformModule);
        debugDumpQueryResults(results);

        const std::vector<std::string> module_list {
            "base-runtime:f26:1:e3b0c442:x86_64",
            "base-runtime:f26:2:e3b0c442:x86_64",
            "httpd:2.4:1:3b6beaf8:x86_64",
            "httpd:2.4:2:3b6beaf8:x86_64",
        };
        CPPUNIT_ASSERT(results.size() == module_list.size());
        for (const auto &modspec : module_list)
            CPPUNIT_ASSERT_MESSAGE(modspec, moduleExists(modspec, results));
    }

    {
        const std::string modname = "httpd";

        logger->debug("query with filtering by spec name equality " + modname);

        std::vector<std::shared_ptr<ModulePackage>> results;
        std::vector<libdnf::Filter> filters;

        filters.push_back(libdnf::Filter(HY_MOD_NSVCAP, HY_EQ, modname.c_str()));

        results = libdnf::dnf_module_query(filters, sack, repos, install_root, platformModule);
        debugDumpQueryResults(results);

        const std::vector<std::string> module_list {
            "httpd:2.2:1:e3b0c442:x86_64",
            "httpd:2.4:1:3b6beaf8:x86_64",
            "httpd:2.4:2:3b6beaf8:x86_64",
        };
        CPPUNIT_ASSERT(results.size() == module_list.size());
        for (const auto &modspec : module_list)
            CPPUNIT_ASSERT_MESSAGE(modspec, moduleExists(modspec, results));
    }

    {
        const std::string modname = "HtTpD";

        logger->debug("query with filtering by case insensitive spec name equality " + modname);

        std::vector<std::shared_ptr<ModulePackage>> results;
        std::vector<libdnf::Filter> filters;

        filters.push_back(libdnf::Filter(HY_MOD_NSVCAP, HY_EQ|HY_ICASE, modname.c_str()));

        results = libdnf::dnf_module_query(filters, sack, repos, install_root, platformModule);
        debugDumpQueryResults(results);

        const std::vector<std::string> module_list {
            "httpd:2.2:1:e3b0c442:x86_64",
            "httpd:2.4:1:3b6beaf8:x86_64",
            "httpd:2.4:2:3b6beaf8:x86_64",
        };
        CPPUNIT_ASSERT(results.size() == module_list.size());
        for (const auto &modspec : module_list)
            CPPUNIT_ASSERT_MESSAGE(modspec, moduleExists(modspec, results));
    }

    {
        const std::string modname = "ttp";

        logger->debug("query with filtering by spec name substring " + modname);

        std::vector<std::shared_ptr<ModulePackage>> results;
        std::vector<libdnf::Filter> filters;

        filters.push_back(libdnf::Filter(HY_MOD_NSVCAP, HY_SUBSTR, modname.c_str()));

        results = libdnf::dnf_module_query(filters, sack, repos, install_root, platformModule);
        debugDumpQueryResults(results);

        const std::vector<std::string> module_list {
            "httpd:2.2:1:e3b0c442:x86_64",
            "httpd:2.4:1:3b6beaf8:x86_64",
            "httpd:2.4:2:3b6beaf8:x86_64",
        };
        CPPUNIT_ASSERT(results.size() == module_list.size());
        for (const auto &modspec : module_list)
            CPPUNIT_ASSERT_MESSAGE(modspec, moduleExists(modspec, results));
    }

    {
        const std::string modname = "Base";

        logger->debug("query with filtering by case insensitive spec name substring " + modname);

        std::vector<std::shared_ptr<ModulePackage>> results;
        std::vector<libdnf::Filter> filters;

        filters.push_back(libdnf::Filter(HY_MOD_NSVCAP, HY_SUBSTR|HY_ICASE, modname.c_str()));

        results = libdnf::dnf_module_query(filters, sack, repos, install_root, platformModule);
        debugDumpQueryResults(results);

        const std::vector<std::string> module_list {
            "base-runtime:f26:1:e3b0c442:x86_64",
            "base-runtime:f26:2:e3b0c442:x86_64",
            "base-runtime:rhel73:1:e3b0c442:x86_64",
        };
        CPPUNIT_ASSERT(results.size() == module_list.size());
        for (const auto &modspec : module_list)
            CPPUNIT_ASSERT_MESSAGE(modspec, moduleExists(modspec, results));
    }

    {
        const std::string modname = "ht*";

        logger->debug("query with filtering by spec name glob " + modname);

        std::vector<std::shared_ptr<ModulePackage>> results;
        std::vector<libdnf::Filter> filters;

        filters.push_back(libdnf::Filter(HY_MOD_NSVCAP, HY_GLOB, modname.c_str()));

        results = libdnf::dnf_module_query(filters, sack, repos, install_root, platformModule);
        debugDumpQueryResults(results);

        CPPUNIT_ASSERT(results.size() == 3);
        const std::vector<std::string> module_list {
            "httpd:2.2:1:e3b0c442:x86_64",
            "httpd:2.4:1:3b6beaf8:x86_64",
            "httpd:2.4:2:3b6beaf8:x86_64",
        };
        CPPUNIT_ASSERT(results.size() == module_list.size());
        for (const auto &modspec : module_list)
            CPPUNIT_ASSERT_MESSAGE(modspec, moduleExists(modspec, results));
    }

    {
        const std::string modname = "Base*Time*";

        logger->debug("query with filtering by case insensitive spec name glob" + modname);

        std::vector<std::shared_ptr<ModulePackage>> results;
        std::vector<libdnf::Filter> filters;

        filters.push_back(libdnf::Filter(HY_MOD_NSVCAP, HY_GLOB|HY_ICASE, modname.c_str()));

        results = libdnf::dnf_module_query(filters, sack, repos, install_root, platformModule);
        debugDumpQueryResults(results);

        const std::vector<std::string> module_list {
            "base-runtime:f26:1:e3b0c442:x86_64",
            "base-runtime:f26:2:e3b0c442:x86_64",
            "base-runtime:rhel73:1:e3b0c442:x86_64",
        };
        CPPUNIT_ASSERT(results.size() == module_list.size());
        for (const auto &modspec : module_list)
            CPPUNIT_ASSERT_MESSAGE(modspec, moduleExists(modspec, results));
    }

    {
        logger->debug("query with multiple filters");

        std::vector<std::shared_ptr<ModulePackage>> results;
        std::vector<libdnf::Filter> filters;

        filters.push_back(libdnf::Filter(HY_MOD_NSVCAP, HY_EQ, "httpd"));
        filters.push_back(libdnf::Filter(HY_MOD_LATEST, HY_EQ, 1));

        results = libdnf::dnf_module_query(filters, sack, repos, install_root, platformModule);
        debugDumpQueryResults(results);

        const std::vector<std::string> module_list {
            "httpd:2.2:1:e3b0c442:x86_64",
            "httpd:2.4:2:3b6beaf8:x86_64",
        };
        CPPUNIT_ASSERT(results.size() == module_list.size());
        for (const auto &modspec : module_list)
            CPPUNIT_ASSERT_MESSAGE(modspec, moduleExists(modspec, results));
    }

    {
        const std::string glob = "*:*:*:*:*/*";

        logger->debug("query with filtering by spec glob " + glob);

        std::vector<std::shared_ptr<ModulePackage>> results;
        std::vector<libdnf::Filter> filters;

        filters.push_back(libdnf::Filter(HY_MOD_NSVCAP, HY_GLOB, glob.c_str()));

        results = libdnf::dnf_module_query(filters, sack, repos, install_root, platformModule);
        debugDumpQueryResults(results);

        const std::vector<std::string> module_list {
            "base-runtime:f26:1:e3b0c442:x86_64",
            "base-runtime:f26:2:e3b0c442:x86_64",
            "base-runtime:rhel73:1:e3b0c442:x86_64",
            "httpd:2.2:1:e3b0c442:x86_64",
            "httpd:2.4:1:3b6beaf8:x86_64",
            "httpd:2.4:2:3b6beaf8:x86_64",
        };
        CPPUNIT_ASSERT(results.size() == module_list.size());
        for (const auto &modspec : module_list)
            CPPUNIT_ASSERT_MESSAGE(modspec, moduleExists(modspec, results));
    }

    {
        const std::string glob = "httpd:*.4";

        logger->debug("query with filtering by spec glob " + glob);

        std::vector<std::shared_ptr<ModulePackage>> results;
        std::vector<libdnf::Filter> filters;

        filters.push_back(libdnf::Filter(HY_MOD_NSVCAP, HY_GLOB, glob.c_str()));

        results = libdnf::dnf_module_query(filters, sack, repos, install_root, platformModule);
        debugDumpQueryResults(results);

        const std::vector<std::string> module_list {
            "httpd:2.4:1:3b6beaf8:x86_64",
            "httpd:2.4:2:3b6beaf8:x86_64",
        };
        CPPUNIT_ASSERT(results.size() == module_list.size());
        for (const auto &modspec : module_list)
            CPPUNIT_ASSERT_MESSAGE(modspec, moduleExists(modspec, results));
    }

    {
        const std::string glob = "*:*:*:*:*/minimal";

        logger->debug("query with filtering by spec glob " + glob);

        std::vector<std::shared_ptr<ModulePackage>> results;
        std::vector<libdnf::Filter> filters;

        filters.push_back(libdnf::Filter(HY_MOD_NSVCAP, HY_GLOB, glob.c_str()));

        results = libdnf::dnf_module_query(filters, sack, repos, install_root, platformModule);
        debugDumpQueryResults(results);

        const std::vector<std::string> module_list {
            "base-runtime:f26:1:e3b0c442:x86_64",
            "base-runtime:f26:2:e3b0c442:x86_64",
            "base-runtime:rhel73:1:e3b0c442:x86_64",
        };
        CPPUNIT_ASSERT(results.size() == module_list.size());
        for (const auto &modspec : module_list)
            CPPUNIT_ASSERT_MESSAGE(modspec, moduleExists(modspec, results));
    }

    {
        const std::string glob = "base-runtime/minimal";

        logger->debug("query with filtering by spec glob " + glob);

        std::vector<std::shared_ptr<ModulePackage>> results;
        std::vector<libdnf::Filter> filters;

        filters.push_back(libdnf::Filter(HY_MOD_NSVCAP, HY_GLOB, glob.c_str()));

        results = libdnf::dnf_module_query(filters, sack, repos, install_root, platformModule);
        debugDumpQueryResults(results);

        const std::vector<std::string> module_list {
            "base-runtime:f26:1:e3b0c442:x86_64",
            "base-runtime:f26:2:e3b0c442:x86_64",
            "base-runtime:rhel73:1:e3b0c442:x86_64",
        };
        CPPUNIT_ASSERT(results.size() == module_list.size());
        for (const auto &modspec : module_list)
            CPPUNIT_ASSERT_MESSAGE(modspec, moduleExists(modspec, results));
    }

    {
        const std::string glob = "base-runtime/minimal";

        logger->debug("query with filtering by spec glob " + glob);

        std::vector<std::shared_ptr<ModulePackage>> results;
        std::vector<libdnf::Filter> filters;

        filters.push_back(libdnf::Filter(HY_MOD_NSVCAP, HY_GLOB, glob.c_str()));

        results = libdnf::dnf_module_query(filters, sack, repos, install_root, platformModule);
        debugDumpQueryResults(results);

        const std::vector<std::string> module_list {
            "base-runtime:f26:1:e3b0c442:x86_64",
            "base-runtime:f26:2:e3b0c442:x86_64",
            "base-runtime:rhel73:1:e3b0c442:x86_64",
        };
        CPPUNIT_ASSERT(results.size() == module_list.size());
        for (const auto &modspec : module_list)
            CPPUNIT_ASSERT_MESSAGE(modspec, moduleExists(modspec, results));
    }

    {
        const std::string glob = "base-runtime:/minimal";

        logger->debug("query with filtering by spec glob " + glob);

        std::vector<std::shared_ptr<ModulePackage>> results;
        std::vector<libdnf::Filter> filters;

        filters.push_back(libdnf::Filter(HY_MOD_NSVCAP, HY_GLOB, glob.c_str()));

        results = libdnf::dnf_module_query(filters, sack, repos, install_root, platformModule);
        debugDumpQueryResults(results);

        const std::vector<std::string> module_list {
            "base-runtime:f26:1:e3b0c442:x86_64",
            "base-runtime:f26:2:e3b0c442:x86_64",
            "base-runtime:rhel73:1:e3b0c442:x86_64",
        };
        CPPUNIT_ASSERT(results.size() == module_list.size());
        for (const auto &modspec : module_list)
            CPPUNIT_ASSERT_MESSAGE(modspec, moduleExists(modspec, results));
    }

    {
        const std::string glob = "*:rhel73/minimal";

        logger->debug("query with filtering by spec glob " + glob);

        std::vector<std::shared_ptr<ModulePackage>> results;
        std::vector<libdnf::Filter> filters;

        filters.push_back(libdnf::Filter(HY_MOD_NSVCAP, HY_GLOB, glob.c_str()));

        results = libdnf::dnf_module_query(filters, sack, repos, install_root, platformModule);
        debugDumpQueryResults(results);

        const std::vector<std::string> module_list {
            "base-runtime:rhel73:1:e3b0c442:x86_64",
        };
        CPPUNIT_ASSERT(results.size() == module_list.size());
        for (const auto &modspec : module_list)
            CPPUNIT_ASSERT_MESSAGE(modspec, moduleExists(modspec, results));
    }

    {
        const std::string glob = "*::::/minimal";

        logger->debug("query with filtering by spec glob " + glob);

        std::vector<std::shared_ptr<ModulePackage>> results;
        std::vector<libdnf::Filter> filters;

        filters.push_back(libdnf::Filter(HY_MOD_NSVCAP, HY_GLOB, glob.c_str()));

        results = libdnf::dnf_module_query(filters, sack, repos, install_root, platformModule);
        debugDumpQueryResults(results);

        const std::vector<std::string> module_list {
            "base-runtime:f26:1:e3b0c442:x86_64",
            "base-runtime:f26:2:e3b0c442:x86_64",
            "base-runtime:rhel73:1:e3b0c442:x86_64",
        };
        CPPUNIT_ASSERT(results.size() == module_list.size());
        for (const auto &modspec : module_list)
            CPPUNIT_ASSERT_MESSAGE(modspec, moduleExists(modspec, results));
    }

    {
        const std::string glob = "*/doc";

        logger->debug("query with filtering by spec glob " + glob);

        std::vector<std::shared_ptr<ModulePackage>> results;
        std::vector<libdnf::Filter> filters;

        filters.push_back(libdnf::Filter(HY_MOD_NSVCAP, HY_GLOB, glob.c_str()));

        results = libdnf::dnf_module_query(filters, sack, repos, install_root, platformModule);
        debugDumpQueryResults(results);

        const std::vector<std::string> module_list {
            "httpd:2.2:1:e3b0c442:x86_64",
            "httpd:2.4:1:3b6beaf8:x86_64",
            "httpd:2.4:2:3b6beaf8:x86_64",
        };
        CPPUNIT_ASSERT(results.size() == module_list.size());
        for (const auto &modspec : module_list)
            CPPUNIT_ASSERT_MESSAGE(modspec, moduleExists(modspec, results));
    }

    {
        const std::string glob = "*:*2*:*:*:*/*";

        logger->debug("query with filtering by spec glob " + glob);

        std::vector<std::shared_ptr<ModulePackage>> results;
        std::vector<libdnf::Filter> filters;

        filters.push_back(libdnf::Filter(HY_MOD_NSVCAP, HY_GLOB, glob.c_str()));

        results = libdnf::dnf_module_query(filters, sack, repos, install_root, platformModule);
        debugDumpQueryResults(results);

        const std::vector<std::string> module_list {
            "base-runtime:f26:1:e3b0c442:x86_64",
            "base-runtime:f26:2:e3b0c442:x86_64",
            "httpd:2.2:1:e3b0c442:x86_64",
            "httpd:2.4:1:3b6beaf8:x86_64",
            "httpd:2.4:2:3b6beaf8:x86_64",
        };
        CPPUNIT_ASSERT(results.size() == module_list.size());
        for (const auto &modspec : module_list)
            CPPUNIT_ASSERT_MESSAGE(modspec, moduleExists(modspec, results));
    }

    {
        const std::string glob = "*:*2*";

        logger->debug("query with filtering by spec glob " + glob);

        std::vector<std::shared_ptr<ModulePackage>> results;
        std::vector<libdnf::Filter> filters;

        filters.push_back(libdnf::Filter(HY_MOD_NSVCAP, HY_GLOB, glob.c_str()));

        results = libdnf::dnf_module_query(filters, sack, repos, install_root, platformModule);
        debugDumpQueryResults(results);

        const std::vector<std::string> module_list {
            "base-runtime:f26:1:e3b0c442:x86_64",
            "base-runtime:f26:2:e3b0c442:x86_64",
            "httpd:2.2:1:e3b0c442:x86_64",
            "httpd:2.4:1:3b6beaf8:x86_64",
            "httpd:2.4:2:3b6beaf8:x86_64",
        };
        CPPUNIT_ASSERT(results.size() == module_list.size());
        for (const auto &modspec : module_list)
            CPPUNIT_ASSERT_MESSAGE(modspec, moduleExists(modspec, results));
    }

    {
        const std::string glob = "*:*:1:*:*/*";

        logger->debug("query with filtering by spec glob " + glob);

        std::vector<std::shared_ptr<ModulePackage>> results;
        std::vector<libdnf::Filter> filters;

        filters.push_back(libdnf::Filter(HY_MOD_NSVCAP, HY_GLOB, glob.c_str()));

        results = libdnf::dnf_module_query(filters, sack, repos, install_root, platformModule);
        debugDumpQueryResults(results);

        const std::vector<std::string> module_list {
            "base-runtime:f26:1:e3b0c442:x86_64",
            "base-runtime:rhel73:1:e3b0c442:x86_64",
            "httpd:2.2:1:e3b0c442:x86_64",
            "httpd:2.4:1:3b6beaf8:x86_64",
        };
        CPPUNIT_ASSERT(results.size() == module_list.size());
        for (const auto &modspec : module_list)
            CPPUNIT_ASSERT_MESSAGE(modspec, moduleExists(modspec, results));
    }

    {
        const std::string glob = "*:*:1:*:*";

        logger->debug("query with filtering by spec glob " + glob);

        std::vector<std::shared_ptr<ModulePackage>> results;
        std::vector<libdnf::Filter> filters;

        filters.push_back(libdnf::Filter(HY_MOD_NSVCAP, HY_GLOB, glob.c_str()));

        results = libdnf::dnf_module_query(filters, sack, repos, install_root, platformModule);
        debugDumpQueryResults(results);

        const std::vector<std::string> module_list {
            "base-runtime:f26:1:e3b0c442:x86_64",
            "base-runtime:rhel73:1:e3b0c442:x86_64",
            "httpd:2.2:1:e3b0c442:x86_64",
            "httpd:2.4:1:3b6beaf8:x86_64",
        };
        CPPUNIT_ASSERT(results.size() == module_list.size());
        for (const auto &modspec : module_list)
            CPPUNIT_ASSERT_MESSAGE(modspec, moduleExists(modspec, results));
    }

    {
        const std::string glob = "*:*:*:*beaf8:*/*";

        logger->debug("query with filtering by spec glob " + glob);

        std::vector<std::shared_ptr<ModulePackage>> results;
        std::vector<libdnf::Filter> filters;

        filters.push_back(libdnf::Filter(HY_MOD_NSVCAP, HY_GLOB, glob.c_str()));

        results = libdnf::dnf_module_query(filters, sack, repos, install_root, platformModule);
        debugDumpQueryResults(results);

        const std::vector<std::string> module_list {
            "httpd:2.4:1:3b6beaf8:x86_64",
            "httpd:2.4:2:3b6beaf8:x86_64",
        };
        CPPUNIT_ASSERT(results.size() == module_list.size());
        for (const auto &modspec : module_list)
            CPPUNIT_ASSERT_MESSAGE(modspec, moduleExists(modspec, results));
    }

    {
        const std::string glob = "*:*:*:*:x86_64/*";

        logger->debug("query with filtering by spec glob " + glob);

        std::vector<std::shared_ptr<ModulePackage>> results;
        std::vector<libdnf::Filter> filters;

        filters.push_back(libdnf::Filter(HY_MOD_NSVCAP, HY_GLOB, glob.c_str()));

        results = libdnf::dnf_module_query(filters, sack, repos, install_root, platformModule);
        debugDumpQueryResults(results);

        const std::vector<std::string> module_list {
            "base-runtime:f26:1:e3b0c442:x86_64",
            "base-runtime:f26:2:e3b0c442:x86_64",
            "base-runtime:rhel73:1:e3b0c442:x86_64",
            "httpd:2.2:1:e3b0c442:x86_64",
            "httpd:2.4:1:3b6beaf8:x86_64",
            "httpd:2.4:2:3b6beaf8:x86_64",
        };
        CPPUNIT_ASSERT(results.size() == module_list.size());
        for (const auto &modspec : module_list)
            CPPUNIT_ASSERT_MESSAGE(modspec, moduleExists(modspec, results));
    }

    {
        const std::string glob = "*:*:*:*:aarch64/*";

        logger->debug("query with filtering by spec glob " + glob);

        std::vector<std::shared_ptr<ModulePackage>> results;
        std::vector<libdnf::Filter> filters;

        filters.push_back(libdnf::Filter(HY_MOD_NSVCAP, HY_GLOB, glob.c_str()));

        results = libdnf::dnf_module_query(filters, sack, repos, install_root, platformModule);
        debugDumpQueryResults(results);

        CPPUNIT_ASSERT(results.size() == 0);
    }

    {
        const std::string glob = "*t*:*2*:2:*8:x*/*d*";

        logger->debug("query with filtering by spec glob " + glob);

        std::vector<std::shared_ptr<ModulePackage>> results;
        std::vector<libdnf::Filter> filters;

        filters.push_back(libdnf::Filter(HY_MOD_NSVCAP, HY_GLOB, glob.c_str()));

        results = libdnf::dnf_module_query(filters, sack, repos, install_root, platformModule);
        debugDumpQueryResults(results);

        const std::vector<std::string> module_list {
            "httpd:2.4:2:3b6beaf8:x86_64",
        };
        CPPUNIT_ASSERT(results.size() == module_list.size());
        for (const auto &modspec : module_list)
            CPPUNIT_ASSERT_MESSAGE(modspec, moduleExists(modspec, results));
    }

    {
        const std::string rpmname = "bash";

        logger->debug("query with filtering by profile rpm name equality " + rpmname);

        std::vector<std::shared_ptr<ModulePackage>> results;
        std::vector<libdnf::Filter> filters;

        filters.push_back(libdnf::Filter(HY_MOD_PROFILE_RPMNAME, HY_EQ, rpmname.c_str()));

        results = libdnf::dnf_module_query(filters, sack, repos, install_root, platformModule);
        debugDumpQueryResults(results);

        const std::vector<std::string> module_list {
            "base-runtime:f26:1:e3b0c442:x86_64",
            "base-runtime:f26:2:e3b0c442:x86_64",
            "base-runtime:rhel73:1:e3b0c442:x86_64",
        };
        CPPUNIT_ASSERT(results.size() == module_list.size());
        for (const auto &modspec : module_list)
            CPPUNIT_ASSERT_MESSAGE(modspec, moduleExists(modspec, results));
    }

    {
        const std::string rpmname = "*doc";

        logger->debug("query with filtering by profile rpm name glob " + rpmname);

        std::vector<std::shared_ptr<ModulePackage>> results;
        std::vector<libdnf::Filter> filters;

        filters.push_back(libdnf::Filter(HY_MOD_PROFILE_RPMNAME, HY_GLOB, rpmname.c_str()));

        results = libdnf::dnf_module_query(filters, sack, repos, install_root, platformModule);
        debugDumpQueryResults(results);

        const std::vector<std::string> module_list {
            "httpd:2.2:1:e3b0c442:x86_64",
            "httpd:2.4:1:3b6beaf8:x86_64",
            "httpd:2.4:2:3b6beaf8:x86_64",
        };
        CPPUNIT_ASSERT(results.size() == module_list.size());
        for (const auto &modspec : module_list)
            CPPUNIT_ASSERT_MESSAGE(modspec, moduleExists(modspec, results));
    }

    {
        const std::string nevra = "bash-0:4.4.12-2.x86_64";

        logger->debug("query with filtering by artifact rpm nevra equality " + nevra);

        std::vector<std::shared_ptr<ModulePackage>> results;
        std::vector<libdnf::Filter> filters;

        filters.push_back(libdnf::Filter(HY_MOD_ARTIFACT_NEVRA, HY_EQ, nevra.c_str()));

        results = libdnf::dnf_module_query(filters, sack, repos, install_root, platformModule);
        debugDumpQueryResults(results);

        const std::vector<std::string> module_list {
            "base-runtime:f26:1:e3b0c442:x86_64",
            "base-runtime:f26:2:e3b0c442:x86_64",
        };
        CPPUNIT_ASSERT(results.size() == module_list.size());
        for (const auto &modspec : module_list)
            CPPUNIT_ASSERT_MESSAGE(modspec, moduleExists(modspec, results));
    }

    {
        const std::vector<std::string> nevra_list = {
            "bash-0:4.4.12-2.x86_64",
            "libnghttp2-0:1.21.1-1.x86_64",
        };

        std::ostringstream oss;
        oss << "query with filtering by artifact rpm nevra list equality:";
        for ( const auto &nevra : nevra_list ) {
            oss << " " << nevra.c_str();
        }
        logger->debug(oss.str());

        std::vector<std::shared_ptr<ModulePackage>> results;
        std::vector<libdnf::Filter> filters;

        std::vector<const char *> nevra_cstr_list(nevra_list.size()+1);
        int i = 0;
        for ( const auto &nevra : nevra_list ) {
            nevra_cstr_list.at(i++) = nevra.c_str();
        }
        filters.push_back(libdnf::Filter(HY_MOD_ARTIFACT_NEVRA, HY_EQ, nevra_cstr_list.data()));

        results = libdnf::dnf_module_query(filters, sack, repos, install_root, platformModule);
        debugDumpQueryResults(results);

        const std::vector<std::string> module_list {
            "base-runtime:f26:1:e3b0c442:x86_64",
            "base-runtime:f26:2:e3b0c442:x86_64",
            "httpd:2.4:1:3b6beaf8:x86_64",
            "httpd:2.4:2:3b6beaf8:x86_64",
        };
        CPPUNIT_ASSERT(results.size() == module_list.size());
        for (const auto &modspec : module_list)
            CPPUNIT_ASSERT_MESSAGE(modspec, moduleExists(modspec, results));
    }

    {
        const std::vector<std::string> nevra_list = {
            "bash-0:4.4.12-2.x86_64",
            "libnghttp2-0:1.21.1-1.x86_64",
        };

        std::ostringstream oss;
        oss << "query with filtering by latest + artifact rpm nevra list equality:";
        for ( const auto &nevra : nevra_list ) {
            oss << " " << nevra.c_str();
        }
        logger->debug(oss.str());

        std::vector<std::shared_ptr<ModulePackage>> results;
        std::vector<libdnf::Filter> filters;

        std::vector<const char *> nevra_cstr_list(nevra_list.size()+1);
        int i = 0;
        for ( const auto &nevra : nevra_list ) {
            nevra_cstr_list.at(i++) = nevra.c_str();
        }
        filters.push_back(libdnf::Filter(HY_MOD_ARTIFACT_NEVRA, HY_EQ, nevra_cstr_list.data()));
        filters.push_back(libdnf::Filter(HY_MOD_LATEST, HY_EQ, 1));

        results = libdnf::dnf_module_query(filters, sack, repos, install_root, platformModule);
        debugDumpQueryResults(results);

        const std::vector<std::string> module_list {
            "base-runtime:f26:2:e3b0c442:x86_64",
            "httpd:2.4:2:3b6beaf8:x86_64",
        };
        CPPUNIT_ASSERT(results.size() == module_list.size());
        for (const auto &modspec : module_list)
            CPPUNIT_ASSERT_MESSAGE(modspec, moduleExists(modspec, results));
    }


    {
        // this is here only as a template to copy/paste into new tests

        const std::vector<std::string> module_list {
            "base-runtime:f26:1:e3b0c442:x86_64",
            "base-runtime:f26:2:e3b0c442:x86_64",
            "base-runtime:rhel73:1:e3b0c442:x86_64",
            "httpd:2.2:1:e3b0c442:x86_64",
            "httpd:2.4:1:3b6beaf8:x86_64",
            "httpd:2.4:2:3b6beaf8:x86_64",
        };
    }
}

void ModuleTest::testReset()
{
    auto logger = libdnf::Log::getLogger();
    GPtrArray *repos = dnf_context_get_repos(context);
    auto sack = dnf_context_get_sack(context);
    auto install_root = dnf_context_get_install_root(context);
    auto platformModule = dnf_context_get_platform_module(context);

    logger->debug("called ModuleTest::testReset()");

    /* revert to original state by re-enabling base-runtime module */
    {
        std::vector<std::string> module_list{"base-runtime"};
        CPPUNIT_ASSERT(libdnf::dnf_module_enable(module_list, sack, repos, install_root, platformModule));
        libdnf::ConfigParser parser{};
        parser.read(TESTDATADIR "/modules/etc/dnf/modules.d/base-runtime.module");
        CPPUNIT_ASSERT(parser.getValue("base-runtime", "state") == "enabled");
        CPPUNIT_ASSERT(parser.getValue("base-runtime", "stream") == "f26");
    }
}
