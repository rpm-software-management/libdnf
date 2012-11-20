#ifndef TESTSYS_H
#define TESTSYS_H

// libsolv
#include <solv/pooltypes.h>

// hawkey
#include "src/packagelist.h"
#include "src/sack.h"

#define UNITTEST_DIR "/tmp/hawkeyXXXXXX"
#define YUM_DIR_SUFFIX "yum/repodata/"
#define YUM_REPO_NAME "nevermac"
#define TEST_FIXED_ARCH "x86_64"
#define TEST_EXPECT_SYSTEM_PKGS 9
#define TEST_EXPECT_SYSTEM_NSOLVABLES TEST_EXPECT_SYSTEM_PKGS
#define TEST_EXPECT_MAIN_NSOLVABLES 13
#define TEST_EXPECT_UPDATES_NSOLVABLES 6
#define TEST_EXPECT_YUM_NSOLVABLES 2

void assert_nevra_eq(HyPackage pkg, const char *nevra);
HyPackage by_name(HySack sack, const char *name);
HyPackage by_name_repo(HySack sack, const char *name, const char *repo);
void dump_packagelist(HyPackageList plist);
int logfile_size(HySack sack);
int query_count_results(HyQuery query);
HyRepo repo_by_name(Pool *pool, const char *name);

#endif /* TESTSYS_H */
