#include "ConfigRepoTest.hpp"

#include <libdnf/conf/Option.hpp>

CPPUNIT_TEST_SUITE_REGISTRATION(ConfigRepoTest);

void ConfigRepoTest::setUp()
{}

// repo_gpgcheck_auto_import_keys is an opt-in: it must default to false so
// that, unless explicitly enabled, dnf keeps prompting before importing a
// repository metadata signing key.
void ConfigRepoTest::testRepoGpgcheckAutoImportKeysDefault()
{
    libdnf::ConfigMain mainConfig;
    CPPUNIT_ASSERT_EQUAL(false, mainConfig.repo_gpgcheck_auto_import_keys().getValue());

    libdnf::ConfigRepo repoConfig(mainConfig);
    CPPUNIT_ASSERT_EQUAL(false, repoConfig.repo_gpgcheck_auto_import_keys().getValue());
}

// The repo option is an OptionChild of the main option, so setting it in the
// [main] config is inherited by repositories that do not override it.
void ConfigRepoTest::testRepoGpgcheckAutoImportKeysInheritsFromMain()
{
    libdnf::ConfigMain mainConfig;
    mainConfig.repo_gpgcheck_auto_import_keys().set(libdnf::Option::Priority::MAINCONFIG, true);

    libdnf::ConfigRepo repoConfig(mainConfig);
    CPPUNIT_ASSERT_EQUAL(true, repoConfig.repo_gpgcheck_auto_import_keys().getValue());

    // A per-repo value overrides the inherited main value.
    repoConfig.repo_gpgcheck_auto_import_keys().set(libdnf::Option::Priority::REPOCONFIG, false);
    CPPUNIT_ASSERT_EQUAL(false, repoConfig.repo_gpgcheck_auto_import_keys().getValue());
}

// The option must be registered under its config name so it can be set from a
// .repo / dnf.conf file or via --setopt.
void ConfigRepoTest::testRepoGpgcheckAutoImportKeysParsedFromConfig()
{
    libdnf::ConfigMain mainConfig;
    libdnf::ConfigRepo repoConfig(mainConfig);

    auto & optionItem = repoConfig.optBinds().at("repo_gpgcheck_auto_import_keys");
    optionItem.newString(libdnf::Option::Priority::REPOCONFIG, "1");
    CPPUNIT_ASSERT_EQUAL(true, repoConfig.repo_gpgcheck_auto_import_keys().getValue());

    optionItem.newString(libdnf::Option::Priority::RUNTIME, "false");
    CPPUNIT_ASSERT_EQUAL(false, repoConfig.repo_gpgcheck_auto_import_keys().getValue());
}
