#ifndef QUERY_H
#define QUERY_H

// hawkey
#include "packagelist.h"
#include "sack.h"
#include "types.h"

enum _hy_filter_flag_e {
    HY_FF_ICASE  = 1 << 8,
    HY_FILTER_FLAG_MASK = HY_FF_ICASE,
};

enum _filter_type_e {
    FT_EQ	= (1 << 0),
    FT_LT	= (1 << 1),
    FT_GT	= (1 << 2),
    FT_NEQ	= FT_LT|FT_GT,
    FT_SUBSTR	= (1 << 3),
    FT_GLOB     = (1 << 4)
};

enum _key_name_e {
    KN_PKG_NAME,
    KN_PKG_ARCH,
    KN_PKG_SUMMARY,
    KN_PKG_REPO,
    KN_PKG_PROVIDES,
    KN_PKG_LATEST,
    KN_PKG_UPDATES,
    KN_PKG_OBSOLETING
};

HyQuery hy_query_create(HySack sack);
void hy_query_free(HyQuery q);
void hy_query_filter(HyQuery q, int keyname, int filter_type, const char *match);
void hy_query_filter_provides(HyQuery q, int filter_type, const char *name,
			   const char *evr);
void hy_query_filter_updates(HyQuery q, int val);
void hy_query_filter_latest(HyQuery q, int val);
void hy_query_filter_obsoleting(HyQuery q, int val);

HyPackageList hy_query_run(HyQuery q);

// internal/deprecated

HyPackageList sack_f_by_name(HySack sack, const char *name);
HyPackageList sack_f_by_summary(HySack sack, const char *summary_substr);

#endif // QUERY_H