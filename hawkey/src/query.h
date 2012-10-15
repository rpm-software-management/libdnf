#ifndef HY_QUERY_H
#define HY_QUERY_H

#ifdef __cplusplus
extern "C" {
#endif

/* hawkey */
#include "packagelist.h"
#include "sack.h"
#include "types.h"

HyQuery hy_query_create(HySack sack);
void hy_query_free(HyQuery q);
void hy_query_clear(HyQuery q);
HyQuery hy_query_clone(HyQuery q);
int hy_query_filter(HyQuery q, int keyname, int filter_type, const char *match);
int hy_query_filter_in(HyQuery q, int keyname, int filter_type,
			const char **matches);
int hy_query_filter_num(HyQuery q, int keyname, int filter_type,
			int match);
int hy_query_filter_num_in(HyQuery q, int keyname, int filter_type, int nmatches,
			   const int *matches);
int hy_query_filter_package_in(HyQuery q, int filter_type,
			       const HyPackageList plist);
int hy_query_filter_provides(HyQuery q, int filter_type, const char *name,
			     const char *evr);
int hy_query_filter_requires(HyQuery q, int filter_type, const char *name,
			     const char *evr);

/**
 * Filter packages that are named same as an installed package but lower version.
 *
 * NOTE: this does not guerantee packages filtered in this way are installable.
 */
void hy_query_filter_downgrades(HyQuery q, int val);
/**
 * Filter packages that are named same as an installed package but higher version.
 *
 * NOTE: this does not guerantee packages filtered in this way are installable.
 */
void hy_query_filter_upgrades(HyQuery q, int val);
void hy_query_filter_latest(HyQuery q, int val);
void hy_query_filter_obsoleting(HyQuery q, int val);

HyPackageList hy_query_run(HyQuery q);

#ifdef __cplusplus
}
#endif

#endif /* HY_QUERY_H */
