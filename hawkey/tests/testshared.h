#ifndef TESTSHARED_H
#define TESTSHARED_H

// libsolv
#include <solv/pooltypes.h>

// hawkey
#include "src/repo.h"

#define UNITTEST_DIR "/tmp/hawkeyXXXXXX"
#define YUM_DIR_SUFFIX "yum/repodata/"
#define YUM_REPO_NAME "nevermac"
#define TEST_FIXED_ARCH "x86_64"
#define TEST_EXPECT_SYSTEM_PKGS 9
#define TEST_EXPECT_SYSTEM_NSOLVABLES TEST_EXPECT_SYSTEM_PKGS
#define TEST_EXPECT_MAIN_NSOLVABLES 13
#define TEST_EXPECT_UPDATES_NSOLVABLES 6
#define TEST_EXPECT_YUM_NSOLVABLES 2

HyRepo glob_for_repofiles(Pool *pool, const char *repo_name, const char *path);
int load_repo(Pool *pool, const char *name, const char *path, int installed);

#endif /* TESTSHARED_H */
