#ifndef HY_SACK_H
#define HY_SACK_H

/* libsolv */
#include "solv/pool.h"

/* hawkey */
#include "repo.h"
#include "package.h"
#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define HY_SYSTEM_REPO_NAME "@System"
#define HY_SYSTEM_RPMDB "/var/lib/rpm/Packages"
#define HY_CMDLINE_REPO_NAME "@commandline"
#define HY_EXT_FILENAMES "-filenames"
#define HY_EXT_PRESTO "-presto"

#define HY_E_CACHE_WRITE -1	// Cache write error
#define HY_E_IO 2		// I/O error
#define HY_E_QUERY 3		// Ill-formed Query.

enum _hy_sack_repo_load_flags {
    HY_BUILD_CACHE	= 1 << 0,
    HY_LOAD_FILELISTS	= 1 << 1,
    HY_LOAD_PRESTO	= 1 << 2
};

HySack hy_sack_create(const char *cache_path, const char *arch);
void hy_sack_free(HySack sack);
char *hy_sack_give_cache_fn(HySack sack, const char *reponame, const char *ext);
const char **hy_sack_list_arches(HySack sack);
void hy_sack_set_installonly(HySack sack, const char **installonly);
void hy_sack_create_cmdline_repo(HySack sack);
HyPackage hy_sack_add_cmdline_package(HySack sack, const char *fn);
int hy_sack_count(HySack sack);

/**
 * Load RPMDB, the system package database.
 *
 * @returns           0 on success, HY_E_IO on fatal error,
 *		      HY_E_CACHE_WRITE on cache write error.
 */
int hy_sack_load_system_repo(HySack sack, HyRepo a_hrepo, int flags);
int hy_sack_load_yum_repo(HySack sack, HyRepo hrepo, int flags);

#ifdef __cplusplus
}
#endif

#endif /* HY_SACK_H */
