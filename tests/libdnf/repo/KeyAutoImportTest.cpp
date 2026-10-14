#include "KeyAutoImportTest.hpp"

#include <libdnf/conf/ConfigMain.hpp>
#include <libdnf/conf/ConfigRepo.hpp>
#include <libdnf/conf/Option.hpp>
#include <libdnf/hy-iutil-private.hpp>
#include <libdnf/repo/Repo-private.hpp>

#include <glib.h>

#include <fstream>
#include <memory>
#include <sys/stat.h>
#include <unistd.h>

#define UNITTEST_DIR "/tmp/libdnfXXXXXX"

CPPUNIT_TEST_SUITE_REGISTRATION(KeyAutoImportTest);

namespace {

void createFile(const std::string & path)
{
    std::ofstream file(path);
    file << "not a real key\n";
    CPPUNIT_ASSERT(file.good());
}

bool autoImportable(libdnf::ConfigRepo & conf, const std::string & keyUrl,
                    const std::string & keysDir)
{
    return libdnf::Repo::Impl::isKeyAutoImportable(conf, keyUrl, keysDir);
}

} // namespace

// The layout under the temporary directory:
//   keys/key                  a key inside the trusted directory
//   keys/key name             a key whose name needs URL encoding
//   keys/sub/key              a key in a subdirectory
//   keys/alias -> key         a symlink that stays inside the directory
//   keys/escape -> ../outside a symlink that leaves the directory
//   keys-other/key            a sibling directory sharing the prefix
//   keys2/key                 a key in a second trusted directory
//   outside                   a key outside the trusted directory
void KeyAutoImportTest::setUp()
{
    tmpdir = g_strdup(UNITTEST_DIR);
    char *retptr = mkdtemp(tmpdir);
    CPPUNIT_ASSERT(retptr);
    const std::string base = tmpdir;
    keysDir = base + "/keys";

    CPPUNIT_ASSERT_EQUAL(0, mkdir(keysDir.c_str(), 0700));
    CPPUNIT_ASSERT_EQUAL(0, mkdir((keysDir + "/sub").c_str(), 0700));
    CPPUNIT_ASSERT_EQUAL(0, mkdir((base + "/keys-other").c_str(), 0700));
    CPPUNIT_ASSERT_EQUAL(0, mkdir((base + "/keys2").c_str(), 0700));
    createFile(keysDir + "/key");
    createFile(keysDir + "/key name");
    createFile(keysDir + "/sub/key");
    createFile(base + "/keys-other/key");
    createFile(base + "/keys2/key");
    createFile(base + "/outside");
    CPPUNIT_ASSERT_EQUAL(0, symlink("key", (keysDir + "/alias").c_str()));
    CPPUNIT_ASSERT_EQUAL(0, symlink("../outside", (keysDir + "/escape").c_str()));
}

void KeyAutoImportTest::tearDown()
{
    g_autoptr(GError) error = nullptr;
    dnf_remove_recursive_v2(tmpdir, &error);
    g_assert_no_error(error);
    g_free(tmpdir);
}

// The option is off by default, and assumeno wins over the option.
void KeyAutoImportTest::testOptionAndAssumeno()
{
    libdnf::ConfigMain mainConfig;
    libdnf::ConfigRepo repoConfig(mainConfig);
    const std::string keyUrl = "file://" + keysDir + "/key";

    CPPUNIT_ASSERT_EQUAL(false, autoImportable(repoConfig, keyUrl, keysDir));

    repoConfig.auto_import_local_keys().set(libdnf::Option::Priority::REPOCONFIG, true);
    CPPUNIT_ASSERT_EQUAL(true, autoImportable(repoConfig, keyUrl, keysDir));

    mainConfig.assumeno().set(libdnf::Option::Priority::COMMANDLINE, true);
    CPPUNIT_ASSERT_EQUAL(false, autoImportable(repoConfig, keyUrl, keysDir));
}

void KeyAutoImportTest::testKeysInsideDirectory()
{
    libdnf::ConfigMain mainConfig;
    libdnf::ConfigRepo repoConfig(mainConfig);
    repoConfig.auto_import_local_keys().set(libdnf::Option::Priority::REPOCONFIG, true);
    const std::string url = "file://" + keysDir;

    CPPUNIT_ASSERT_EQUAL(true, autoImportable(repoConfig, url + "/key", keysDir));
    CPPUNIT_ASSERT_EQUAL(true, autoImportable(repoConfig, url + "/sub/key", keysDir));
    CPPUNIT_ASSERT_EQUAL(true, autoImportable(repoConfig, url + "/alias", keysDir));
    // The URL is decoded before the path is checked, as librepo decodes it when reading the key.
    CPPUNIT_ASSERT_EQUAL(true, autoImportable(repoConfig, url + "/key%20name", keysDir));
    // The directory may be given with a trailing slash.
    CPPUNIT_ASSERT_EQUAL(true, autoImportable(repoConfig, url + "/key", keysDir + "/"));
}

