#ifndef TESTSYS_H
#define TESTSYS_H

// hawkey
#include "src/packagelist.h"
#include "src/sack.h"

#define UNITTEST_DIR "/tmp/hawkeyXXXXXX"
#define YUM_DIR_SUFFIX "yum/repodata/"
#define YUM_REPO_NAME "nevermac"
#define TEST_FIXED_ARCH "x86_64"
#define TEST_EXPECT_SYSTEM_PKGS 7
#define TEST_EXPECT_SYSTEM_NSOLVABLES TEST_EXPECT_SYSTEM_PKGS
#define TEST_EXPECT_MAIN_NSOLVABLES 13
#define TEST_EXPECT_UPDATES_NSOLVABLES 2
#define TEST_EXPECT_YUM_NSOLVABLES 2

HyPackage by_name(HySack sack, const char *name);
void dump_packagelist(HyPackageList plist);
HyRepo glob_for_repofiles(Pool *pool, const char *repo_name, const char *path);
int load_repo(Pool *pool, const char *name, const char *path, int installed);
int logfile_size(HySack sack);
int query_count_results(HyQuery query);
HyRepo repo_by_name(Pool *pool, const char *name);

#endif /* TESTSYS_H */
