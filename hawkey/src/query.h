#ifndef HY_QUERY_H
#define HY_QUERY_H

/* hawkey */
#include "packagelist.h"
#include "sack.h"
#include "types.h"

enum _hy_comparison_type_e {
    /* part 1: flags that mix with all types */
    HY_ICASE  = 1 << 0,
    HY_NOT    = 1 << 1,
    HY_COMPARISON_FLAG_MASK = HY_ICASE | HY_NOT,

    /* part 2: comparison types that mix with each other */
    HY_EQ	= (1 << 8),
    HY_LT	= (1 << 9),
    HY_GT	= (1 << 10),

    /* part 3: comparison types that only make sense for strings */
    HY_SUBSTR	= (1 << 11),
    HY_GLOB     = (1 << 12),

    /* part 4: frequently used combinations */
    HY_NEQ	= HY_EQ | HY_NOT,
};

enum _hy_key_name_e {
    HY_PKG_NAME,
    HY_PKG_ARCH,
    HY_PKG_EVR,
    HY_PKG_SUMMARY,
    HY_PKG_FILE,
    HY_PKG_REPO,
    HY_PKG_PROVIDES,
    HY_PKG_LATEST,
    HY_PKG_DOWNGRADES,
    HY_PKG_UPGRADES,
    HY_PKG_OBSOLETING
};

HyQuery hy_query_create(HySack sack);
void hy_query_free(HyQuery q);
void hy_query_clear(HyQuery q);
void hy_query_filter(HyQuery q, int keyname, int filter_type, const char *match);
void hy_query_filter_in(HyQuery q, int keyname, int filter_type,
			const char **matches);
void hy_query_filter_provides(HyQuery q, int filter_type, const char *name,
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

#endif /* HY_QUERY_H */
