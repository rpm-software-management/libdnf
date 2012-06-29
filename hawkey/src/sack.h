#ifndef HY_SACK_H
#define HY_SACK_H

/* libsolv */
#include "solv/pool.h"

/* hawkey */
#include "repo.h"
#include "package.h"
#include "types.h"

#define HY_SYSTEM_REPO_NAME "@System"
#define HY_SYSTEM_RPMDB "/var/lib/rpm/Packages"
#define HY_CMDLINE_REPO_NAME "@commandline"
#define HY_EXT_FILENAMES "-filenames"
#define HY_EXT_PRESTO "-presto"

HySack hy_sack_create(const char *cache_path, const char *arch);
void hy_sack_free(HySack sack);
char *hy_sack_give_cache_fn(HySack sack, const char *reponame, const char *ext);
void hy_sack_set_installonly(HySack sack, const char **installonly);
void hy_sack_create_cmdline_repo(HySack sack);
HyPackage hy_sack_add_cmdline_rpm(HySack sack, const char *fn);
void hy_sack_load_rpm_repo(HySack sack);
int hy_sack_load_yum_repo(HySack sack, HyRepo repo);
int hy_sack_load_filelists(HySack sack);
int hy_sack_load_presto(HySack sack);
int hy_sack_write_all_repos(HySack sack);
int hy_sack_write_filelists(HySack sack);
int hy_sack_write_presto(HySack sack);

#endif /* HY_SACK_H */
