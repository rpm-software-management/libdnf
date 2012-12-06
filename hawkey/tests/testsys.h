#ifndef TESTSYS_H
#define TESTSYS_H

// libsolv
#include <solv/pooltypes.h>

// hawkey
#include "src/packagelist.h"
#include "src/sack.h"
#include "testshared.h"

void assert_nevra_eq(HyPackage pkg, const char *nevra);
HyPackage by_name(HySack sack, const char *name);
HyPackage by_name_repo(HySack sack, const char *name, const char *repo);
void dump_packagelist(HyPackageList plist);
void dump_query_results(HyQuery query);
int logfile_size(HySack sack);
int query_count_results(HyQuery query);
HyRepo repo_by_name(Pool *pool, const char *name);

#endif /* TESTSYS_H */
