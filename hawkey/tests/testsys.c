#define _GNU_SOURCE
#include <check.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

// libsolv
#include <solv/repo.h>
#include <solv/util.h>

// hawkey
#include "src/package.h"
#include "src/query.h"
#include "src/sack_internal.h"
#include "src/util.h"
#include "testsys.h"

void
assert_nevra_eq(HyPackage pkg, const char *nevra)
{
    char *pkg_nevra = hy_package_get_nevra(pkg);
    ck_assert_str_eq(pkg_nevra, nevra);
    solv_free(pkg_nevra);
}

HyPackage
by_name(HySack sack, const char *name)
{
    HyQuery q = hy_query_create(sack);
    hy_query_filter(q, HY_PKG_NAME, HY_EQ, name);
    HyPackageList plist = hy_query_run(q);
    hy_query_free(q);
    HyPackage pkg = hy_packagelist_get_clone(plist, 0);
    hy_packagelist_free(plist);

    return pkg;
}

HyPackage
by_name_repo(HySack sack, const char *name, const char *repo)
{
    HyQuery q = hy_query_create(sack);
    hy_query_filter(q, HY_PKG_NAME, HY_EQ, name);
    hy_query_filter(q, HY_PKG_REPONAME, HY_EQ, repo);
    HyPackageList plist = hy_query_run(q);
    hy_query_free(q);
    HyPackage pkg = hy_packagelist_get_clone(plist, 0);
    hy_packagelist_free(plist);

    return pkg;
}

void
dump_packagelist(HyPackageList plist)
{
    for (int i = 0; i < hy_packagelist_count(plist); ++i) {
	HyPackage pkg = hy_packagelist_get(plist, i);
	char *nvra = hy_package_get_nevra(pkg);
	printf("\t%s\n", nvra);
	hy_free(nvra);
    }
}

void
dump_query_results(HyQuery query)
{
    HyPackageList plist = hy_query_run(query);
    dump_packagelist(plist);
    hy_packagelist_free(plist);
}

int
logfile_size(HySack sack)
{
    const int fd = fileno(sack->log_out);
    struct stat st;

    fstat(fd, &st);
    return st.st_size;
}

int
query_count_results(HyQuery query)
{
    HyPackageList plist = hy_query_run(query);
    int ret = hy_packagelist_count(plist);
    hy_packagelist_free(plist);
    return ret;
}