// Only the "file:///path" spelling is accepted, the others keep the confirmation.
void KeyAutoImportTest::testUrlSpellings()
{
    libdnf::ConfigMain mainConfig;
    libdnf::ConfigRepo repoConfig(mainConfig);
    repoConfig.auto_import_local_keys().set(libdnf::Option::Priority::REPOCONFIG, true);
    const std::string key = keysDir + "/key";

    CPPUNIT_ASSERT_EQUAL(false, autoImportable(repoConfig, "file:" + key, keysDir));
    CPPUNIT_ASSERT_EQUAL(false, autoImportable(repoConfig, key, keysDir));
    CPPUNIT_ASSERT_EQUAL(false, autoImportable(repoConfig, "file://localhost" + key, keysDir));
    // A relative path has two slashes, so it fails the spelling check.
    CPPUNIT_ASSERT_EQUAL(false, autoImportable(repoConfig, "file://keys/key", keysDir));
    CPPUNIT_ASSERT_EQUAL(false, autoImportable(repoConfig, "https://example.com/key", keysDir));
    CPPUNIT_ASSERT_EQUAL(false, autoImportable(repoConfig, "", keysDir));
}

void KeyAutoImportTest::testKeysOutsideDirectory()
{
    libdnf::ConfigMain mainConfig;
    libdnf::ConfigRepo repoConfig(mainConfig);
    repoConfig.auto_import_local_keys().set(libdnf::Option::Priority::REPOCONFIG, true);
    const std::string base = tmpdir;
    const std::string url = "file://" + keysDir;

    CPPUNIT_ASSERT_EQUAL(false, autoImportable(repoConfig, "file://" + base + "/outside", keysDir));
    CPPUNIT_ASSERT_EQUAL(false, autoImportable(repoConfig, url + "/../outside", keysDir));
    CPPUNIT_ASSERT_EQUAL(false, autoImportable(repoConfig, url + "/%2e%2e/outside", keysDir));
    CPPUNIT_ASSERT_EQUAL(false, autoImportable(repoConfig, url + "/escape", keysDir));
    CPPUNIT_ASSERT_EQUAL(
        false, autoImportable(repoConfig, "file://" + base + "/keys-other/key", keysDir));
    CPPUNIT_ASSERT_EQUAL(false, autoImportable(repoConfig, url, keysDir));
    CPPUNIT_ASSERT_EQUAL(false, autoImportable(repoConfig, url + "/missing", keysDir));
    // A trusted directory that does not exist trusts nothing.
    CPPUNIT_ASSERT_EQUAL(false, autoImportable(repoConfig, url + "/key", base + "/missing"));
    // A URL that does not decode, or decodes to a path with a zero byte, is not a path.
    CPPUNIT_ASSERT_EQUAL(false, autoImportable(repoConfig, url + "/key%", keysDir));
    CPPUNIT_ASSERT_EQUAL(false, autoImportable(repoConfig, url + "/key%zz", keysDir));
    CPPUNIT_ASSERT_EQUAL(false, autoImportable(repoConfig, url + "/key%00", keysDir));
}

// The trusted directories are given as a colon-separated list, a key in any of them is accepted.
void KeyAutoImportTest::testKeysDirsList()
{
    libdnf::ConfigMain mainConfig;
    libdnf::ConfigRepo repoConfig(mainConfig);
    repoConfig.auto_import_local_keys().set(libdnf::Option::Priority::REPOCONFIG, true);
    const std::string base = tmpdir;
    const std::string key1 = "file://" + keysDir + "/key";
    const std::string key2 = "file://" + base + "/keys2/key";
    const std::string dirs = keysDir + ":" + base + "/keys2";

    CPPUNIT_ASSERT_EQUAL(true, autoImportable(repoConfig, key1, dirs));
    CPPUNIT_ASSERT_EQUAL(true, autoImportable(repoConfig, key2, dirs));
    CPPUNIT_ASSERT_EQUAL(false, autoImportable(repoConfig, key2, keysDir));
    CPPUNIT_ASSERT_EQUAL(false, autoImportable(repoConfig, "file://" + base + "/outside", dirs));
    // Empty entries are skipped, so a leading, trailing or doubled colon trusts nothing extra.
    CPPUNIT_ASSERT_EQUAL(true, autoImportable(repoConfig, key2, ":" + dirs + "::"));
    CPPUNIT_ASSERT_EQUAL(false, autoImportable(repoConfig, key1, ""));
    CPPUNIT_ASSERT_EQUAL(false, autoImportable(repoConfig, key1, ":"));
}

// The public method checks against the directories set at build time.
void KeyAutoImportTest::testRepoMethod()
{
    libdnf::ConfigMain mainConfig;
    std::unique_ptr<libdnf::ConfigRepo> repoConfig(new libdnf::ConfigRepo(mainConfig));
    repoConfig->auto_import_local_keys().set(libdnf::Option::Priority::REPOCONFIG, true);
    libdnf::Repo repo("test", std::move(repoConfig));

    // A key outside the build-time directories is rejected, and so is a missing key inside them.
    CPPUNIT_ASSERT_EQUAL(false, repo.isKeyAutoImportable("file://" + keysDir + "/key"));
    const std::string dirs = AUTO_IMPORT_KEYS_DIRS;
    CPPUNIT_ASSERT_EQUAL(
        false, repo.isKeyAutoImportable("file://" + dirs.substr(0, dirs.find(':')) + "/missing"));
    CPPUNIT_ASSERT_EQUAL(false, repo.isKeyAutoImportable("https://example.com/key"));
}
