#include "ConfigRepoTest.hpp"

#include <libdnf/conf/ConfigMain.hpp>
#include <libdnf/conf/ConfigRepo.hpp>
#include <libdnf/conf/Option.hpp>

CPPUNIT_TEST_SUITE_REGISTRATION(ConfigRepoTest);

// The option is an opt-in, so it must default to false on both configs.
void ConfigRepoTest::testAutoImportLocalKeysDefault()
{
    libdnf::ConfigMain mainConfig;
    CPPUNIT_ASSERT_EQUAL(false, mainConfig.auto_import_local_keys().getValue());

    libdnf::ConfigRepo repoConfig(mainConfig);
    CPPUNIT_ASSERT_EQUAL(false, repoConfig.auto_import_local_keys().getValue());
}

// The repository option is a child of the main option: a repository inherits the
// value set in [main] and a value set for the repository overrides it.
void ConfigRepoTest::testAutoImportLocalKeysInheritsFromMain()
{
    libdnf::ConfigMain mainConfig;
    mainConfig.auto_import_local_keys().set(libdnf::Option::Priority::MAINCONFIG, true);

    libdnf::ConfigRepo repoConfig(mainConfig);
    CPPUNIT_ASSERT_EQUAL(true, repoConfig.auto_import_local_keys().getValue());

    repoConfig.auto_import_local_keys().set(libdnf::Option::Priority::REPOCONFIG, false);
    CPPUNIT_ASSERT_EQUAL(false, repoConfig.auto_import_local_keys().getValue());
    CPPUNIT_ASSERT_EQUAL(true, mainConfig.auto_import_local_keys().getValue());
}

// The option is registered under its name on both configs, so it parses from
// dnf.conf, from a .repo file and from --setopt.
void ConfigRepoTest::testAutoImportLocalKeysParsedFromConfig()
{
    libdnf::ConfigMain mainConfig;
    libdnf::ConfigRepo repoConfig(mainConfig);

    mainConfig.optBinds().at("auto_import_local_keys")
        .newString(libdnf::Option::Priority::MAINCONFIG, "1");
    CPPUNIT_ASSERT_EQUAL(true, mainConfig.auto_import_local_keys().getValue());
    CPPUNIT_ASSERT_EQUAL(true, repoConfig.auto_import_local_keys().getValue());

    repoConfig.optBinds().at("auto_import_local_keys")
        .newString(libdnf::Option::Priority::REPOCONFIG, "False");
    CPPUNIT_ASSERT_EQUAL(false, repoConfig.auto_import_local_keys().getValue());
}
